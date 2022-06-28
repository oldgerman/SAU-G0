/*
 * GUI.cpp
 *
 *  Created on: Jun 24, 2022
 *      Author: OldGerman
 */


#include "GUI.h"
#include "oled_init.h"
#include "Buttons.hpp"
#include "Page.hpp"
#include "CustomPage.hpp"
#include "string.h"	  //提供memset()
#include "BMP.h"		//提供一些字体和图标的位图

void drawFan(bool markBackFromMenu);

extern ButtonState buttons;
bool 	sysPowerOn;				//首次开机标记
static uint8_t indexFanFrame;
static uint8_t secondPrev;
static uint64_t secondPionthalf1s;		//秒点的半秒标记
static bool secondPiontColor;		//秒点的颜色
static uint32_t drawFanTimeOld;

void GUI_Init() {
	sysPowerOn = true;
	indexFanFrame = 0;
	secondPrev =  RTC_GetNowSceond();
	secondPiontColor = 1;
	drawFanTimeOld = HAL_GetTick();



	Contrast_Init();
	if(systemSto.data.settingsBits[sysBits].bits.bit0 || firstPwrOffToRUN == true){
		//绘制开机logo
		drawLogoAndVersion();
		u8g2.sendBuffer();
		Contrast_Brighten();
		uint32_t timeOld = HAL_GetTick();
		while(!waitTime(&timeOld, 888))
			;
		u8g2.clearBuffer();
	}
}



void GUI_Update() {
	Power_AutoShutdownUpdate();	//从STOP模式退出，在此处继续执行

	//一些标记变量用于按键状态锁定，因为主屏进入菜单是长按中键，菜单返回主屏也是长按中键
	static bool markBackFromMenu = false;
	static ButtonState oldButtons;
	if(markBackFromMenu) {
		if(buttons != oldButtons)
			markBackFromMenu = false;
	}

	//检测按键
	buttons = getButtonState();
	if(!markBackFromMenu) {
		switch (buttons) {
		case BUTTON_NONE:
			break;
		case BUTTON_A_SHORT:
			break;
		case BUTTON_A_LONG:
			oldButtons = buttons;
			columsAccessibility_Battery();
			u8g2.clearBuffer();
			markBackFromMenu = true;
			break;
		case BUTTON_B_SHORT:
			break;
		case BUTTON_B_LONG:
			break;
		case BUTTON_BOTH:
			break;
		case BUTTON_BOTH_LONG:
			break;
		case BUTTON_OK_SHORT:
			oldButtons = buttons;
			synchronisedTimeSys();				//同步系统设置时间到最近一次时间，兼顾初始化
			synchronisedTimeStartCollect();		//同步系采集开始时间到最近一次时间
			enterSettingsMenu(); // enter the settings menu
			u8g2.clearBuffer();
			markBackFromMenu = true;
			break;
		case BUTTON_OK_LONG:
			break;
		default:
			break;
		}
	}

	//绘制温湿度信息
	if (TH_DataUpdated() || sysPowerOn || markBackFromMenu) {
		sysPowerOn = false;
		u8g2.setDrawColor(1);
		uint16_t busZNum1, busZNum2, busPNum1;
		// th_C_X10 = 234  实际23.4摄氏度
		uint16_t C_X10 = TH_GetDataC_X10();
		uint16_t RH_X1 = TH_GetDataRH_X1();
		if(markBackFromMenu){
			C_X10 = 888;
			RH_X1 = 88;
		}
		busZNum1 = C_X10 / 100;	//2 得到十位
		C_X10 = C_X10 % 100;	//取模得到 34
		busZNum2 = C_X10 / 10;	//3 得到个位
		C_X10 = C_X10 % 10;	//取模得到4 小点数后第1位
		busPNum1 = C_X10;
		u8g2.drawBitmap(5, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum1][0]);
		u8g2.drawBitmap(21, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum2][0]);
		u8g2.drawBox(36, 20, 2, 2);	//温度小数点
		drawFan(markBackFromMenu);
		u8g2.drawBitmap(40, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busPNum1][0]);
		u8g2.drawBitmap(56, 0, icon8x14_W, icon8x14_H, icon8x14DegreeC);//	摄氏度符号

		busZNum1 = RH_X1 / 10;
		RH_X1 = RH_X1 % 10;
		busZNum2 = RH_X1;
		drawFan(markBackFromMenu);
		u8g2.drawBitmap(5, 26, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum1][0]);
		u8g2.drawBitmap(21, 26, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum2][0]);
		u8g2.drawBitmap(37, 26, icon8x10_W, icon8x10_H, icon8x10Percent);//	湿度百分符号
	}

	drawFan(markBackFromMenu);


