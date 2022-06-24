/*
 * CustomPageFuc.cpp
 *
 *  Created on: May 25, 2022
 *      Author: OldGerman (过气德国佬)
 */
#include "CustomPage.hpp"
#include "cppports.h"
//	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
//const char *str1 	= "OK   CXL";	//Cancel的英文简称是CXL
const char *strChooseOK 	= "确认";
const char *strChooseCXL 	= "取消";
const char *strfeaturesNotRealized = "暂未实现！";
//按照SCL SDA先后
enum I2CLines {
	I2C2_PA11_PA12 = 0,
	I2C1_PB8_PB7,
	I2C1_EX_PB6_PB7
};
//i2c扫描的线路配置字符串
const char *strI2cInfo[] = {
		"I2C2:",
		"I2C1:",
		"I2C1 EX:"
};
//版本信息第二页字符串
const char *strVerInfo[] = {
	"SAU-G031",
	"<A>v1.0", // Print version number
	__DATE__,
	__TIME__,	// 编译日期
	"github.com/",
	"oldgerman",
};
void columsAccessibility_RunTime(){
	colum_FeaturesUnrealized();
}

/*
 * 计算一个数的n次方
 * 注意结果不要溢出32位
 * @param base 		基数
 * @param exponexn 	指数
 */
uint32_t numNthPower(uint8_t base, uint8_t exponent){
    uint32_t result = 1;
    while (exponent != 0)
    {
        result *= base;
        --exponent;
    }
    return result;
}
/*
 * 拆分num的每个位到numX4Type类型的各个元素
 * num十进制下最多4位数
 * @example
 * 	num    decimalPlaces	num0 num1 num2 num3
 * 	23      3               3    2    0    0
 * 	456		2				6	 5    4    0
 * 	1256    2 				6    5    4    1
 *
 */
numX4Type numSplit(uint16_t num){
	numX4Type numx4T;
	memset(&numx4T, 0, sizeof(numX4Type));
	uint8_t *ptr = &numx4T.num3;
	for(int i = 0; i < 4; i ++){
		uint16_t thPower10 = numNthPower(10, 3 - i);
		*(ptr + i) = num / thPower10;
		num = num % thPower10;
	}
	return numx4T;
}

// max size = 127
template<class T, uint8_t SIZE>
struct history {
	static const uint8_t size = SIZE;	//static const 类成员，为所有类的实例共享的数据，必须在类内对其初始化
	T buf[size];
	int32_t sum;	//T类型值的总和
	uint8_t loc;	//元素编号（location）

	void update(T const val) {
		// step backwards so i+1 is the previous value.
		//向后退一步，因此i + 1是前一个值。

		sum -= buf[loc];
		sum += val;
		buf[loc] = val;
		loc = (loc + 1) % size;	//loc0+8次1才会使loc=1？
	}

	//下标运算符 [] 重载
	T operator[](uint8_t i) const {
		// 0 = newest, size-1 = oldest.
		i = (i + loc) % size;
		return buf[i];
	}

	T average() const {
		return sum / size;
	}
};

