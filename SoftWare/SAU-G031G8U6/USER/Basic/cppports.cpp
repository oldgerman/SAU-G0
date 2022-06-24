/*
 * cppports.cpp
 *
 *  Created on: May 16, 2022
 *      Author: OldGerman
 */

#include "cppports.h"
#include "MillisTaskManager.h"
/******************* C标准库 *******************/
#include "stdio.h"   //提供vsnprintf()
#include "stdlib.h"  //提供abs()
#include "string.h"	  //提供memset()
#include "dtostrf.h" //提供dtostrf()
/******************* 字体图标 *******************/
#include "BMP.h"		//提供一些字体和图标的位图
/******************* 硬件驱动 *******************/
#include "oled_init.h"
#include "ee24.hpp"
#include "Buttons.hpp"
#include "Page.hpp"
#include "CustomPage.hpp"


#define ms_ADC_Calibration	200			//ADC自校准最少时间
bool 	sysPowerOn;				//首次开机标记


/******************* RTC *******************/
//由于SWD和串口不同时使用，故Debug只能模拟串口接收到了数据

RTC_PCF212x rtc;
DateTime now;	//now变量即作为打印时间的变量，也作为串口修改的时间字符串存储的变量
bool nowCOMChanged;
extern const char daysOfTheWeek[7][12];
#define DBG_COM_DATE_TIME_ADJ 0					////< Change 0 to 1 to open debug
#if DBG_COM_DATE_TIME_ADJ
char testSaveBuffer[17] = {
		'2', '2', '/', 						    //2022年
		'0', '5','/', '2', '2', '/', 			//5月22日
		'2', '1','/', '2', '6', '/', '4', '7'	//21时26分47秒
};
#endif
uint8_t COMDateTimeAdjust_countIsDigit;
uintDateTime  nowRXbufferToNum;
/******************* USART *******************/
uint8_t aRxTemp[2];
uint8_t aRxBuffer[RX_BUFFER_SIZE];
uint8_t TxBuffer[TX_BUFFER_SIZE];
uint8_t aRxSaveBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxSaveCounter = 0;
volatile uint8_t txBusy = 0;
volatile uint8_t rxDone = 0;
volatile uint8_t rxBusy = 0;
volatile uint8_t rxCounter = 0;
volatile uint32_t rxTick = 0;

/******************* 按键状态全局变量 *******************/
extern ButtonState buttons;


/******************* 编译日期 *******************/
typedef struct tagXDate {
	int year;
	int month;
	int day;
} XDate;

XDate compileXDate;	//编译日期决定版本号后3组XX数字："v1.0.XX.XX.XX"

