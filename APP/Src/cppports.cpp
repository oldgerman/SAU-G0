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
#include "DFRobot_AHT20.h"
#include "LIS3DH.hpp"
#include "ee24.hpp"
#include "Buttons.hpp"
#include "Page.hpp"
#include "CustomPage.hpp"
#define swPressedTimePowerOn 100		//至少短按开机时间
#define swPressedTimeshutDown 2000		//至少长按关机时间
#define oledContrastStepsMs 50			//oled每次亮度发生更改的步进时间
#define ms_ADC_Calibration	200			//ADC自校准最少时间
bool 	sysPowerOn = true;				//首次开机标记
/******************* 温湿度计 *******************/
uint8_t th_RH_X1 = 88;		//范围0~100，无小数点
int16_t th_C_X10 = 888;		//温度有正负，-9.9~85.0
bool th_MeasurementUpload = false;


/******************* 加速度计 *******************/
#define MOVFilter 8
bool DetectedAccelerometer = false;		//检测到加速度计的标记
uint8_t accelInit = 0;		// 首次标记
uint32_t lastMovementTime = 0;		//首次进入shouldShuntDown()函数置为1才会累计时间
int32_t axisError = 0;
AxisData axData;
AxisAvg axAvg;
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
/******************* 屏幕亮度 *******************/
uint16_t screenBrightnessVal = 0;
AutoValue screenBrightness(&screenBrightnessVal, 3, 100, 0, 5, 5, false);
bool firstScreenBright = true; //亮屏标记

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



/* 非阻塞下等待固定的时间
 * @param timeOld 必须传入局部静态变量或全局变量
 * @param 等待时间
 * @return bool
 */
bool waitTime(uint32_t *timeOld, uint32_t wait) {
	uint32_t time = HAL_GetTick();
	if ((time - *timeOld) > wait) {	//250决定按键长按的延迟步幅
		*timeOld = time;
		return true;
	}
	return false;
}


void setup(){
	powerOnDectet(swPressedTimePowerOn);
#if 1
//主线程序
	ee24.autoInit(false);
	restoreSettings(); //恢复设置
	setupGUI();
	setupCOM();
	setupRTC();
	setupAHT20();
	setupMOV();
	//校准ADC、开启ADC DMA
	calibrationADC();
	startADC();

    /*任务注册*/									//注意调度时间占比，影响主屏幕时间的秒点闪烁周期的平均度
    mtmMain.Register(loopGUI, 25);                	//25ms：屏幕刷新
    mtmMain.Register(loopRTC, 100);                 //100ms：RTC监控
    mtmMain.Register(loopMIX, 200);   			 	//200ms：杂七杂八的传感器监控
    mtmMain.Register(loopCOM, 1000);            	//1000ms：COM收发监控
    loop();	//暂时丢到这里方便其他单元测试
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
		uint32_t deviceSize = ee24.determineMemSize();
		uint16_t pageSize =  ee24.determinePageSize();
		int i = 0;
		ee24.recoverI2CPins();
		deviceSize = ee24.determineMemSize();
		pageSize =  ee24.determinePageSize();
		i = 0;
	}
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
  if (!rtc.begin(&FRToSI2C1, false))
    DBG_PRINT("Couldn't find RTC");
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
//		HAL_Delay(500);
}

/**
 * 返回睡眠超时阈值时间
 */
static uint32_t getSleepTimeout() {
		return systemSto.data.SleepTime * 1000;
}
/**
 * 比较运动或按钮超时阈值，来决定返回是否睡眠
 * 超时并且未检测到运动和按钮动作返回true
 */
static bool shouldBeSleeping() {
	if (systemSto.data.settingsBits[sysBits].bits.bit1) { 		//自动休眠位域
		if (lastMovementTime > 0 || lastButtonTime > 0) {		//只有当这两个值非0才可能进入休眠
			if ((HAL_GetTick() - lastMovementTime) > getSleepTimeout() //1000ms 临时设置的休眠超时时间
				&& (HAL_GetTick() - lastButtonTime) > getSleepTimeout()) {
				return true;
			}
		}
	}
	return false;
}

void loopMIX()
{
//		loopI2cScan(1000);
//		loopMOV();	//合并到screenBrightAdj()
		loopDataCollet();
		loopAHT20();	//4次状态机一次测量

//		HAL_Delay(100);	//也就是0.4s一次AHT20
}

