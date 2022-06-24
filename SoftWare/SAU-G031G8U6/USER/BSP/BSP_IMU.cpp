/*
 * BSP_Power.cpp
 *
 *  Created on: 2022年6月22日
 *      Author: OldGerman
 */

/*
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */


/*
 ******************************************************************************
 * @modified OldGerman
 * 	2022/06/22 修改此示例文件以适配SAU-G0
 ******************************************************************************
 */

/*
 * If you need to run this example on a different hardware platform a
 * modification of the functions: `platform_write`, `platform_read`,
 * `tx_com` and 'platform_init' is required.
 *
 */

/* ATTENTION: By default the driver is little endian. If you need switch
 *            to big endian please see "Endianness definitions" in the
 *            header file of the driver (_reg.h).
 */
/* 注意：默认情况下，驱动程序是小端。如果你需要开关
 * 到大端请参阅“字节序定义”
 * 驱动程序的头文件 (_reg.h)
 */


#define SENSOR_BUS hi2c2


/* Includes ------------------------------------------------------------------*/
#include "BSP.h"
#include "Settings.h"	//	提供systemSto.data.Sensitivity
#include <string.h>
#include <stdio.h>
#include "lis3dh_reg.h"
#include "stm32G0xx_hal.h"
#include "usart.h"
#include "gpio.h"
#include "i2c.h"

/* Private macro -------------------------------------------------------------*/
#define LIS3DH_I2C_ADDRESS (25 << 1)	//25D=11001b=0x19（7bit i2c地址）

#ifndef DEBUG_PRINT_IMU
#if 1  //< Change 0 to 1 to open debug macro and check program debug information
#define DEBUG_PRINT_IMU usb_printf
#else
	#define DEBUG_PRINT_IMU(...)
	#endif
#endif

/* Private variables ---------------------------------------------------------*/
//static uint8_t whoamI;
uint32_t lastMovementTime = 0;		//首次进入shouldShuntDown()函数置为1才会累计时间
/* Extern variables ----------------------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
/*
 *   WARNING:
 *   Functions declare in this section are defined at the end of this file
 *   and are strictly related to the hardware platform used.
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len);
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len);
static void platform_init(void);


static stmdev_ctx_t dev_ctx; //The device interface function

void lis3dh_wake_up_setup(void)
{
  /*  Initialize mems driver interface */
  lis3dh_int1_cfg_t int1_cfg;
  lis3dh_ctrl_reg3_t ctrl_reg3;
  uint8_t dummy;

//CTRL_REG2
  /* High-pass filter enabled on interrupt activity 1
   * CTRL_REG2 ---> HP_IA1
   * High-pass filter enabled for AOI function on interrupt 1 */
  lis3dh_high_pass_int_conf_set(&dev_ctx, LIS3DH_ON_INT1_GEN);
  /* Enable HP filter for wake-up event detection
   * 为唤醒事件检测启用 HP 过滤器
   * Use this setting to remove gravity on data output
   * 使用此设置消除数据输出的重力
   * */
  lis3dh_high_pass_on_outputs_set(&dev_ctx, PROPERTY_ENABLE);

//CTRL_REG3
  /* Enable AOI1 on int1 pin
   * 将IA1中断使能到INT1 Pin上*/
  lis3dh_pin_int1_config_get(&dev_ctx, &ctrl_reg3);
  ctrl_reg3.i1_ia1 = PROPERTY_ENABLE;
  lis3dh_pin_int1_config_set(&dev_ctx, &ctrl_reg3);

//CTRL_REG6
  /*将默认INT pin1是高电平有效的极性设置为低电平*/
  lis3dh_ctrl_reg6_t ctrl_reg6;
  lis3dh_pin_int2_config_get(&dev_ctx, &ctrl_reg6);
  ctrl_reg6.int_polarity = 1;	//active-low
  lis3dh_pin_int2_config_set(&dev_ctx, &ctrl_reg6);

//CTRL_REG5
  //不锁存，以避免执行进入STOP1模式的代码时，触发了加速度计中断，无法再次敲击板子唤醒
  /* Interrupt 1 pin latched 中断1锁存*/
//  lis3dh_int1_pin_notification_mode_set(&dev_ctx,
//                                        LIS3DH_INT1_LATCHED);
//CTRL_REG4
  /* Set full scale to 2 g
   * 设置全量程为2g */
  lis3dh_full_scale_set(&dev_ctx, LIS3DH_2g);

//INT1_THS
  /* Set interrupt threshold
   * 例如16，那么16x16 = 256 mg
   * 注意不同Full Scale下每LSb代表的加速度步进值不一样：
   * 1 LSb = 16 mg @ FS = ±2 g
   * 1 LSb = 32 mg @ FS = ±4 g
   * 1 LSb = 62 mg @ FS = ±8 g
   * 1 LSb = 186 mg @ FS = ±16 g
   * */
  IMU_SetThreshold();
//INT1_DURATION
  /* Set no time duration
   * 中断能触发的最短持续时间为0 */
  lis3dh_int1_gen_duration_set(&dev_ctx, 0);

//INT1_REFERENCE
  /* Dummy read to force the HP filter to current acceleration value.
   * 虚拟读取以强制 HP 滤波器为当前加速度值。
   * CTRL_REG的HPM[1:0]默认值是00，即High-pass filter mode configuration为Normal Mode (reset by reading REFERENCE (26h))
   * 这里读一次REFERENCE(0x26)寄存器以复位高通滤波器，为啥要复位？
   * */
  lis3dh_filter_reference_get(&dev_ctx, &dummy);