/************ 16bit ADC DMA 测电池电压 ************/
uint16_t ADCReadings[ADC_SAMPLES] = {0};
static void calibrationADC(){
//	__ExecuteFuncWithTimeout();
	  while (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
		  ;
}
void startADC(){
	  HAL_ADC_Start_DMA(&hadc1,(uint32_t *)&ADCReadings,ADC_SAMPLES);
}
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
	sysPowerOn = true;

	Power_Init();
#if 1
//主线程序
	ee24.autoInit(false);
	restoreSettings(); //恢复设置
	setupGUI();
	setupCOM();
	setupRTC();
	TH_Init();
	IMU_Init();
	//校准ADC、开启ADC DMA
	calibrationADC();
	startADC();

    /*任务注册*/	//子任务里放for(;;)会阻塞其他任务//注意调度时间占比，影响主屏幕时间的秒点闪烁周期的平均度
    mtmMain.Register(loopGUI, 20);                	//25ms：屏幕刷新
    mtmMain.Register(loopRTC, 100);                 //100ms：RTC监控
    mtmMain.Register(loopMIX, 200);   			 	//200ms：杂七杂八的传感器监控
    mtmMain.Register(loopCOM, 1000);            	//1000ms：COM收发监控
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
		int i = 0;
		ee24.recoverI2CPins();
		Debug_deviceSize = ee24.determineMemSize();
		Debug_pageSize =  ee24.determinePageSize();
		i = 0;
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

void setupRTC()
{
	nowCOMChanged = false;
	//如果是STOP1模式恢复过来，那么3.3V断过电，但RTC是没有断VBAT电源的，之前的配置仍然有效，所以不需要重新初始化
	//从STOP1模式恢复过来时，
	if(recoverFromSTOP1 == false){
	  if (!rtc.begin(&FRToSI2C1, false)){
		DBG_PRINT("Couldn't find RTC");

		rtc.setIntAlarm(RESET);	//关闭Alarm中断
		rtc.clearFlagAlarm();	//清除Alarm Flag
	  }
	}

#if 0
  if (rtc.lostPower()) {
    DBG_PRINT("RTC source clock is running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
//	    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(SECONDS_FROM_1970_TO_2000));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
#endif
}


void loopRTC() {
	if(nowCOMChanged)
	{
		rtc.adjust(DateTime(
			nowRXbufferToNum.yOff + 2000U,//补上2000年
			nowRXbufferToNum.m,
			nowRXbufferToNum.d,
			nowRXbufferToNum.hh,
			nowRXbufferToNum.mm,
			nowRXbufferToNum.ss
		));
		nowCOMChanged = false;
	}
	DBG_PRINT("%d/%d/%d %d:%d:%d\r\n",
			//2022/12/31 (Wednesday) 21:45:32
			//最多31个ASCII字符 + "\r\n\0" 是34个
			nowRXbufferToNum.yOff,
			nowRXbufferToNum.m,
			nowRXbufferToNum.d,
			nowRXbufferToNum.hh,
			nowRXbufferToNum.mm,
			nowRXbufferToNum.ss
	);

	now = rtc.now();
	DBG_PRINT("%d/%d/%d (%s) %d:%d:%d\r\n",
			//2022/12/31 (Wednesday) 21:45:32
			//最多31个ASCII字符 + "\r\n\0" 是34个
			now.year(),
			now.month(),
			now.day(),
			daysOfTheWeek[now.dayOfTheWeek()],
			now.hour(),
			now.minute(),
			now.second()
	);
	DBG_PRINT(" since midnight 1/1/1970 = %d s = %dd", now.unixtime(), now.unixtime() / 86400L);

	// calculate a date which is 7 days, 12 hours, 30 minutes, 6 seconds into the future
	DateTime future (now + TimeSpan(7,12,30,6));
	DBG_PRINT(" now + 7d + 12h + 30m + 6s: %d/%d/%d %d:%d:%d",
			future.year(),
			future.month(),
			future.day(),
			future.hour(),
			future.minute(),
			future.second()
	);
}



void loopMIX()
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
			uint8_t data[4] = {0};
			data[0] = C_X4T.num3*10 + C_X4T.num2;
			data[1] = C_X4T.num1*10 + C_X4T.num0;
			data[2] = RH_X4T.num3*10 + RH_X4T.num2;
			data[3] = RH_X4T.num1*10 + RH_X4T.num0;
			ee24.writeBytes(
					sizeof(systemStorageType) + (systemSto.data.NumDataCollected.uint_16 - 1)*sizeof(data),	//本次写入的地址偏移，注意乘以数据组大小
					data, 			//
					sizeof(data));	//每次写入的数据byte数
			//获取结构体成员偏移结构体地址多少byte
			ee24.writeBytes(0, systemSto.data.NumDataCollected.ctrl, 2);	//保存已采集个数
		}
		else {
			systemSto.data.settingsBits[colBits].bits.bit7 = 0;	//任务开关:关闭
			__LimitValue(systemSto.data.NumDataCollected.uint_16, //限制数量
					0 , systemSto.data.NumDataWillCollect);
		}
	}
}