#define batVoltage100 415
#define batVoltage0 280
 void columsAccessibility_Battery(){
	int8_t ok = -1;
	uint8_t y = 13;
	uint8_t x = 1;
	uint8_t yBitmap = 4;
	uint8_t xBitmap = 4;
	history<uint16_t, 20> adcFilter= {{0}, 0, 0};
	uint32_t timeOld = HAL_GetTick();
	uint8_t usage = 0;
	bool markBackFromOtherUI = true;
	ButtonState oldButtons = buttons;
	for (;;) {
		Power_AutoShutdownUpdate();
		/*读取电池电压*/
		adcFilter.update(ADC_Get());
		uint32_t adcVal = adcFilter.average();
		uint16_t battVoltageX100 =  adcVal * 330 * 2 >> 16;
		if(markBackFromOtherUI) {
			if(buttons != oldButtons)
				markBackFromOtherUI = false;
		}
		buttons = getButtonState();
		if(!markBackFromOtherUI){
			switch (buttons) {
			case BUTTON_A_SHORT:
			case BUTTON_B_SHORT:
			case BUTTON_OK_SHORT:
				ok = 0;
			default:
				break;
			}
		}
		//每500ms更新一次显示
		if(waitTime(&timeOld, 500)) {
			u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
			u8g2.clearBuffer();
			u8g2.drawBitmap(xBitmap, yBitmap, iconBattery_CNT, iconBattery_H,
					iconBattery24x8);

			u8g2.drawUTF8(x, y + 16, "电量");
	//		u8g2.drawUTF8(x, y + 32, "容量");
			u8g2.setFont(u8g2_font_unifont_tr);
			u8g2.drawUTF8(x-2, y + 32, "ADC");
			char buf[10] = {0};
			sprintf(buf, "%01d.%02dV", battVoltageX100 / 100, battVoltageX100 % 100);
			u8g2.drawUTF8(x + 23, y, buf);
			memset(buf, 0, strlen(buf));
			//上面显示实际电压不限制到有效范围，这里计算百分比需要限制以防止溢出
			__LimitValue(battVoltageX100, batVoltage0, batVoltage100);
			sprintf(buf, "%3d%%", ((battVoltageX100 - batVoltage0) * 100) / (batVoltage100 - batVoltage0));
			u8g2.drawUTF8(x + 31, y + 16, buf);
			memset(buf, 0, strlen(buf));
			sprintf(buf, "%d", (uint16_t)adcVal);
			u8g2.drawUTF8(x + 24, y + 32, buf);
			/*是否充电*/
			bool Is_BattCharging = HAL_GPIO_ReadPin(CHARGE_DETECT_GPIO_Port, CHARGE_DETECT_Pin);
			/*电压映射到遮挡矩形的宽度*/
			uint8_t symIndex = map(battVoltageX100, batVoltage0, batVoltage100, 4 ,0);
			if(Is_BattCharging)
			{
				usage++;
				usage %= (symIndex + 1);//至少一个电量格
				symIndex -= usage;		//翻转值
			}
			else{
				usage = 0;
			}
			u8g2.setDrawColor(0);
			u8g2.drawBox(xBitmap + 4, yBitmap + 2, symIndex * 3, 4);
			u8g2.setDrawColor(1);
			u8g2.sendBuffer();
			if(ok != -1)
				break;
		}
	}
}

	/* 第二页
	 * 显示方式1
	 * 累计时间,需要预留5位数 0~99999s(0~27.8h)
	 * 使用u8g2.setFont(u8g2_font_IPAandRUSLCD_tr); //7pixel字体;显示下面三行，耗费24像素，然后中文字12pixel
	 * 12345678901	//一行最多11个字
	 * RUN   437s	//数字右对齐
	 * LPW 99999s	//99999秒 约27.8h
	 * SP1 99999m	//99999分 约69.4天
	 */
	/*
	 * 显示方式2
	 * 12345678901	//一行最多11个字
	 * <RUN>
	 * 27h 48m 53s<--实时更新
	 * <LPW_RUN>
	 * 27h 53m 12s
	 * <STOP1> 60d<--这里显示下面那行写不下的天数
	 * 08h 07m 06s
	 */

