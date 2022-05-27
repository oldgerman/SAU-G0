/*
 * CustomPageFuc.cpp
 *
 *  Created on: May 25, 2022
 *      Author: OldGerman
 */
#include "CustomPage.hpp"


const char *strChooseOneFromTwo = 	  "OK  CXL";	//Cancel的英文简称是CXL
const char *strfeaturesNotRealized = "暂未实现!";
/*
 * Colum对象位数组指定位修改API
 */
void coulmBinCodeAdj() {
	colum_FeaturesUnrealized();
}

void columsAccessibility_Battery(){
	colum_FeaturesUnrealized();
}

void columsDateTime_ChangeDateTime(){
	colum_FeaturesUnrealized();
}


void columsDataCollected_Schedule(){
	colum_FeaturesUnrealized();
}
/**
 * 用于某些colums二选一
 * 默认打印当前选中的colum标题字符串，也可指定字符串
 */
bool colums_StrChooseOneFromTwo(bool featuresRealized, const char *str) {
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
	u8g2.clearBuffer();
	u8g2.sendBuffer();	//立即发送空buffer,消除残影和乱码
	uint8_t y = 0;
	if (featuresRealized)	//若功能实现
	{
		if(!str)
		{
			const Colum *ptrColum = Page::ptrPage->getColumsSelected();
			//if(ptrColum->str != nullptr)
			u8g2.drawUTF8(1, y, ptrColum->str);	//打印中文字符，编译器需要支持UTF-8编码，显示的字符串需要存为UTF-8编码
		}
		else {
			u8g2.drawUTF8(1, y, str);	//打印中文字符，编译器需要支持UTF-8编码，显示的字符串需要存为UTF-8编码
		}
	} else {					//若未实现
		u8g2.drawUTF8(1, y, strfeaturesNotRealized);
	}
	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
	y += 16;
	u8g2.drawUTF8(1, y, strChooseOneFromTwo);
	u8g2.sendBuffer();

	return waitingToChooseOneFromTwo();
}

/*打印未实现函数指针功能colums*/
void colum_FeaturesUnrealized() {
	colums_StrChooseOneFromTwo(false);
}
//不做任何动作
void colum_FuncNull() {
	;
}

void columsScreenSettings_Brightness() {
//	screenBrightness.upper = systemSettings.ScreenBrightness;
//	*screenBrightness.val = screenBrightness.upper;
//	setContrast(*screenBrightness.val);
}


void columsAccessibility_ResetSettings() {
	//向FLashEEPROM写入APP跳转回DFU的标志位，该情况bootloader不打印logo，无需按键直接进入DFU模式
	/* .... */
	bool ok = colums_StrChooseOneFromTwo(true);
	if (ok) {
		//shutScreen();
		resetSettings();
		saveSettings();
	}
}

void columsAccessibility_I2CScaner() {
	u8g2.clearBuffer();
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese);	//12x12 pixels
	uint8_t y = 0;

	const Colum *ptrColum = Page::ptrPage->getColumsSelected();
	u8g2.drawUTF8(1, y, ptrColum->str);	//打印中文字符，编译器需要支持UTF-8编码，显示的字符串需要存为UTF-8编码

#ifdef STM32F1
	I2C_HandleTypeDef* hi2cPtr = &hi2c2;
#elif defined(STM32F4)
	I2C_HandleTypeDef *hi2cPtr = &hi2c3;
#else
#endif
	uint8_t i = 0;
	HAL_StatusTypeDef status = HAL_OK;
	int8_t ok = -1;
	char buffUSB[5] = { 0 };
	bool exchangeSate = false;	//标记: 扫描/清除
	bool firstIn = true; 		//标记: 第一次进入
	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
	uint8_t i2cDevices = 0;

	for (;;) {
		buttons = getButtonState();
		switch (buttons) {
		case BUTTON_A_SHORT:
			ok = 1;
			break;
		case BUTTON_B_SHORT:
			ok = 0;
			break;
		default:
			break;
		}
		//每1s 扫描/清除 切换一次

		uint32_t previousState = HAL_GetTick();
		static uint32_t previousStateChange = HAL_GetTick();
		if ((previousState - previousStateChange) > 1500 || firstIn) {
			firstIn = false;	//修改首次进入标记
			previousStateChange = previousState;
			exchangeSate = !exchangeSate;
			u8g2.setDrawColor(0);
			u8g2.drawBox(0, 15, 128, 16);
			u8g2.setDrawColor(1);
			if (exchangeSate)
				u8g2.drawStr(0, 15, "Scaning...");
			else {
				uint8_t xAddr = 0;
				for (i = 0; i < 127; i++) {
//					status = HAL_I2C_Master_Transmit(hi2cPtr, i << 1, 0, 0,200);

					if (status == HAL_OK && i != 0) {
						++i2cDevices;	//设备计数+1
						sprintf(buffUSB, "0x%02X", i);
						u8g2.drawStr(xAddr, 15, buffUSB);
						xAddr += 40;	//最多三个地址
					}
					//HAL_Delay(10);	//会造成 扫描/清除 延迟不对等
					//HAL_Delay(10);	//会死机
				}
				memset(buffUSB, 0, 5);
				if (!i2cDevices)	//未扫描到设备
					u8g2.drawStr(0, 15, "Nothing!");
			}
		}
		if (ok != -1)
			break;
		u8g2.sendBuffer();
		HAL_Delay(MENU_DELAY);
	}
}


//重启
void columsHome_Reset()
{
	bool ok = colums_StrChooseOneFromTwo(true);
	if(ok)
	{
//		shutScreen();

		//在这个时间内按下中键不放可以进入DFU
		uint16_t timeWaitingStartPage = HAL_GetTick();
		for (;;) {
			if (HAL_GetTick() - timeWaitingStartPage > 666)
				break;
//			resetWatchdog();
			HAL_Delay(MENU_DELAY);
		}

		NVIC_SystemReset();
	}
}



void columsHome_ShowVerInfo() {
//	drawLogoAndVersion('A');
	u8g2.sendBuffer();

	bool getBack = false;
	bool state = waitingToChooseOneFromTwo();
	if (state == 0)
		getBack = true;
	if (!getBack) {
		//第二页
		u8g2.clearBuffer();
		for (int i = 0; i < 4; i++) {
			HAL_Delay(10);
//			u8g2.drawStr(0, i * 8, DebugMenu[i]);
		}
		u8g2.sendBuffer();
		waitingToChooseOneFromTwo();
	}
}