void loopDataCollet(){
	if(systemSto.data.settingsBits[colBits].bits.bit7){
//		DateTime dt(
//				systemSto.data.STyy + 2000,
//				systemSto.data.STMM,
//				systemSto.data.STdd,
//				systemSto.data.SThh,
//				systemSto.data.STmm,
//				systemSto.data.STss);
//		columsDrawDateTime(&dt);
//
//		bool mmark;
//		DateTime alarmDateTime(2000, 1, 21, 0, 0, 5);	//每分钟的第5秒一次闹钟
//		mmark = rtc.alarmFired();
//		mmark =  rtc.clearFlagAlarm();
//		mmark = rtc.setTimeAlarm(&alarmDateTime, PCF212x_A_Second);
//	//	mmark = rtc.setTimeAlarm(&alarmDateTime, PCF212x_A_PerSecond);
//		mmark = rtc.setIntAlarm(true);
//		while(1) {
//			now = rtc.now();
//			if(intFromRTC){			//由G031的PB5检测PCF2129的中断下降沿回调函数更改
//	//		if(rtc.alarmFired()){	//也可以轮询检测
//				mmark = rtc.clearFlagAlarm();
//				mmark = rtc.alarmFired();
//				intFromRTC = false;
//			}
//		}

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

void powerOnDectet(uint16_t ms) {
	uint32_t timeOld = HAL_GetTick();
	/*电源使能保持*/
	//usb_printf("Power On: Waiting...\r\n");
	while (!waitTime(&timeOld, ms)){
		;
	}
	HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_SET);
}

void loopPowerOffDetect(uint16_t msShutDown) {
	static uint64_t tick_cnt_old = 0;
	static int64_t tick_sum = 0;

	uint64_t tick_cnt = HAL_GetTick();
	if (tick_cnt - tick_cnt_old > 100) {
		if (HAL_GPIO_ReadPin(KEY_OK_GPIO_Port, KEY_OK_Pin)
				== GPIO_PIN_RESET) {
			tick_sum += 100;
			tick_cnt_old = tick_cnt;
		} else {
			tick_sum -= 100;
			if (tick_sum < 0)
				tick_sum = 0;
		}
	}else
	{
		/*++ 很奇怪为啥要加这句*/
		HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_SET);
	}
	//累积关机
	if (tick_sum > msShutDown) {
		shutScreen();
		HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_RESET);
	}
}

void setupGUI() {
	u8g2.begin();
	u8g2.setDrawColor(1);
	u8g2.setBitmapMode(0);	//0是无色也覆盖下层的bitmap，无需u8g2.clearBuffer();
	u8g2.clearBuffer();

	//强制更新最后一次动作状态时间
	lastButtonTime = HAL_GetTick() - systemSto.data.SleepTime;
	lastMovementTime = HAL_GetTick() - systemSto.data.SleepTime;

	//从Flash载入屏幕亮度为screenBrightness的最大值
	//woc,那这里德国烙铁写错了
	screenBrightness.upper = systemSto.data.ScreenBrightness;
	setContrast(*screenBrightness.val);	//这个时候*val还是0
	u8g2.sendBuffer();	//相当于在发送调节背光命令
	firstScreenBright = true; //重置第一次亮屏标记

	if(systemSto.data.settingsBits[sysBits].bits.bit0){
		//绘制开机logo
		drawLogoAndVersion();
		u8g2.sendBuffer();
		brightScreen();
		uint32_t timeOld = HAL_GetTick();
		while(!waitTime(&timeOld, 888))
			;
//		shutScreen();
		u8g2.clearBuffer();
//		u8g2.sendBuffer();
	}
}