void columsDataCollect_Export(){
	if(systemSto.data.NumDataCollected.uint_16 != 0)
	{
		if(colums_StrSelect(true, SEL_2, "通过串口", 1, "导出数据？")) {
			uint32_t timeCounter = HAL_GetTick();
			u8g2.clearBuffer();
//			u8g2.sendBuffer();
			u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
			uint8_t y = 13;
			uint8_t x = 1;
			u8g2.drawUTF8(x, y, "已导");
			u8g2.drawUTF8(x, y + 16, "待导");
			u8g2.drawUTF8(x, y + 32, "正在导出");
			u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels

			uint8_t numXOffset = 10;
			uint8_t places = 4;			//数据最大位数
			usb_printf("| Date & Time     | T(C)  | H(%%)  |\r\n");	//格式化打印%需要输入两个%
			usb_printf("| --------------- | ----- | ----- |\r\n");

			DateTime dt(getEEPROMData_DateTime(&systemSto.data.dtStartCollect));

			ee24.exchangeI2CPins();
			for(uint16_t i = 0; i <= systemSto.data.NumDataCollected.uint_16; i++) {
				if(i <  systemSto.data.NumDataCollected.uint_16) {
					uint8_t data[4] = {0};
					ee24.readBytes(sizeof(systemStorageType) + i * sizeof(data), data, sizeof(data));
//	每行格式：			| 2022/5/10 8:00  | 18.72 | 65.83 |\r\n
					TimeSpan timeSpan(
							(Sec24H / systemSto.data.NumDataOneDay) * 	//得到任务周期，单位：秒
							i);											//得到每次步进
					DateTime now = dt + timeSpan;	//虽然有operator+，但不能写dt = dt + timeSpan;
					//导出为EXCEL真日期，不需要加前导0
					usb_printf("| %d/%d/%d %d:%d:%d | %d.%d | %d.%d |\r\n",
							now.year(),
							now.month(),
							now.day(),
							now.hour(),
							now.minute(),
							now.second(),
							data[0],data[1],
							data[2],data[3]
					);
				}
				//每10次才更新一下显示计数
				if((i % 10 == 0) || i == systemSto.data.NumDataCollected.uint_16) {
					u8g2.setDrawColor(0);
					u8g2.drawBox(2+24, 0, 64, 32);
					u8g2.setDrawColor(1);
					drawNumber((OLED_WIDTH - numXOffset) - places * 6, y,
							i, places);
					drawNumber((OLED_WIDTH - numXOffset) - places * 6, y + 16,
							systemSto.data.NumDataCollected.uint_16 - i, places);
					u8g2.sendBuffer();
				}
			}
			ee24.recoverI2CPins();
			timeCounter = HAL_GetTick()- timeCounter;
			u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
			u8g2.drawUTF8(x, y + 32, "导出完成！");
			u8g2.sendBuffer();
			waitingSelect(SEL_3);
			//待实现打印用时时间
			char buf[10] = {0};
			sprintf(buf, "%ldms", timeCounter);
			colums_StrSelect(true, SEL_3, "导出用时：", 1, buf, false);	//中文不能打unifont数字怎么办？
		}
	}else
		colums_StrSelect(true, SEL_3, "没有数据！", 1);
}

void columsDateTime_ChangeDateTime(){
	DateTime dt = getEEPROMData_DateTime(&dtSys);
	const Colum *ptrColum = Page::ptrPage->getColumsSelected();
	columsDrawDateTime(&dt, ptrColum->str);
	waitingSelect(SEL_3);
	bool CheckNumRange = RTC_CheckUintDateTime(&dtSys);
	if(CheckNumRange){
		if(colums_StrSelect(true, SEL_2, "时间有效！", 1, "更改时间？")) {
			rtc.adjust(dt);
		}
	}
	else{
		colums_StrSelect(true, SEL_3, "时间无效！", 1);
	}
}


void columsDataCollected_Schedule(){
	u8g2.clearBuffer();
	u8g2.sendBuffer();
	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
	uint8_t y = 13;
	const Colum *ptrColum = Page::ptrPage->getColumsSelected();
	u8g2.drawUTF8(1, y, ptrColum->str);

	u8g2.drawUTF8(1, y + 16, "已采");
	u8g2.drawUTF8(1, y + 32, "待采");

	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
	uint8_t numXOffset = 10;
	uint8_t places = 4;			//数据最大位数
	drawNumber((OLED_WIDTH - numXOffset) - places * 6, y + 16,
			systemSto.data.NumDataCollected.uint_16, places);
	drawNumber((OLED_WIDTH - numXOffset) - places * 6, y + 32,
			systemSto.data.NumDataWillCollect, places);

	u8g2.sendBuffer();
	waitingSelect(SEL_3);
}

