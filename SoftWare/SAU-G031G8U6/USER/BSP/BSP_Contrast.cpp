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
uint16_t screenBrightnessVal;
AutoValue screenBrightness(&screenBrightnessVal, 3, 100, 0, 5, 5, false);
bool firstScreenBright = true; //亮屏标记


//映射0~100亮度到oled背光寄存器0~255
void Contrast_SetVal() {
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
		Contrast_SetVal();
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
			Contrast_SetVal();
			u8g2.sendBuffer();	//相当于在发送调节背光命令
			if (*screenBrightness.val == screenBrightness.upper)
				break;
		}
	}
}

/**
  * @brief  初始化对比度（即OLED亮度），需要事先载入EEPROM数据以获取亮度
  * @param  None
  * @retval None
  */
void Contrast_Init(){
	//从Flash载入屏幕亮度为screenBrightness的最大值
	//woc,那这里德国烙铁写错了
	screenBrightnessVal = 0;
	screenBrightness.upper = systemSto.data.ScreenBrightness;
	Contrast_SetVal();	//这个时候*val还是0
}

/**
  * @brief  菜单里调节屏幕亮度调用的函数
  * @param  None
  * @retval None
  */
void Contrast_SetUpperAndVal(){
	screenBrightness.upper = systemSto.data.ScreenBrightness;
	*screenBrightness.val = screenBrightness.upper;
	Contrast_SetVal();
}
/**
  * @brief  更新对比度（即OLED亮度）
  * @param  FunPtr 对比度为0时执行的函数
  * @retval None
  */
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
			Contrast_SetVal();
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
				Contrast_SetVal();
				u8g2.setPowerSave(0);
			}
	}
}