//这个函数准备用在定时器回调函数里调用
void screenBrightAdj(){
	loopMOV();
	//超时熄屏
	if (firstScreenBright
			&& (*screenBrightness.val == screenBrightness.upper 	//从上电或熄屏唤醒首次达到最大亮度
			|| *screenBrightness.val == screenBrightness.lower)){	//从熄屏最低亮度唤起
		firstScreenBright = false;
	}

	if (shouldBeSleeping()){
//		u8g2.setFont(u8g2_font_profont22_mr);	//12pixel 字间距
//		u8g2.setFontRefHeightExtendedText();
//		u8g2.drawStr(FONT16_XOFFSET, 15, "PWR OFF");
		static uint32_t timeOld = HAL_GetTick();
		if(waitTime(&timeOld, oledContrastStepsMs)) {
			screenBrightness--;
			setContrast(*screenBrightness.val);
		}
		if (*screenBrightness.val == 0) {
			u8g2.setPowerSave(1);
			firstScreenBright = true;
		} else
			u8g2.setPowerSave(0);
	} else {
			static uint32_t timeOld = HAL_GetTick();
			if(waitTime(&timeOld, oledContrastStepsMs)) {
				screenBrightness++;
				setContrast(*screenBrightness.val);
				u8g2.setPowerSave(0);
			}
	}
}
void loopGUI() {
	loopPowerOffDetect(swPressedTimeshutDown);
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
			if(*screenBrightness.val != screenBrightness.lower){
				oldButtons = buttons;
				enterSettingsMenu(); // enter the settings menu
				u8g2.clearBuffer();
				markBackFromMenu = true;
			}
			break;
		case BUTTON_OK_LONG:
			break;
		default:
			break;
		}
	}

	screenBrightAdj();

	//绘制温湿度信息
	if (th_MeasurementUpload || sysPowerOn || markBackFromMenu) {
		th_MeasurementUpload = false;
		sysPowerOn = false;
		u8g2.setDrawColor(1);
		uint16_t busZNum1, busZNum2, busPNum1;
		// th_C_X10 = 234  实际23.4摄氏度
		if(markBackFromMenu){
			th_C_X10 = 888;
			th_RH_X1 = 88;
		}
		busZNum1 = th_C_X10 / 100;	//2 得到十位
		th_C_X10 = th_C_X10 % 100;	//取模得到 34
		busZNum2 = th_C_X10 / 10;	//3 得到个位
		th_C_X10 = th_C_X10 % 10;	//取模得到4 小点数后第1位
		busPNum1 = th_C_X10;
		u8g2.drawBitmap(5, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum1][0]);
		u8g2.drawBitmap(21, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum2][0]);
		u8g2.drawBox(36, 20, 2, 2);	//温度小数点
		u8g2.drawBitmap(40, 0, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busPNum1][0]);
		u8g2.drawBitmap(56, 0, icon8x14_W, icon8x14_H, icon8x14DegreeC);//	摄氏度符号

		busZNum1 = th_RH_X1 / 10;
		th_RH_X1 = th_RH_X1 % 10;
		busZNum2 = th_RH_X1;
		u8g2.drawBitmap(5, 26, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum1][0]);
		u8g2.drawBitmap(21, 26, bmpDigitalStyle1_W, bmpDigitalStyle1_H,
				&bmpDigitalStyle1[busZNum2][0]);
		u8g2.drawBitmap(37, 26, icon8x10_W, icon8x10_H, icon8x10Percent);//	湿度百分符号
	}

	//绘制小风扇
	u8g2.setDrawColor(0);
	static uint8_t indexFanFrame = 0;
	u8g2.drawXBM(49 - 1, 26 - 2, 16, 16, &icon16x16Fan[indexFanFrame][0]);
	indexFanFrame = (indexFanFrame + 1) % 5;	//0~4
	u8g2.setDrawColor(1);
	u8g2.drawBox(0, 23, 64, 2);	//分割线

	//绘制休眠检测标记点，显示表示不休眠，不显示表示休眠时间已到
	if(shouldBeSleeping())
	{
		u8g2.setDrawColor(0);
		u8g2.drawBox(0, 0, 2, 2);
		u8g2.setDrawColor(1);
	}else {
		u8g2.drawBox(0, 0, 2, 2);
	}

	//绘制时间
	static uint8_t secondPrev = now.second();
	static bool secondPiontColor = 1;	//秒时间闪烁点的颜色，1绘制白色，0绘制黑色
	static uint64_t secondPionthalf1s;		//秒点的半秒标记

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

DFRobot_AHT20 aht20(&FRToSI2C1);
void setupAHT20() {
	uint8_t status;
	if ((status = aht20.begin()) != 0) {
		DBG_PRINT("AHT20 sensor initialization failed. error status : %d\r\n", status);
	}
}

void loopAHT20() {
	if (aht20.measurementFSM()) {
		th_C_X10 = aht20.getTemperature_C();
		th_RH_X1 = aht20.getHumidity_RH();
		th_MeasurementUpload = true;
	}
}


/**
 * @brief  检测加速度计型号，并做寄存器初始化配置
 * @param  void
 * @retval void
 */
void detectAccelerometerVersion() {
	DetectedAccelerometer = true;
	if (LIS3DH::detect()) {
		// Setup the ST Accelerometer
		if (LIS3DH::initalize()) {
			DetectedAccelerometer = true;
//			uint8_t whoamiptr;
//			uint8_t whoami = FRToSI2C2.Mem_Read(LIS3DH_I2C_ADDRESS, LIS_WHO_AM_I, &whoamiptr, 1);
		}
	} else {
		// disable imu sensitivity，没检测到加速度计，会禁用休眠模式
//		systemSto.data.Sensitivity = 0;
		DetectedAccelerometer = false;
	}
}

/**
 * @brief  读取加速度计
 * @param  x，y，z，枚举方向
 * @retval void
 */