//获取以uint16_t类型存储在EEPROM中的时间对象
//必须以年、月、日、时、分秒顺序排列
//DateTime实现了 DateTime(const DateTime &copy)
//可以返回对象副本作为新实例构造函数的参数
DateTime getEEPROMData_DateTime(uintDateTime* ptr){
	 DateTime dt(
			 ptr->yOff + 2000,
			 ptr->m,
			 ptr->d,
			 ptr->hh,
			 ptr->mm,
			 ptr->ss);
	return dt;
}

void columsDataCollect_Switch(){
	rtc.setIntAlarm(RESET);	//关闭Alarm中断
	rtc.clearFlagAlarm();	//清除Alarm Flag
	intFromRTC = RESET;		//复位EXT中断回调函数的标记类
	/*
	 *不需要强制对齐开始时间到等分24小时的采集时间点上，因为再过24小时就是开始时间点+24小时
	 */
	/*需要根据设置的每天次数--即算出采集周期的有效值
	 * 有三种情况，需要选择对应的触发模式来设置闹钟：
	 * 1. 小时、分、秒 	都存在非0值 	PCF212x_A_Hour
	 * 2. 分、秒 		都存在非0值		PCF212x_A_Minute
	 * 3. 秒			都存在非0值		PCF212x_A_Second
	 * 不需要整这么麻烦，统一用PCF212x_A_Hour模式触发闹钟就好
	 * 将周期换算为TimeSpan，乘以已采集的数量，加上开始啊时间的结果，来步进闹钟下次的设置时间
	 */
	DateTime dt = getScheduleSetting_NextDateTime();
	rtc.setTimeAlarm(&dt, PCF212x_A_Hour);	//设置闹钟时间
	rtc.setIntAlarm(SET);	///打开Alarm中断
}


void columsDataCollect_ScheduleDelete(){
	colums_StrSelect(true, SEL_2, nullptr, 1);
	resetDataCollectSettings();
//	saveSettings();	这里不保存，等退出菜单一起保存
}

//清除记录
void columsDataCollect_CollectedDelete(){
	if(colums_StrSelect(true, SEL_2, nullptr, 1)){
		ee24.exchangeI2CPins();
//		不需要擦除，只需要清零采集数量就读不到了
//		ee24.eraseChip(sizeof(systemStorageType));		//从systemStorageType之后的地址开始擦除剩余所有空间
		systemSto.data.NumDataCollected.uint_16 = 0;	//清零已采集数量

		ee24.writeBytes(0, systemSto.data.NumDataCollected.ctrl, 2);
		ee24.recoverI2CPins();
		systemSto.data.settingsBits[colBits].bits.bit7 = 0;	//任务开关:关闭
	}
}

//值约束NumDataOneDay
void columsDataCollect_ScheduleSetting_NumDataOneDay(){
	switch (buttons) {
	case BUTTON_A_LONG:
	case BUTTON_A_SHORT:
		while((Sec24H % systemSto.data.NumDataOneDay) != 0) {
			systemSto.data.NumDataOneDay--;
		}
		break;
	case BUTTON_B_LONG:
	case BUTTON_B_SHORT:
		while((Sec24H % systemSto.data.NumDataOneDay) != 0) {
			systemSto.data.NumDataOneDay++;
		}
		break;
	default:
		break;
	}
}
//值约束NumDataWillCollect
void columsDataCollect_ScheduleSetting_NumDataWillCollect(){
	//每个采集对象数据占用2byte，因此将剩余EEPROM空间先除以2，
	//再除以采集对象数量，得到可存储的采集数据组的最大个数，其作为值约束
	uint32_t maxNumDataSave = getEEPROM_FreeSize() / 2 /
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
	DBG_PAGE_PRINT("maxNumDataSave = %d\r\n",maxNumDataSave);
}