//INT1_CFG
  /* Configure wake-up interrupt event on all axis
   * 设置唤醒中断事件在所以的轴向
   * 为啥都是某一个轴的 high event or on Direction recognition？不能用low event or on Direction recognition吗？*/
  lis3dh_int1_gen_conf_get(&dev_ctx, &int1_cfg);
  int1_cfg.zhie = PROPERTY_ENABLE;
  int1_cfg.yhie = PROPERTY_ENABLE;
  int1_cfg.xhie = PROPERTY_ENABLE;
  int1_cfg.aoi = PROPERTY_DISABLE;	//AOI配置为或模式，任意一个轴发生高事件都能触发中断
  lis3dh_int1_gen_conf_set(&dev_ctx, &int1_cfg);


//CTRL_REG1 CTRL_REG4
  /*
   * 请参考datasheet P17酌情选择工作模式以降低电流，以下为节选
	| Hz   | 8bit[uA] | 10bit[uA] | 12bit[uA] |
	| ---- | -------- | --------- | --------- |
	| 1    | 2        | 2         | 2         |
	| 10   | 3        | 4         | 4         |
	| 25   | 4        | 6         | 6         |
	| 50   | 6        | 11        | 11        |
	| 100  | 10       | 20        | 20        |
	| ...  | ...      | ...       | ...       |
   */
  /* Set device in HR mode (High-resolution output mode)
   * 设置为输出分辨率*/
  lis3dh_operating_mode_set(&dev_ctx, LIS3DH_LP_8bit);
  /* Set Output Data Rate to 100 Hz
   * 设置输出频率*/
  lis3dh_data_rate_set(&dev_ctx, LIS3DH_ODR_25Hz);


}

void IMU_Init(void)
{
	/* Initialize the device interface function */
	dev_ctx.write_reg = platform_write;
	dev_ctx.read_reg = platform_read;
	dev_ctx.handle = &SENSOR_BUS;
	/* Initialize platform specific hardware */
	platform_init();
	/*  Check device ID */
//	lis3dh_device_id_get(&dev_ctx, &whoamI);
//
//	if (whoamI != LIS3DH_ID) {
//		while (1) {
//			/* manage here device not found */
//		}
//	}

	lis3dh_wake_up_setup();
}

void IMU_Update(){
	//动作时间lastMovementTime转移到外部中断回调函数更新
#if 0
	//以下代码只适用于中断锁存模式，而我改为不锁存之后不适用
	lis3dh_int1_src_t src;
	/* Read INT pin 1 in polling mode
	 * or read src status register
	 */
	lis3dh_int1_gen_source_get(&dev_ctx, &src);

	if (src.xh || src.yh || src.zh) {
		lis3dh_int1_gen_source_get(&dev_ctx, &src);
		lastMovementTime = HAL_GetTick();	//更新动作时间
		DEBUG_PRINT_IMU("wake-up detected: x %d, y %d, z %d\r\n", src.xh, src.yh, src.zh);
	}
#endif
}

void IMU_SetThreshold(){
	lis3dh_int1_gen_threshold_set(&dev_ctx, map(systemSto.data.Sensitivity, 0, 100, 0, 127));
}

//暂时没用
uint8_t IMU_GetLSbSteps(){
	/*
	 * 注意不同Full Scale下每LSb代表的加速度步进值不一样：
	 * 1 LSb = 16 mg @ FS = ±2 g
	 * 1 LSb = 32 mg @ FS = ±4 g
	 * 1 LSb = 62 mg @ FS = ±8 g
	 * 1 LSb = 186 mg @ FS = ±16 g
	 */
	uint8_t LSb_Steps = 0;

	//读取全量程以知道每LSb的g值
	lis3dh_fs_t val = LIS3DH_2g;
	lis3dh_full_scale_set(&dev_ctx, val);
	switch (val)
	{
	case LIS3DH_2g:
		LSb_Steps = 16;
		break;
	case LIS3DH_4g:
		LSb_Steps = 32;
		break;
	case LIS3DH_8g:
		LSb_Steps = 62;
		break;
	case LIS3DH_16g:
		LSb_Steps = 186;
		break;
	default:
		LSb_Steps = 0;
		break;
	}
	return LSb_Steps;
}

/*
 * @brief  Write generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to write
 * @param  bufp      pointer to data to write in register reg
 * @param  len       number of consecutive register to write
 *
 */
static int32_t platform_write(void *handle, uint8_t reg, const uint8_t *bufp,
                              uint16_t len)
{
  /* Write multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Write((I2C_HandleTypeDef*)handle, LIS3DH_I2C_ADDRESS, reg,
                    I2C_MEMADD_SIZE_8BIT, (uint8_t*) bufp, len, 1000);

  return 0;
}

/*
 * @brief  Read generic device register (platform dependent)
 *
 * @param  handle    customizable argument. In this examples is used in
 *                   order to select the correct sensor bus handler.
 * @param  reg       register to read
 * @param  bufp      pointer to buffer that store the data read
 * @param  len       number of consecutive register to read
 *
 */
static int32_t platform_read(void *handle, uint8_t reg, uint8_t *bufp,
                             uint16_t len)
{
  /* Read multiple command */
  reg |= 0x80;
  HAL_I2C_Mem_Read((I2C_HandleTypeDef*)handle, LIS3DH_I2C_ADDRESS, reg,
                   I2C_MEMADD_SIZE_8BIT, bufp, len, 1000);

  return 0;
}

/*
 * @brief  platform specific initialization (platform dependent)
 */
static void platform_init(void)
{
	;	//Nothing to do
}