//绘制休眠检测标记点，显示表示不休眠，不显示表示休眠时间已到
//	if(shouldBeSleeping())
//	{
//		u8g2.setDrawColor(0);
//		u8g2.drawBox(0, 0, 2, 2);
//		u8g2.setDrawColor(1);
//	}else {
//		u8g2.drawBox(0, 0, 2, 2);
//	}

	DateTime now = RTC_GetNowDateTime();
	//绘制时间
	if((secondPrev != now.second()) ||
			(HAL_GetTick() - secondPionthalf1s >= 500))
	{
		secondPionthalf1s = HAL_GetTick(); //变相同步了 os系统滴答计时的0.5秒 与 RTC的1秒的 每秒时间
		secondPrev = now.second();

		u8g2.setFont(u8g2_font_IPAandRUSLCD_tr);	//8pixel字体
		u8g2.setFontRefHeightText();
		char buffer[4] = { 0 };	//20：43
		u8g2.setDrawColor(1);//黑底白字

		//绘制时间
		sprintf(buffer, "%02d",
				now.hour()
		);
		u8g2.drawStr(37, 48, buffer);

		memset(buffer, 0, strlen(buffer));
		sprintf(buffer, "%02d",
				now.minute()
		);
		u8g2.drawStr(53, 48, buffer);

		//绘制秒计时点
		//不能这么搞，要每次变化1s时都亮灭一次，不是比如第23秒灭第24秒亮，也就是需要半秒来一个操作
		u8g2.setDrawColor(secondPiontColor);
		u8g2.drawPixel(50, 42);
		u8g2.drawPixel(50, 46);
		secondPiontColor = !secondPiontColor;
	}
	u8g2.setDrawColor(1);

	//发送oled buffer
	u8g2.sendBuffer();

}


void DataCollect_Update(){
	if(systemSto.data.settingsBits[colBits].bits.bit7){
		++systemSto.data.NumDataCollected.uint_16;	//增加已采集的数据个数,步进一次getScheduleSetting_NextDateTime()的计算结果
		if(systemSto.data.NumDataCollected.uint_16 <= systemSto.data.NumDataWillCollect){
			rtc.setIntAlarm(RESET);	//关闭Alarm中断
			rtc.clearFlagAlarm();	//清除Alarm Flag

			DateTime alarmDateTime = getScheduleSetting_NextDateTime();	//得到下次任务时间
			rtc.clearFlagAlarm();
			rtc.setTimeAlarm(&alarmDateTime, PCF212x_A_Hour);
			rtc.setIntAlarm(SET);
			numX4Type C_X4T = numSplit(TH_GetDataC_X10() * 10);
			numX4Type RH_X4T = numSplit(TH_GetDataRH_X1() * 100);
			usb_printf("C = %dC, RH = %d%%\r\n",TH_GetDataC_X10(), TH_GetDataRH_X1());
			uint8_t data[4] = {0};
			data[0] = C_X4T.num3*10 + C_X4T.num2;
			data[1] = C_X4T.num1*10 + C_X4T.num0;
			data[2] = RH_X4T.num3*10 + RH_X4T.num2;
			data[3] = RH_X4T.num1*10 + RH_X4T.num0;


			ee24.exchangeI2CPins();
			ee24.writeBytes(
				sizeof(systemStorageType) +
					(systemSto.data.NumDataCollected.uint_16 - 1)*sizeof(data),	//本次写入的地址偏移，注意乘以数据组大小
				data, 			//
				sizeof(data));	//每次写入的数据byte数
			//获取结构体成员偏移结构体地址多少byte
			ee24.writeBytes(0, systemSto.data.NumDataCollected.ctrl, 2);	//保存已采集个数
			ee24.recoverI2CPins();
		}
		else {
			systemSto.data.settingsBits[colBits].bits.bit7 = 0;	//任务开关:关闭
			__LimitValue(systemSto.data.NumDataCollected.uint_16, //限制数量
					0 , systemSto.data.NumDataWillCollect);
		}
	}
}

//执行过长的绘图程序中调用，例如绘制温湿度BITMAP
//风扇是中心对称，每转每1/4转到会自对称度需要5帧，转一圈20帧
void drawFan(bool markBackFromMenu){
	if(waitTime(&drawFanTimeOld, 10))
	{
		//绘制小风扇
		u8g2.setDrawColor(0);
		u8g2.drawXBM(49 - 1, 26 - 2, 16, 16, &icon16x16Fan[indexFanFrame][0]);
		indexFanFrame = (indexFanFrame + 1) % 5;	//0~4
		u8g2.setDrawColor(1);
		u8g2.drawBox(0, 23, 64, 2);	//分割线
		u8g2.setDrawColor(1);
		if(!markBackFromMenu)
			u8g2.sendBuffer();
	}
}