//检查串口键入时间的有效性
void checkCOMDateTimeAdjustAvailable()
{
#if DBG_COM_DATE_TIME_ADJ
		char* ptr = testSaveBuffer;
#else
	char* ptrBuf = (char*)aRxSaveBuffer;
#endif

   uint8_t* ptrNum = (uint8_t *) &(nowRXbufferToNum.yOff);
   for(int i = 0; i < 6; i++)
	   *(ptrNum+i) = 0;

	int j = 0;
    for(int i = 0; i < 17 ; i++)
    {
      if(!isDigit(*ptrBuf))
        j++;
      else
    	  *(ptrNum + j) =  *(ptrNum + j) * 10 + (*ptrBuf - '0');	//	字符转整形

      //与arduino串口每次接收1个字符不同
      //本程序串口是y用缓冲区接收完整的一个字符串，因此ptr++
      ptrBuf++;
    }
    COMDateTimeAdjust_countIsDigit = 17 - j;	//计算字符串中数字的个数

    bool CheckNumRange = checkDateTimeAdjust(&nowRXbufferToNum);

    //检查数字范围和数字个数的有效性
	if(CheckNumRange && COMDateTimeAdjust_countIsDigit  == 12) {
		usb_printf("Input is valid! ");
	   nowCOMChanged = true;
	}
	else{
		usb_printf("Invalid input! ");
	   nowCOMChanged = false;
	}
}

bool checkDateTimeAdjust(uintDateTime *dt) {
	bool CheckNumRange = true;	//判断是否在有效范围
    //检查时间范围有效性
	uint16_t year = dt->yOff;
    uint8_t month = dt->m;
    uint8_t date =  dt->d;

    //判断时分秒范围
    if(((0 <= dt->hh && dt->hh <= 23) &&
	   (0 <= dt->mm && dt->mm <= 59) &&
	   (0 <= dt->ss && dt->ss <= 59))) {
		if(0 <= year && year <= 99){														//年份是否有效
			if (1 <= month && month <= 12) { 												//月份是否有效
				if(month==4 || month==6 || month==9 || month==11) { 						//是否为非2月的小月
					if(!(1 <= date && date <= 30)) CheckNumRange = false;					//小月最多30天
				}
				else {																		//为2月或大月
						if(month==2) {														//检查2月
							year += 2000U;													//补上2000年
							//判断闰年 （四年一闰，百年不闰) || 四百年在闰年
							if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) { 	//是闰年
								if(!(1 <= date && date <= 29))	CheckNumRange = false;		//检查2月日期（闰年 = 29天）
							}else{															//非闰年
								if(!(1 <= date && date <= 28))	CheckNumRange = false; 		//检查2月日期（非闰年 = 28天）
							}
						}
						else { 																//检查大月
							if(!(1 <= date && date <= 31)) CheckNumRange = false;			//日期是否在大月范围
						}
					}
				}else CheckNumRange = false; 												//月份无效
			}else CheckNumRange = false; 													//年份无效
	}else CheckNumRange = false;															//时分秒无效
    return CheckNumRange;
}
void setupCOM()
{
	HAL_UART_Receive_IT(&huart2, aRxTemp, 1); //串口接收中断启动函数
	DBG_PRINT("MCU: Initialized.\r\n");
}
void loopCOM()
{
#if DBG_COM_DATE_TIME_ADJ
		  checkCOMDateTimeAdjustAvailable();
#else
		loopUART(HAL_UART_TIMEOUT_VALUE);
		if (rxSaveCounter && (!txBusy)) {
		  checkCOMDateTimeAdjustAvailable();
		  u2Print(aRxSaveBuffer, rxSaveCounter);
		  rxSaveCounter = 0;
		}
#endif
//		HAL_Delay(500);	//XCOM上位机定时发送数据的周期大于等于HAL_Delay()的时间才不会丢帧
}

void i2cScan(I2C_HandleTypeDef *hi2c, uint8_t i2cBusNum) {
	uint8_t i = 0;
	HAL_StatusTypeDef status;
	DBG_PRINT("MCU: i2c%d scan...\r\n",i2cBusNum);

	for (i = 0; i < 127; i++) {
		status = HAL_I2C_Master_Transmit(hi2c, i << 1, 0, 0, 200);
		if (status == HAL_OK) {
			DBG_PRINT("addr: 0x%02X is ok\r\n",i);
		} else if (status == HAL_TIMEOUT) {
			DBG_PRINT("addr: 0x%02X is timeout\r\n",i);
		} else if (status == HAL_BUSY) {
			DBG_PRINT("addr: 0x%02X is busy\r\n",i);
		}
	}
}

