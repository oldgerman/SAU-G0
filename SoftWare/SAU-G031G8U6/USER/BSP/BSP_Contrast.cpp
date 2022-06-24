/*
 * BSP_Screen.cpp
 *
 *  Created on: Jun 22, 2022
 *      Author: OldGerman
 */

#include "BSP.h"

#include "Colum.hpp"	//提供AutoValue
#include "oled_init.h"

#define oledContrastStepsMs 50			//oled每次亮度发生更改的步进时间
uint16_t screenBrightnessVal = 0;
AutoValue screenBrightness(&screenBrightnessVal, 3, 100, 0, 5, 5, false);
bool firstScreenBright = true; //亮屏标记


//映射0~100亮度到oled背光寄存器0~255
void Contrast_Set(uint16_t val) {
	u8g2.setContrast(map(*screenBrightness.val, 0, 100, 0, 255));
}
/**
 * 阻塞熄屏函数
 */
void Contrast_Darken() {
	for (;;) {
		static uint32_t timeOld = HAL_GetTick();
		if(waitTime(&timeOld, oledContrastStepsMs)) {
		screenBrightness--;
		Contrast_Set(*screenBrightness.val);
		u8g2.sendBuffer(); //相当于在发送调节背光命令
		if (*screenBrightness.val == screenBrightness.lower)
			break;
		}
	}
	u8g2.setPowerSave(1);
}

/**
 * 阻塞亮屏函数
 */
void Contrast_Brighten() {
	u8g2.setPowerSave(0);
	for (;;) {
		static uint32_t timeOld = HAL_GetTick();
		if(waitTime(&timeOld, oledContrastStepsMs)) {
			screenBrightness++;
			Contrast_Set(*screenBrightness.val);
			u8g2.sendBuffer();	//相当于在发送调节背光命令
			if (*screenBrightness.val == screenBrightness.upper)
				break;
		}
	}
}

void Contrast_Update(void (*FunPtr)(void)){
	//超时熄屏
	if (firstScreenBright
			&& (*screenBrightness.val == screenBrightness.upper 	//从上电或熄屏唤醒首次达到最大亮度
			|| *screenBrightness.val == screenBrightness.lower)){	//从熄屏最低亮度唤起
		firstScreenBright = false;
	}

	if (shouldBeSleeping()){
		static uint32_t timeOld = HAL_GetTick();
		if(waitTime(&timeOld, oledContrastStepsMs)) {
			screenBrightness--;
			Contrast_Set(*screenBrightness.val);
		}
		if (*screenBrightness.val == 0) {
			u8g2.setPowerSave(1);
			firstScreenBright = true;
			//关闭主MOS，只留VBatWeek电源轨
			HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_RESET);
			//进入STOP1
			FunPtr();
		} else{
			u8g2.setPowerSave(0);
		}
	} else {
			static uint32_t timeOld = HAL_GetTick();
			if(waitTime(&timeOld, oledContrastStepsMs)) {
				screenBrightness++;
				Contrast_Set(*screenBrightness.val);
				u8g2.setPowerSave(0);
			}
	}
}
