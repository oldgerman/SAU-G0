/*
 * cppports.cpp
 *
 *  Created on: May 16, 2022
 *      Author: OldGerman
 */

#include "cppports.h"
#include "MillisTaskManager.h"
#include "ee24.hpp"
#include "GUI.h"
#ifndef DBG_PRINT
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PRINT usb_printf
#else
	#define DBG_PRINT(...)
	#endif
#endif


/******************* 开机自检标记 *******************/
//检查凡是可能存在超时的函数，标记它，不存储在EEPROM中
//目前不实现本（超时跳过报告启动那些东西失败）功能，若容量有余会考虑
settingsBitsType fucTimeOutBits;

/******************* 任务调度器 *******************/
static MillisTaskManager mtmMain;
uint32_t Debug_deviceSize = 0;
uint16_t Debug_pageSize =  0;

void setup(){
	/*初始化或从STOP1退出时重置标记*/
	Power_Init();
#if 1
//主线程序

	DBG_PRINT("MCU: Initialized.\r\n");
	RGB_TurnOff();
	bool checkVersion = restoreSettings(); 	//恢复设置
	OLED_Init();		//U8g2初始化OLED
	Contrast_Init();
	GUI_Init();
	USART_Init();
	RTC_Init();
	TH_Init();
	IMU_Init();
	ADC_Init();

	//如果是第一次硬复位开机或者eeprom检测版本号失败，那么强制打印自检信息
	if(firstPwrOffToRUN == true || checkVersion == false)
		selfCheck();

	firstPwrOffToRUN = false;
    /*任务注册*/	//子任务里放for(;;)会阻塞其他任务//注意调度时间占比，影响主屏幕时间的秒点闪烁周期的平均度
    mtmMain.Register(GUI_Update, 20);                	//25ms：屏幕刷新
    mtmMain.Register(RTC_Update, 100);                 //100ms：RTC监控
    mtmMain.Register(MIX_Update, 200);   			 	//200ms：杂七杂八的传感器监控
    mtmMain.Register(USART_Update, 1000);            	//1000ms：COM收发监控
#elif 0
//EE24类临时交换I2C引脚测试
//	ee24_write_test_A();
	/*
	 * 测试     deviceSize   pageSize
	 * 24C02       256 			8		//	#define  _EEPROM_SIZE_KBIT   2
	 * 24C128	  16384		    64		//	#define  _EEPROM_SIZE_KBIT   32~1024均可，但小于32不行：，deviceSize固定为0，pageSize固定为8	--这个如何处理，宏定义先要搞一个，是不是得用HAL_Transmit?
	 */
	ee24.autoInit(true);

	while(1)
	{
		ee24.exchangeI2CPins();
		Debug_deviceSize = ee24.determineMemSize();
		Debug_pageSize =  ee24.determinePageSize();
//		int i = 0;
//		ee24.recoverI2CPins();
//		Debug_deviceSize = ee24.determineMemSize();
//		Debug_pageSize =  ee24.determinePageSize();
//		i = 0;
	}
#elif 0
	for(;;)
		loopI2cScan(2000);
#elif 0
//RTC PCF2129 闹钟中断测试
	setupRTC();
	bool mmark;
	DateTime alarmDateTime(2000, 1, 21, 0, 0, 5);	//每分钟的第5秒一次闹钟
	mmark = rtc.alarmFired();
	mmark =  rtc.clearFlagAlarm();
	mmark = rtc.setTimeAlarm(&alarmDateTime, PCF212x_A_Second);
//	mmark = rtc.setTimeAlarm(&alarmDateTime, PCF212x_A_PerSecond);
	mmark = rtc.setIntAlarm(true);
	while(1) {
		now = rtc.now();
		if(intFromRTC){			//由G031的PB5检测PCF2129的中断下降沿回调函数更改
//		if(rtc.alarmFired()){	//也可以轮询检测
			mmark = rtc.clearFlagAlarm();
			mmark = rtc.alarmFired();
			intFromRTC = false;
		}
	}
#else
	ee24.autoInit(false);
	while(1){
		ee24.eraseChip();

		HAL_Delay(1000);
	}
#endif
}

void loop(){
	while(1) {
		mtmMain.Running(HAL_GetTick());
	}
}



void MIX_Update()
{

//	IMU_Update();	//合并到screenBrightAdj()
	TH_Update();
	/*
	 * 如果intFromRTC由中断回调函数修改为TRUE，说明RTC中断产生
	 * 直到AHT20完成测量既th_MeasurementUpload为true时，才执行loopDataCollect()
	 * 并修改intFromRTC 为 false;
	 */
	if(intFromRTC && TH_DataUpdated()){
			intFromRTC = false;
			DataCollect_Update();
	}
}