void loopI2cScan(uint16_t ms) {
	static uint8_t mark = 0;
	static uint64_t tick_cnt_old = 0;
	uint64_t tick_cnt = HAL_GetTick();
	if (tick_cnt - tick_cnt_old > ms) {
		tick_cnt_old = tick_cnt;

		if (mark) {
			i2cScan(&hi2c2, 2);
			mark = 0;
		} else {
			i2cScan(&hi2c1, 1);
			mark = 1;
		}
	}
}



static uint8_t indexFanFrame;
static uint8_t secondPrev = now.second();
static bool secondPiontColor = 1;	//秒时间闪烁点的颜色，1绘制白色，0绘制黑色
static uint64_t secondPionthalf1s;		//秒点的半秒标记
static uint32_t drawFanTimeOld;

void setupGUI() {
	indexFanFrame = 0;
	secondPrev = now.second();
	secondPiontColor = 1;
	drawFanTimeOld = HAL_GetTick();

	u8g2.begin();
	u8g2.setDisplayRotation(U8G2_R2);
	u8g2.setDrawColor(1);
	u8g2.setBitmapMode(0);	//0是无色也覆盖下层的bitmap，无需u8g2.clearBuffer();
	u8g2.clearBuffer();

	//强制更新最后一次动作状态时间
//	lastButtonTime = HAL_GetTick() - systemSto.data.SleepTime;
//	lastMovementTime = HAL_GetTick() - systemSto.data.SleepTime;

	//从Flash载入屏幕亮度为screenBrightness的最大值
	//woc,那这里德国烙铁写错了
	screenBrightness.upper = systemSto.data.ScreenBrightness;
	setContrast(*screenBrightness.val);	//这个时候*val还是0
	u8g2.sendBuffer();	//相当于在发送调节背光命令

	if(systemSto.data.settingsBits[sysBits].bits.bit0){
		//绘制开机logo
		drawLogoAndVersion();
		u8g2.sendBuffer();
		brightScreen();
		uint32_t timeOld = HAL_GetTick();
		while(!waitTime(&timeOld, 888))
			;
		u8g2.clearBuffer();
	}
}

//执行过长的绘图程序中调用，例如绘制温湿度BITMAP
//风扇是中心对称，每转每1/4转到会自对称度需要5帧，转一圈20帧
void drawFan(bool sendBuffer){
	if(waitTime(&drawFanTimeOld, 10))
	{
		//绘制小风扇
		u8g2.setDrawColor(0);
		u8g2.drawXBM(49 - 1, 26 - 2, 16, 16, &icon16x16Fan[indexFanFrame][0]);
		indexFanFrame = (indexFanFrame + 1) % 5;	//0~4
		u8g2.setDrawColor(1);
		u8g2.drawBox(0, 23, 64, 2);	//分割线
		u8g2.setDrawColor(1);
		if(sendBuffer)
			u8g2.sendBuffer();
	}
}

void loopGUI() {
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
//			if(*screenBrightness.val != screenBrightness.lower){
				oldButtons = buttons;
				synchronisedTimeSys();				//同步系统设置时间到最近一次时间，兼顾初始化
				synchronisedTimeStartCollect();		//同步系采集开始时间到最近一次时间
				enterSettingsMenu(); // enter the settings menu
				u8g2.clearBuffer();
				markBackFromMenu = true;
//			}
			break;
		case BUTTON_OK_LONG:
			break;
		default:
			break;
		}
	}

	Power_AutoShutdownUpdate();	//从STOP模式退出，在此处继续执行

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
		drawFan(1);
		u8g2.drawBitmap(40, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busPNum1][0]);
		u8g2.drawBitmap(56, 0, icon8x14_W, icon8x14_H, icon8x14DegreeC);//	摄氏度符号

		busZNum1 = RH_X1 / 10;
		RH_X1 = RH_X1 % 10;
		busZNum2 = RH_X1;
		drawFan(1);
		u8g2.drawBitmap(5, 26, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum1][0]);
		u8g2.drawBitmap(21, 26, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum2][0]);
		u8g2.drawBitmap(37, 26, icon8x10_W, icon8x10_H, icon8x10Percent);//	湿度百分符号
	}

	drawFan(1);