//返回以btye为单位的EEPROM剩余大小
uint32_t getEEPROM_FreeSize(){
#if DBG_PAGE
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
	 ee24.exchangeI2CPins();
	 uint32_t FreeSize = ee24.getMemSizeInByte() - sizeof(systemStorageType);
	 ee24.recoverI2CPins();
	 return FreeSize;
#endif
}

void columsDrawDateTime(DateTime *dt, const char* str)
{
	uint8_t y = 13;
	char buf[15] = {0};
	sprintf(buf, "%02d/%02d/%02d", dt->year()-2000, dt->month(), dt->day());

	u8g2.setDrawColor(1);
	u8g2.clearBuffer();
	u8g2.sendBuffer();

	u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
	if(str) u8g2.drawUTF8(1, y, str);
	y += 16;

	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
	u8g2.drawUTF8(1, y, buf);
	memset(buf, 0, strlen(buf));
	sprintf(buf, "%02d:%02d:%02d", dt->hour(),dt->minute(), dt->second());
	y += 16;
	u8g2.drawUTF8(1, y, buf);

	u8g2.sendBuffer();
}
void columsDataCollect_ScheduleSetting_StartDateTime(){
	DateTime dt(getEEPROMData_DateTime(&systemSto.data.dtStartCollect));
	columsDrawDateTime(&dt, "开始时间");
	waitingSelect(SEL_3);
}
void columsDataCollect_ScheduleSetting_NextDateTime(){
	DateTime dt = getScheduleSetting_NextDateTime();
	const Colum *ptrColum = Page::ptrPage->getColumsSelected();
	columsDrawDateTime(&dt, ptrColum->str);
	waitingSelect(SEL_3);
}

//获取下次任务时间
DateTime getScheduleSetting_NextDateTime()
{
	DateTime alarmDateTime(getEEPROMData_DateTime(&systemSto.data.dtStartCollect));
	TimeSpan timeSpan(
			(Sec24H / systemSto.data.NumDataOneDay) * 	//得到任务周期，单位：秒
			systemSto.data.NumDataCollected.uint_16);	//乘以已采集数据组个数，得到下次任务的时间戳
	alarmDateTime = alarmDateTime + timeSpan;			//开始时间加上时间戳，得到下次任务的闹钟时间
	return alarmDateTime;
}

void columsDataCollect_ScheduleSetting_EndDateTime(){
	/*
	 * Debug: 396B/264B(有print和无print)
	 * 设置的开始日期为2022/06/25 08:12:13
	 * 每天
	 * 串口打印:
	 * 	systemSto.data.SThh = 11
	 * 	dt1 = 2022/6/25 (Saturday) 11:12:13
	 * 	dt2 = 2022/6/27 (Saturday) 11:12:13
	 */
	DBG_PAGE_PRINT("systemSto.data.SThh = %d\r\n", systemSto.data.SThh);
	DateTime dt(getEEPROMData_DateTime(&systemSto.data.dtStartCollect));
	DBG_PAGE_PRINT("dt1 = %d/%d/%d (%s) %d:%d:%d\r\n",
			dt.year(),
			dt.month(),
			dt.day(),
			daysOfTheWeek[dt.dayOfTheWeek()],
			dt.hour(),
			dt.minute(),
			dt.second()
	);

	TimeSpan timeSpan(
			(Sec24H / systemSto.data.NumDataOneDay) * 	//得到任务周期，单位：秒
			systemSto.data.NumDataWillCollect);			//乘以将采集数据组个数，得到总任务时间戳
	/**
	 * 笔记：C语言中指针的*号和乘法的*号，怎么区分？
	 * 不用区分，出现指针*的地方不会出现乘号*，反之亦然
	 * 指针*是一元操作符，乘法*是二元操作符
	 */
	dt = dt + timeSpan;

	DBG_PAGE_PRINT("dt2 = %d/%d/%d (%s) %d:%d:%d\r\n",
			dt.year(),
			dt.month(),
			dt.day(),
			daysOfTheWeek[dt.dayOfTheWeek()],
			dt.hour(),
			dt.minute(),
			dt.second()
	);
	const Colum *ptrColum = Page::ptrPage->getColumsSelected();
	columsDrawDateTime(&dt, ptrColum->str);
	waitingSelect(SEL_3);
}
/**
 * 用于某些colums二选一
 * 默认打印当前选中的colum标题字符串，也可指定字符串
 * @param featuresRealized	功能是否实现 false:打印strfeaturesNotRealized字符串，true:打印指定的字符串
 * @param str nullptr: 打印当前Colum的字符串 不是nullptr:打印自定义字符串
 * @param OK_CXL_OffSetNumColum OK和CXLy方向继续向下偏移colum个数,默认为0
 */
