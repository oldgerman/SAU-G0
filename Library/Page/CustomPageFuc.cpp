/*
 * CustomPageFuc.cpp
 *
 *  Created on: May 25, 2022
 *      Author: OldGerman (过气德国佬)
 */
#include "CustomPage.hpp"
#include "cppports.h"

const char *strChooseOneFromTwo 	= "OK   CXL";	//Cancel的英文简称是CXL
const char *strfeaturesNotRealized = "暂未实现";
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
	u8g2.clearBuffer();
	u8g2.sendBuffer();
	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
	uint8_t y = 13;
	u8g2.drawUTF8(1, y, "已采");
	u8g2.drawUTF8(1, y + 16, "待采");

	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
//	u8g2.dra
	uint8_t numXOffset = 10;
	uint8_t places = 4;			//数据最大位数
	drawNumber((OLED_WIDTH - numXOffset) - places * 6, y,
			systemSto.data.NumDataCollected, places);
	drawNumber((OLED_WIDTH - numXOffset) - places * 6, y + 16,
			systemSto.data.NumDataWillCollect, places);
	u8g2.drawUTF8(1, y + 32, strChooseOneFromTwo);

	u8g2.sendBuffer();
	waitingToChooseOneFromTwo();
}

void columsDataCollect_ScheduleSetting_NumDataOneDay(){
	if(Sec24H % systemSto.data.NumDataOneDay != 0) {
		switch (buttons) {
		case BUTTON_A_LONG:
		case BUTTON_A_SHORT:
			systemSto.data.NumDataOneDay--;
			break;
		case BUTTON_B_LONG:
		case BUTTON_B_SHORT:
			systemSto.data.NumDataOneDay++;
			break;
		default:
			break;
		}
	}
}

void columsDataCollect_ScheduleSetting_NumDataWillCollect(){
	//每个采集对象数据占用2byte，因此将剩余EEPROM空间先除以2，
	//再除以采集对象数量，得到可存储的采集数据组的最大个数，其作为值约束
	uint32_t maxNumDataSave = getEEPROMFreeSize() / 2 /
			(
				systemSto.data.settingsBits[colBits].bits.bit1 +
				systemSto.data.settingsBits[colBits].bits.bit2 +
				systemSto.data.settingsBits[colBits].bits.bit3 +
				systemSto.data.settingsBits[colBits].bits.bit4 +
				systemSto.data.settingsBits[colBits].bits.bit5 +
				systemSto.data.settingsBits[colBits].bits.bit6
			);
	if(systemSto.data.NumDataWillCollect > maxNumDataSave)
		systemSto.data.NumDataWillCollect = maxNumDataSave;

	/*
	 * 使用24C02（256byte）进行测试：
	 * 当只采集温度和湿度时，对象数量为2，那么192/2/2 = 48
	 * 串口打印：
	 * 	maxNumDataSave = 48
	 */
#if 0
	DBG_PAGE_PRINT("maxNumDataSave = %d\r\n",maxNumDataSave);
#endif
}

uint32_t getEEPROMFreeSize(){
#if 0
	/*
	 * 使用24C02（256byte）进行测试：
	 * 串口打印：
	 * a = 256, b = 64, c = 192
	 */
	 uint32_t a = ee24.getMemSizeInByte();
	 uint32_t b =  sizeof(systemStorageType);
	 uint32_t c = a - b;
	 DBG_PAGE_PRINT("a = %d, b = %d, c = %d\r\n",a,b,c);
	 return c;
#else
	 return ee24.getMemSizeInByte() - sizeof(systemStorageType);
#endif
}

