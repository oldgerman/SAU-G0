/*
 * BSP_Power.cpp
 *
 *  Created on: 2022年6月7日
 *      Author: OldGerman
 */
#include "BSP.h"
#include "stm32g0xx_hal.h"
#include "main.h"	//提供GPIO User Label
#include "Settings.h"
#include "Buttons.hpp"

//提供 MX_XX_Init(),用于从STOP1进入RUN模式时恢复外设配置
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
#include "cppports.h"	//提供setup()

#define swPressedTimeshutDown 2000		//至少长按关机时间
bool recoverFromSTOP1 = false;	//从STOP模式恢复标记

void Power_Shutdown();

/*
 * @brief 从STOP1模式退出时，恢复时钟、外设、外挂芯片的配置
 * @param  None
 * @retval None
 */
void STOP1_to_RUN(){
	//中断使退出STOP模式，在此处继续执行代码
	SystemClock_Config();
	/* Initialize all configured peripherals */
	  MX_GPIO_Init();
	  MX_USART2_UART_Init();
	  MX_DMA_Init();
	  MX_SPI1_Init();
	  MX_I2C2_Init();
	  MX_I2C1_Init();
	  MX_ADC1_Init();
	  MX_TIM1_Init();
	/* USER CODE BEGIN 2 */
	//解锁I2C
	FRToI2CxSInit();

	setup();	//重新初始化所有外挂芯片，因为3.3V断电过
}

/*
 * @breif 打开主电源MOS函数
 */
void powerOn() {
	HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_SET);
}


static uint32_t powerOffDetect_timeOld = 0;
static int64_t powerOffDetect_tickSum = 0;

/*
 * @breif 关机长按按键检测函数
 * @param msShutDown 按键持续按多少时间关机
 * @retval 从STOP1模式回到RUN模式，返回true; 未超过需要长按的时间返回false,
 * 			这个返回值避免刚从STOP1恢复系统时屏幕亮度为0，而继续执行
 * 			Contrast_Update(Power_Shutdown);里的Power_Shutdown()再次休眠
 */
bool powerOffDetect(uint16_t ms) {
	if (!waitTime(&powerOffDetect_timeOld, ms)) {
		if (HAL_GPIO_ReadPin(KEY_OK_GPIO_Port, KEY_OK_Pin)
				== GPIO_PIN_RESET) {
			powerOffDetect_tickSum += 100;
		} else {
//			powerOffDetect_tickSum -= 100;
//			if (powerOffDetect_tickSum < 0)
				powerOffDetect_tickSum = 0;
		}
	}else
	{
		/*++ 很奇怪为啥要加这句*/
		HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_SET);
	}
	//累积关机
	if (powerOffDetect_tickSum > ms) {
		Contrast_Darken();
		HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_RESET);
		Power_Shutdown();
		return true;	//从STOP1模式回到RUN模式，返回true;
	}
	return false;
}


/**
 * @breif 获得睡眠超时阈值时间
 * @param  None
 * @retval uint32_t
 */
static uint32_t getSleepTimeout() {
	return systemSto.data.SleepTime * 1000;
}
/**
 * @breif 比较运动或按钮超时阈值，来决定返回是否睡眠
 * @param  None
 * @retval 超时并且未检测到运动和按钮动作返回true
 */
bool shouldBeSleeping() {
	if (systemSto.data.settingsBits[sysBits].bits.bit1) { 		//自动休眠位域
		if (lastMovementTime > 0 || lastButtonTime > 0) {		//只有当这两个值非0才可能进入休眠
			if ((HAL_GetTick() - lastMovementTime) > getSleepTimeout() //1000ms 临时设置的休眠超时时间
					&& (HAL_GetTick() - lastButtonTime) > getSleepTimeout()) {
				return true;
			}
		}
	}
	return false;
}


/**
 * @brief  进入STOP1模式时按照低功耗要求配置GPIO
 * @param  None
 * @retval None
 */