inline void readAccelerometer(AxisData *axData, Orientation &rotation) {
	if (DetectedAccelerometer) {
		LIS3DH::getAxisData(axData);			//读3轴向数据
		rotation = LIS3DH::getOrientation();	//读姿态
	}
}

void loopMOV() {
	static int16_t datax[MOVFilter] = { 0 };
	static int16_t datay[MOVFilter] = { 0 };
	static int16_t dataz[MOVFilter] = { 0 };
	static uint8_t currentPointer = 0;	//本次task
	static Orientation rotation = ORIENTATION_FLAT;	//默认姿态：平坦
	int32_t threshold = map(systemSto.data.Sensitivity, 0, 100, 0, 2048);	//映射0~100到触发阈值0~2048范围
	//								^百分之10 临时使用

	/*******************for*******************************/
	readAccelerometer(&axData, rotation);
//
//		if (systemSto.data.OrientationMode == 2) {
//			if (rotation != ORIENTATION_FLAT) { //非平坦状态才会旋转屏幕
//				//rotation是否等于左手的值0？
//				//是则rotation=0, 即右手模式, 传入OLED::setRotation(1)
//				//否则rotation=1, 即左手模式, 传入OLED::setRotation(0)
//				//OLED::setRotation(rotation == ORIENTATION_LEFT_HAND); // link the data through
//				u8g2.setDisplayRotation((rotation == ORIENTATION_LEFT_HAND)? U8G2_R2: U8G2_R0);
//			}
//		}

	datax[currentPointer] = (int32_t) axData.x;
	datay[currentPointer] = (int32_t) axData.y;
	dataz[currentPointer] = (int32_t) axData.z;

	// 首次进入循环先将读到的一次三轴数据复制8份（MOVFilter = 8），塞满datax、datay、dataz的8个元素
	// 离开将首次标记accelInit置1，下次task不会再执行本if内的语句
	if (!accelInit) {
		for (uint8_t i = currentPointer + 1; i < MOVFilter; i++) {
			datax[i] = (int32_t) axData.x;
			datay[i] = (int32_t) axData.y;
			dataz[i] = (int32_t) axData.z;
		}
		accelInit = 1;
	}

	currentPointer = (currentPointer + 1) % MOVFilter; //整除则变为0，从0重新计数到7
	axAvg.avgx = axAvg.avgy = axAvg.avgz = 0;
	// calculate averages

	for (uint8_t i = 0; i < MOVFilter; i++) {
		axAvg.avgx += datax[i];
		axAvg.avgy += datay[i];
		axAvg.avgz += dataz[i];
	}

	axAvg.avgx /= MOVFilter;
	axAvg.avgy /= MOVFilter;
	axAvg.avgz /= MOVFilter;

	// 求三轴变化量绝对值的总和 //Sum the deltas	//abs()求传入数据的绝对值
	axisError = (abs(axAvg.avgx - axData.x) + abs(axAvg.avgy - axData.y)
			+ abs(axAvg.avgz - axData.z));
	// So now we have averages, we want to look if these are different by more
	// than the threshold

	// If movement has occurred then we update the tick timer
	// 如果发生了移动，那么我们更新滴答计时器
	if (axisError > threshold) {
		lastMovementTime = HAL_GetTick();	//更新动作时间
		usb_printf("Movement detected! lastMovementTime = %ld\r\n", lastMovementTime);		//若触发移动则打印
	}
}



/**
 * 阻塞熄屏函数
 */
void setupMOV() {
	// 检测加速度计型号，并做寄存器初始化配置
	detectAccelerometerVersion();
	HAL_Delay(50); // wait ~50ms for setup of accel to finalise

	// 清零没运动的计时标记值（用于判断超时休眠）
	lastMovementTime = 0;
}
void shutScreen() {
	for (;;) {
		static uint32_t timeOld = HAL_GetTick();
		if(waitTime(&timeOld, oledContrastStepsMs)) {
		screenBrightness--;
		setContrast(*screenBrightness.val);
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
void brightScreen() {
	u8g2.setPowerSave(0);
	for (;;) {
		static uint32_t timeOld = HAL_GetTick();
		if(waitTime(&timeOld, oledContrastStepsMs)) {
			screenBrightness++;
			setContrast(*screenBrightness.val);
			u8g2.sendBuffer();	//相当于在发送调节背光命令
			if (*screenBrightness.val == screenBrightness.upper)
				break;
		}
	}
}

//映射0~100亮度到oled背光寄存器0~255
void setContrast(uint16_t val) {
	u8g2.setContrast(map(*screenBrightness.val, 0, 100, 0, 255));
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