bool colums_StrSelect(bool featuresRealized, SelectState sel, const char *str,
		uint8_t OK_CXL_OffSetNumColum, const char *str2, bool str2Chinese) {
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
			if(str2) {
				if(!str2Chinese)
					u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
				u8g2.drawUTF8(1, y + 16, str2);
			}
		}
	} else {					//若未实现
		u8g2.drawUTF8(1, y, strfeaturesNotRealized);
	}
	u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
	y += (16 + 16 * OK_CXL_OffSetNumColum);
	if(sel == SEL_2) {
		u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese); //12x12 pixels
		u8g2.drawUTF8(1, y, strChooseOK);
		u8g2.drawUTF8(1 + 12 * 2 + 12, y, strChooseCXL);
	}
	u8g2.sendBuffer();

	return waitingSelect(sel);
}

/*打印未实现函数指针功能colums*/
void colum_FeaturesUnrealized() {
	colums_StrSelect(false, SEL_3, nullptr, 1);
}
//不做任何动作
void colum_FuncNull() {
	;
}


/**
 * 屏幕设置
 * Note：
 *  亮度为1仍有亮度，为0则熄屏
 */
void columsScreenSettings_Brightness() {
	Contrast_SetUpperAndVal();
}


void columsScreenOffAndWKUP_Sensitivity(){
	IMU_SetThreshold();
}

void columsAccessibility_ResetSettings() {
	//向FLashEEPROM写入APP跳转回DFU的标志位，该情况bootloader不打印logo，无需按键直接进入DFU模式
	/* .... */
	bool ok = colums_StrSelect(true, SEL_2, nullptr, 1);
	if (ok) {
		resetSettings();
		saveSettings();
	}
}