//绘制休眠检测标记点，显示表示不休眠，不显示表示休眠时间已到
//	if(shouldBeSleeping())
//	{
//		u8g2.setDrawColor(0);
//		u8g2.drawBox(0, 0, 2, 2);
//		u8g2.setDrawColor(1);
//	}else {
//		u8g2.drawBox(0, 0, 2, 2);
//	}


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
				//now.second()
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



/**
 * @brief 以非阻塞模式发送数据到USRAT2
 * 			会回调重写的HAL_UART_TxCpltCallback()回调函数
 * 			length通常是用rxSaveCounter计数器的值
 * 			单独使用此函数也行，但一般配合串口接收中断用
 * 			单独使用建议使用usb_printf_IT()
 * 			Send message to UART2 peripheral.
 * @param {uint8_t}*sendBuff Buffer to send
 * @param {uint16_t}length Message length, 0 for const
 * @return {uint8_t} error code 1 for uart and Double Buffering busy, 2 for
 * empty or too long message.
 */
uint8_t u2Print(uint8_t *sendBuff, uint16_t length) {
  if (length == 0)
	  return 2;
  if (!txBusy) {
    txBusy = 1;
    HAL_UART_Transmit_IT(&huart2, sendBuff, length);
  } else
    return 1;
  return 0;
}


/**
 * usb_printf()的非阻塞版本
 */
void usb_printf_IT(const char *format, ...) {
	va_list args;
	uint32_t length;
	va_start(args, format);
	length = vsnprintf((char*) TxBuffer, TX_BUFFER_SIZE, (char*) format, args);
	va_end(args);

	u2Print(TxBuffer, length);
}

/**
 * 从https://blog.csdn.net/u010779035/article/details/104369515修改
 */
void usb_printf(const char *format, ...) {
	va_list args;
	uint32_t length;
	va_start(args, format);
	length = vsnprintf((char*) TxBuffer, TX_BUFFER_SIZE, (char*) format, args);
	va_end(args);
	HAL_UART_Transmit(&huart2, TxBuffer, length, 1000);
}

/**
 * @brief uart service, call in while loop
 * @param {*}
 * @return {*}
 */
void loopUART(uint16_t ms) {
  if (rxDone || (rxBusy && HAL_GetTick() - rxTick > ms)) {
    rxDone = 1;
    if (rxSaveCounter) return;
    memcpy(aRxSaveBuffer, aRxBuffer, rxCounter);
    rxSaveCounter = rxCounter;
    rxCounter = 0;
    rxBusy = 0;
    rxDone = 0;
  }
}

/**
 * @brief  串口发送总断回调函数
 * 			解除busy标签
 * @param {UART_HandleTypeDef} *huart
 * @return {*}
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  txBusy = 0;
}

/**
 * @brief  Rx Transfer Uploadd callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART2) {
    if (!rxDone) {  //丢弃数据
      rxBusy = 1;
      rxTick = HAL_GetTick();
      aRxBuffer[rxCounter++] = aRxTemp[0];
      if (rxCounter > RX_BUFFER_SIZE) {
        rxDone = 1;
      }
    }
    HAL_UART_Receive_IT(&huart2, aRxTemp, 1);
  }
}


#if 1
void drawLogoAndVersion()
{
	//第一页
//	u8g2.drawdrawXBM();
	u8g2.drawBitmap(0, 17, startLogo_H, startLogo_W, startLogo_SAU_G0);
	u8g2.setFont(u8g2_font_IPAandRUSLCD_tr); //7pixel字体;
	u8g2.drawStr(42, 39 , "v1.0");
}
#else

/*此函数会在Os优化下占用4.19KB，
 * 引入了 _vfiprintf_r 和 _vfprintf_r等恐龙级函数*/
