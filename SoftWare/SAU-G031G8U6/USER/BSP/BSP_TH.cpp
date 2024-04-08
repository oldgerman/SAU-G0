/*
 * BSP_TH_Sensor.cpp
 *
 *  Created on: 2022年6月24日
 *      Author: OldGerman
 */

#include "BSP.h"
#include "DFRobot_AHT20.h"

#ifndef DBG_PRINT_TH
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PRINT_TH usb_printf
#else
	#define DBG_PRINT_TH(...)
	#endif
#endif


static DFRobot_AHT20 aht20(&FRToSI2C1);
static int16_t th_C_X10;		//温度有正负，-9.9~85.0
static uint8_t th_RH_X1;		//范围0~100，无小数点
static bool th_DataUpdated;	//温湿度计数据更新标志

void TH_Init() {
	th_RH_X1 = 88;
	th_C_X10 = 888;
	th_DataUpdated = false;

	uint8_t status;
	if ((status = aht20.begin()) != 0) {
		DBG_PRINT_TH("AHT20 sensor initialization failed. error status : %d\r\n", status);
	}
}

void TH_Update() {
	if (aht20.measurementFSM()) {	//每4次状态机状态不重复的切换完成一次测量
		th_C_X10 = aht20.getTemperature_C();
		th_RH_X1 = aht20.getHumidity_RH();
		th_DataUpdated = true;
	}
	else
		th_DataUpdated = false;
}

/**
  * @brief  检查温湿度数据是否更新
  * @param  无
  * @retval 温湿度计数据更新标志
  */
bool TH_DataUpdated() {
	return th_DataUpdated;
}

/**
  * @brief  获取温度
  * @param  无
  * @retval sec:时间(秒)
  */
int16_t TH_GetDataC_X10()
{
    return th_C_X10;
}

/**
  * @brief  获取湿度
  * @param  无
  * @retval sec:时间(秒)
  */
uint8_t TH_GetDataRH_X1()
{
    return th_RH_X1;
}