void columsAccessibility_I2CScaner() {
	uint8_t i = 0;
	int8_t ok = -1;
	uint8_t i2cDevices = 0;
	char buffUSB[5] = { 0 };
	HAL_StatusTypeDef status = HAL_OK;
	uint8_t i2cLine = I2C2_PA11_PA12;
	I2C_HandleTypeDef *hi2c = nullptr;
	uint8_t y = 13;	//y坐标
	if(colums_StrSelect(true, SEL_2, nullptr, 1)){
		u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
		for (;;) {
			buttons = getButtonState();
			switch (buttons) {
			case BUTTON_A_SHORT:
			case BUTTON_B_SHORT:
			case BUTTON_OK_SHORT:
				ok = 0;
			default:
				break;
			}


			//每1.5s 扫描/清除 切换一次
			static uint32_t timeOld = HAL_GetTick();
			uint8_t stri2cOffset = 0;
			if(waitTime(&timeOld, 1500))
			{

				u8g2.clearBuffer();
				switch (i2cLine) {
					case I2C2_PA11_PA12:
						hi2c = &hi2c2;
						stri2cOffset = 0;
						break;
					case I2C1_PB8_PB7:
						hi2c = &hi2c1;
						stri2cOffset = 1;

#if _EEPROM_EXC_PINS
						ee24.recoverI2CPins();
#endif
						break;
					case I2C1_EX_PB6_PB7:
#if _EEPROM_EXC_PINS
						hi2c = &hi2c1;
						ee24.exchangeI2CPins();
#else
						hi2c = nullptr;
#endif
						stri2cOffset = 2;
						break;
					default:
						break;
				}
				u8g2.drawStr(0, y, strI2cInfo[stri2cOffset]);
				uint8_t indexColums = 0;
				uint8_t x = 0;
				i2cDevices = 0;
				if(hi2c){
					for (i = 0; i < 127; i++) {
						status = HAL_I2C_Master_Transmit(hi2c, i << 1, 0, 0,50);
						if (status == HAL_OK) {
							++i2cDevices;	//设备计数+1,从1开始计数
							sprintf(buffUSB, "0x%02X", i);
	//						u8g2.drawStr(0, y  + i2cDevices * 16, buffUSB);
							u8g2.drawStr(x, y + 16 + indexColums * 16, buffUSB);
							memset(buffUSB, 0, 5);

							indexColums = (indexColums + 1) % 2;
							if(indexColums == 0 && x == 0)
								x = 33;
							else if(indexColums == 0 && x == 33)
								x = 0;
							else
								;
						}
					}
				}
				if (!i2cDevices)	//未扫描到设备
					u8g2.drawStr(0, y + 16, "Nothing!");
				u8g2.sendBuffer();
				i2cLine = (i2cLine + 1) % 3;
				if (ok == 0)
					break;
			}
		}
	}
}

//重启
void columsHome_Reset()
{
	bool ok = colums_StrSelect(true, SEL_2, nullptr, 1);
	if(ok)
	{
//		shutScreen();
//		uint16_t timeWaitingStartPage = HAL_GetTick();
//		for (;;) {
//			if (HAL_GetTick() - timeWaitingStartPage > 666)
//				break;
//			resetWatchdog();
//			HAL_Delay(MENU_DELAY);
//		}

		NVIC_SystemReset();
	}
}

void drawLogoAndVersion()
{
	//第一页
//	u8g2.drawdrawXBM();
	u8g2.drawBitmap(0, 17, startLogo_H, startLogo_W, startLogo_SAU_G0);
	u8g2.setFont(u8g2_font_IPAandRUSLCD_tr); //7pixel字体;
	u8g2.drawStr(42, 39 , "v1.0");
}

void columsHome_ShowVerInfo() {
	u8g2.clearBuffer();
	drawLogoAndVersion();
	u8g2.sendBuffer();
	if (waitingSelect(SEL_3)) {
		//第二页
		u8g2.clearBuffer();
		for (int i = 0; i < 6; i++)
			u8g2.drawStr(0, 8 + i * 8, strVerInfo[i]);
		u8g2.sendBuffer();
		waitingSelect(SEL_3);
	}
}


void synchronisedTimeSys(){
	synchronisedUintDateTime(&dtSys);
}
void synchronisedUintDateTime(uintDateTime * udt){
	DateTime now = RTC_GetNowDateTime();
	udt->yOff = now.year() - 2000;
	udt->m = now.month();
	udt->d = now.day();
	udt->hh = now.hour();
	udt->mm = now.minute();
	udt->ss = now.second();
}
//当未开始任务，强制同步采样开始日期到最近的时间
void synchronisedTimeStartCollect(){
	if(!systemSto.data.settingsBits[colBits].bits.bit7 && 	//若任务开关：关闭
			systemSto.data.NumDataCollected.uint_16 == 0) {	//且已采集的数据为0组
		synchronisedUintDateTime(&systemSto.data.dtStartCollect);
	}
}