static void STOP1_GPIO_Config(void)
{
#if 0
	//GPIOA
#define KEY_A_Pin GPIO_PIN_1
#define KEY_B_Pin GPIO_PIN_4
#define DISP_RES_Pin GPIO_PIN_5
#define PW_HOLD_Pin GPIO_PIN_6		//不能动它，STOP1加速度计中断唤醒
#define RGB_LED_Pin GPIO_PIN_8
#define SCL2_Pin GPIO_PIN_11
#define SDA2_Pin GPIO_PIN_12
#define DISP_CS_Pin GPIO_PIN_15

	//GPIOB
#define BAT_INFO_Pin GPIO_PIN_0
#define KEY_OK_Pin GPIO_PIN_1		//不能动它，STOP1按OK键唤醒
#define DISP_DC_Pin GPIO_PIN_4
#define INT_RTC_Pin GPIO_PIN_5		//不能动它，STOP1 RTC唤醒
#define SDA1_Pin GPIO_PIN_7
#define SCL1_Pin GPIO_PIN_8
#endif
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	/*Configure GPIOA pins of GPIO_MODE_ANALOG + GPIO_NOPULL*/
	GPIO_InitStruct.Pin = KEY_A_Pin|KEY_B_Pin|DISP_RES_Pin|RGB_LED_Pin
			|DISP_CS_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pins of GPIO_MODE_ANALOG + GPIO_NOPULL*/
	GPIO_InitStruct.Pin = BAT_INFO_Pin|DISP_DC_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIOA pins of GPIO_MODE_INPUT + GPIO_PULLDOWN*/
	GPIO_InitStruct.Pin = SCL2_Pin|SDA2_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIOB pins of GPIO_MODE_INPUT + GPIO_PULLDOWN*/
	GPIO_InitStruct.Pin = SCL1_Pin|SDA1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLDOWN;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PtPin */
	GPIO_InitStruct.Pin = INT_RTC_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(INT_RTC_GPIO_Port, &GPIO_InitStruct);

}


/**
 * @brief  进入低功耗模式
 * @param  None
 * @retval None
 */
static void Power_EnterLowPowerMode()
{
	STOP1_GPIO_Config();
	/*关外设*/
	HAL_ADC_DeInit(&hadc1);	//会执行HAL_DMA_DeInit对应的通道
	HAL_I2C_DeInit(&hi2c1);
	HAL_I2C_DeInit(&hi2c2);
	HAL_SPI_DeInit(&hspi1);
	HAL_UART_DeInit(&huart2);
//	HAL_ADC_MspDeInit(&hadc1);
//	HAL_I2C_MspDeInit(&hi2c1);
//	HAL_I2C_MspDeInit(&hi2c2);
//	HAL_SPI_MspDeInit(&hspi1);
//	HAL_UART_MspDeInit(&huart2);
	HAL_TIM_Base_MspDeInit(&htim1);//关闭TIM1和TIM1的DMA通道
	//根据低功耗设计配置GPIO状态
	/*进STOP模式*/
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);//WFI(等待中断)、WFE(等待事件)
}


/**
 * @brief  电源初始化
 * @param  None
 * @retval None
 */
void Power_Init()
{
	//重置按键关机计数器
	powerOffDetect_timeOld = HAL_GetTick();
	powerOffDetect_tickSum = 0;

	/*长按中键一定时间才会使电源保持*/
	powerOn();

	//强制更新最后一次动作状态时间
	lastButtonTime = HAL_GetTick() - systemSto.data.SleepTime;
	lastMovementTime = HAL_GetTick() - systemSto.data.SleepTime;
}

/**
 * @brief  执行关机
 * @param  None
 * @retval None
 */
void Power_Shutdown()
{
	/* USER CODE BEGIN 1 */
	//在进入STOP1关外设函数前，需要保存的数据在此执行操作
	//例如更新RUN、LPW_RUN、STOP1的累计时间到EEPROM

	/* USER CODE END 1 */

	Power_EnterLowPowerMode();
}

/**
 * @brief  自动关机监控
 * @param  None
 * @retval None
 */
void Power_AutoShutdownUpdate()
{
	//在执行关机函数前判断最近一次RTC中断是否马上来临(防止进入STOP1时中断来临却得不到响应)，若是则阻止进入休眠
//	if(!RTC_AlarmWillTrigger())
	{
		if(powerOffDetect(swPressedTimeshutDown) == false)
			Contrast_Update(Power_Shutdown);
	}
//	else{
//		Contrast_Update(nullptr);
//	}
}

/**
 * @brief  是否在充电
 * @param  None
 * @retval bool
 */
bool Power_IsCharging(){
	return !HAL_GPIO_ReadPin(CHARGE_DETECT_GPIO_Port, CHARGE_DETECT_Pin);
}

