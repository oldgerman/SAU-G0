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

	//如果是第一次硬复位开机或者eeprom检测版本号失败，那么强制打印自检信息
	if(firstPwrOffToRUN == true || checkVersion == false)
		selfCheck();

	firstPwrOffToRUN = false;
    /*任务注册*/	//子任务里放for(;;)会阻塞其他任务//注意调度时间占比，影响主屏幕时间的秒点闪烁周期的平均度
    mtmMain.Register(GUI_Update, 20);                	//25ms：屏幕刷新
    mtmMain.Register(RTC_Update, 100);                 //100ms：RTC监控
    mtmMain.Register(MIX_Update, 200);   			 	//200ms：杂七杂八的传感器监控
    mtmMain.Register(USART_Update, 1000);            	//1000ms：COM收发监控
}

void loop(){
	while(1) {
		mtmMain.Running(HAL_GetTick());
	}
}

void MIX_Update()
{
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