void columsDrawDateTime(DateTime *dt)
{
	uint8_t y = 13;
	char buf[15] = {0};
	sprintf(buf, "%02d/%02d/%02d", dt->year()-2000, dt->month(), dt->day());

	u8g2.setDrawColor(1);
	u8g2.clearBuffer();
	u8g2.sendBuffer();
	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels

	u8g2.drawUTF8(1, y, buf);
	memset(buf, 0, strlen(buf));
	sprintf(buf, "%02d:%02d:%02d", dt->hour(),dt->minute(), dt->second());
	y += 16;
	u8g2.drawUTF8(1, y, buf);
	y += 16;
	u8g2.drawUTF8(1, y, strChooseOneFromTwo);
	u8g2.sendBuffer();
}
void columsDataCollect_ScheduleSetting_STDateTime(){
	DateTime dt(
			systemSto.data.STyy + 2000,
			systemSto.data.STMM,
			systemSto.data.STdd,
			systemSto.data.SThh,
			systemSto.data.STmm,
			systemSto.data.STss);
	columsDrawDateTime(&dt);
	waitingToChooseOneFromTwo();
}
void columsDataCollect_ScheduleSetting_EDDateTime(){
	/*
	 * Debug: 396B/264B(有print和无print)
	 * 设置的开始日期为2022/06/25 08:12:13
	 * 每天
	 * 串口打印:
	 * 	systemSto.data.SThh = 11
	 * 	dt1 = 2022/6/25 (Saturday) 11:12:13
	 * 	dt2 = 2022/6/27 (Saturday) 11:12:13
	 */
//	DBG_PAGE_PRINT("systemSto.data.SThh = %d\r\n", systemSto.data.SThh);
	DateTime dt(
			systemSto.data.STyy + 2000,
			systemSto.data.STMM,
			systemSto.data.STdd,
			systemSto.data.SThh,
			systemSto.data.STmm,
			systemSto.data.STss);
//	DBG_PAGE_PRINT("dt1 = %d/%d/%d (%s) %d:%d:%d\r\n",
//			dt.year(),
//			dt.month(),
//			dt.day(),
//			daysOfTheWeek[dt.dayOfTheWeek()],
//			dt.hour(),
//			dt.minute(),
//			dt.second()
//	);

	TimeSpan timeSpan(
			(Sec24H / systemSto.data.NumDataOneDay) * 	//得到任务周期，单位：秒
			systemSto.data.NumDataWillCollect);			//乘以将采集数据组个数，得到总任务时间戳
	/**
	 * 笔记：C语言中指针的*号和乘法的*号，怎么区分？
	 * 不用区分，出现指针*的地方不会出现乘号*，反之亦然
	 * 指针*是一元操作符，乘法*是二元操作符
	 */
	dt = dt + timeSpan;

//	DBG_PAGE_PRINT("dt2 = %d/%d/%d (%s) %d:%d:%d\r\n",
//			dt.year(),
//			dt.month(),
//			dt.day(),
//			daysOfTheWeek[dt.dayOfTheWeek()],
//			dt.hour(),
//			dt.minute(),
//			dt.second()
//	);
	columsDrawDateTime(&dt);
	waitingToChooseOneFromTwo();
}
/**
 * 用于某些colums二选一
 * 默认打印当前选中的colum标题字符串，也可指定字符串
 * @param featuresRealized	功能是否实现 false:打印strfeaturesNotRealized字符串，true:打印指定的字符串
 * @param str nullptr: 打印当前Colum的字符串 不是nullptr:打印自定义字符串
 * @param OK_CXL_OffSetNumColum OK和CXLy方向继续向下偏移colum个数,默认为0
 */
bool colums_StrChooseOneFromTwo(bool featuresRealized, const char *str, uint8_t OK_CXL_OffSetNumColum) {
	u8g2.setDrawColor(1);
	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
	u8g2.clearBuffer();
	u8g2.sendBuffer();	//立即发送空buffer,消除残影和乱码
	uint8_t y = 13; 	//偏移字符串y坐标
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
	y += (16 + 16 * OK_CXL_OffSetNumColum);
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
//	screenBrightness.upper = systemSto.data.ScreenBrightness;
//	*screenBrightness.val = screenBrightness.upper;
//	setContrast(*screenBrightness.val);
}


void columsAccessibility_ResetSettings() {
	//向FLashEEPROM写入APP跳转回DFU的标志位，该情况bootloader不打印logo，无需按键直接进入DFU模式
	/* .... */
	bool ok = colums_StrChooseOneFromTwo(true);
	if (ok) {
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