bool GetCompileDate(XDate *date) {
	bool succeed = true;
	char complieDate[] = { __DATE__ };	//"Jul 06 2021"
	//字符串长度，可使用strlen()函数直接求出，切记，在使用strlen()求出字符串长度时，勿忘+1

	/**
	 strtok、strtok_s、strtok_r 字符串分割函数
	 https://blog.csdn.net/hustfoxy/article/details/23473805
	 */
	char *ptr;
	ptr = strtok(complieDate, " ");
	char *month = ptr;
	ptr = strtok(nullptr, " ");
	char *day = ptr;
	ptr = strtok(nullptr, " ");
	char *yearNoIntercept = ptr;	//未截取的年分：4位
	char year[3] = { 0 };					//储存截取年份的后2位
	/*
	 * C语言截取从某位置开始指定长度子字符串方法
	 * https://blog.csdn.net/zmhawk/article/details/44600075
	 */
	strncpy(year, yearNoIntercept + 2, 2);	//截取年后两位
	ptr = strtok(nullptr, " ");
	date->day = atoi(day);	//atoi()函数：将字符串转换成int(整数)
	if (date->day == 0)
		succeed = false;
	date->year = atoi(year);	//atoi()函数：将字符串转换成int(整数)
	if (date->year == 0)
		succeed = false;
	//依次判断月份
	const char months[][4] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
			"Aug", "Sep", "Oct", "Nov", "Dec" };
	date->month = 0;
	for (int i = 0; i < 12; i++) {
		if (strcmp(month, months[i]) == 0) {
			date->month = i + 1;
			break;
		}
	}
	if (date->month == 0)
		succeed = false;
	return succeed;
}

void drawLogoAndVersion(char firmwareMark)
{
	//第一页
//	u8g2.drawXBM(0, 0, 128, 32, startLogo);
	u8g2.setFont(u8g2_font_IPAandRUSLCD_tr); //7pixel字体;

	char buf[15] { 0 };	//"v1.0.21.06.12"; 固定13个可打印字符
	if (GetCompileDate(&compileXDate)) {
		sprintf(buf, "<A>%02d.%02d.%02d", compileXDate.year,
				compileXDate.month, compileXDate.day);
	} else {
		sprintf(buf, "<A>XX.XX.XX");
	}
//	uint8_t x = OLED_WIDTH - strlen(buf) * 5/*5=字体宽度*/- 2/*计算失误的偏差*/;	//版本号右对齐
	u8g2.drawStr(1, 10 , " Uni-Sensor");
	u8g2.drawStr(4, 20 , "       v1.0");
	u8g2.drawStr(0, 30, buf);
}
#endif
/****************************************************
*函数:strtoint(char *str,int result)
*输入:unsigned 字符串
*输出：整型数字
比如：收到“12345”  赋值给变量就是12345
https://blog.csdn.net/u013457167/article/details/45459887
*****************************************************/
//int mi(unsigned char dat, unsigned char mi) {
//	unsigned char i;
//	int sum = 1;
//
//	for (i = 0; i < mi; i++)
//		sum = sum * dat;
//
//	return sum;
//}
//int strtoint(unsigned char* str, int result)
// {
//	int i, tmp = 0;         //i,tmp临时变量
//	int length = strlen((char*) str);         //strlen参数为const char*,故强制转换
//	i = 0;
//	if (str[0] == '-')  //	可以输入-12345，负号也给你转成有符号整数
//		i = 1;
//	for (; i < length; i++) {
//		tmp = str[i] & 0x0f;         //如果原数组中存放的是ascii码，直接将其转换为数字
//		result += tmp * mi(10, length - i - 1); //1*100+2*10+3*1
//	}
//	if (str[0] == '-')
//		return -result;
//	return result;
//}
