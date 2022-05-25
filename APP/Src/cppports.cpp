/*
 * cppports.cpp
 *
 *  Created on: May 16, 2022
 *      Author: OldGerman
 */

#include "cppports.h"
#include "cmsis_os.h"
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
#include "RTClib.h"
#include "Buttons.hpp"

#define swPressedTimePowerOn 100		//至少短按开机时间
#define swPressedTimeshutDown 1000		//至少长按关机时间
/******************* 温湿度计 *******************/
uint8_t th_RH_X1 = 0;		//范围0~100，无小数点
int16_t th_C_X10 = 0;		//温度有正负，-9.9~85.0
bool th_MeasurementUpload = false;


/******************* 加速度计 *******************/
#define MOVFilter 8
bool DetectedAccelerometer = false;		//检测到加速度计的标记
uint8_t accelInit = 0;		// 首次标记
TickType_t lastMovementTime = 0;		//首次进入shouldShuntDown()函数置为1才会累计时间
int32_t axisError = 0;
AxisData axData;
AxisAvg axAvg;
/******************* RTC *******************/
//由于SWD和串口不同时使用，故Debug只能模拟串口接收到了数据
#define DBG_COM_DATE_TIME_ADJ 0					////< Change 0 to 1 to open debug

RTC_PCF212x rtc;
DateTime now;	//now变量即作为打印时间的变量，也作为串口修改的时间字符串存储的变量
bool nowCOMChanged;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

#if DBG_COM_DATE_TIME_ADJ
char testSaveBuffer[17] = {
		'2', '2', '/', 						    //2022年
		'0', '5','/', '2', '2', '/', 			//5月22日
		'2', '1','/', '2', '6', '/', '4', '7'	//21时26分47秒
};
#endif
uint8_t COMDateTimeAdjust_countIsDigit;
/*
 * now_RXbufferToNum结构体类型，用整形临时存储时间
 * 例如成员分别为 {22 05 22 18 19 00}	表示 2022年5月22日18点19分00秒
 */
struct now_RXbufferToNum{
	uint8_t year;	//	后面的处理需要自己加2000
	uint8_t month;
	uint8_t date;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
} nowRXbufferToNum;
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

int mi(unsigned char dat, unsigned char mi) {
	unsigned char i;
	int sum = 1;

	for (i = 0; i < mi; i++)
		sum = sum * dat;

	return sum;
}
/******************* 按键状态全局变量 *******************/
extern ButtonState buttons;
/****************************************************
*函数:strtoint(char *str,int result)
*输入:unsigned 字符串
*输出：整型数字
比如：收到“12345”  赋值给变量就是12345
https://blog.csdn.net/u013457167/article/details/45459887
*****************************************************/
int strtoint(unsigned char* str, int result)
 {
	int i, tmp = 0;         //i,tmp临时变量
	int length = strlen((char*) str);         //strlen参数为const char*,故强制转换
	i = 0;
	if (str[0] == '-')  //	可以输入-12345，负号也给你转成有符号整数
		i = 1;
	for (; i < length; i++) {
		tmp = str[i] & 0x0f;         //如果原数组中存放的是ascii码，直接将其转换为数字
		result += tmp * mi(10, length - i - 1); //1*100+2*10+3*1
	}
	if (str[0] == '-')
		return -result;
	return result;
}

void doRTCWork() {
#if 1	//开启或禁用RTC线程
		nowCOMChanged = false;
	  if (!rtc.begin(&FRToSI2C1, false))
	    DBG_PRINT("Couldn't find RTC");

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
	for (;;) {
#if 0
		if (ulTaskNotifyTake(pdTRUE, 2000)) {

		} else {
			osDelay(1000);
		}
#else
		if(nowCOMChanged)
		{
			rtc.adjust(DateTime(
				nowRXbufferToNum.year + 2000U,//补上2000年
				nowRXbufferToNum.month,
				nowRXbufferToNum.date,
				nowRXbufferToNum.hour,
				nowRXbufferToNum.minute,
				nowRXbufferToNum.second
			));
			nowCOMChanged = false;
		}
		DBG_PRINT("%d/%d/%d %d:%d:%d\r\n",
				//2022/12/31 (Wednesday) 21:45:32
				//最多31个ASCII字符 + "\r\n\0" 是34个
				nowRXbufferToNum.year,
				nowRXbufferToNum.month,
				nowRXbufferToNum.date,
				nowRXbufferToNum.hour,
				nowRXbufferToNum.minute,
				nowRXbufferToNum.second
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
		osDelay(500);
#endif
	}
#endif
	osDelay(500);
}
/**
 * 比较运动或按钮超时阈值，来决定返回是否睡眠
 * 超时并且未检测到运动和按钮动作返回true
 */
static bool shouldBeSleeping(bool inAutoStart) {
	// Return true if the iron should be in sleep mode
//	if (systemSettings.Sensitivity && systemSettings.SleepTime) {
		if (inAutoStart) {
			// In auto start we are asleep until movement
			if (lastMovementTime == 0 /*&& lastButtonTime == 0*/) {
				return true;
			}
		}
		if (lastMovementTime > 0 /*|| lastButtonTime > 0*/) {
			if ((xTaskGetTickCount() - lastMovementTime) > 500 //1000ms 临时设置的休眠超时时间
				/*&& (xTaskGetTickCount() - lastButtonTime) > getSleepTimeout()*/) {
				return true;
			}
		}
//	}
	return false;
}

void printTaskUsage(const char *str, uint16_t stackSize,
		osThreadId TaskHandle) {
	uint16_t stackFreeSize = uxTaskGetStackHighWaterMark(TaskHandle);
	uint16_t used = stackSize - stackFreeSize;
	if(used);
	DBG_PRINT("%s: %3d/%-3d  %d%%\r\n", str, used, stackSize, used * 100 / stackSize);
}
void showTaskUsage(uint16_t ms) {
	static uint64_t tick_cnt_old = 0;
	uint64_t tick_cnt = xTaskGetTickCount();
	if (tick_cnt - tick_cnt_old > ms) {
		tick_cnt_old = tick_cnt;
		printTaskUsage("APPTask", APPTask_stacksize, APPTaskHandle);
		printTaskUsage("RTCTask", RTCTask_stacksize, RTCTaskHandle);
		printTaskUsage("COMTask", COMTask_stacksize, COMTaskHandle);
		printTaskUsage("MIXTask", MIXTask_stacksize, MIXTaskHandle);
	}
}
void doAPPWork() {
	setupGUI();
	u8g2.clearBuffer();
	u8g2.sendBuffer();
	for (;;) {
		loopGUI();
        loopPowerOffDetect(swPressedTimeshutDown);
		osDelay(50);
	}
}
void doMIXWork() {
	 powerOnDectet(swPressedTimePowerOn);
	 setupMOV();
	 setupAHT20();
	for (;;) {
//		loopBlink(blinkDelay);
//		loopI2cScan(1000);
//		showTaskUsage(2000);
		loopMOV();
		loopAHT20();	//4次状态机一次测量
		osDelay(100);	//也就是0.4s一次AHT20
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

   uint8_t* ptrNum = (uint8_t *) &(nowRXbufferToNum.year);
   for(int i = 0; i < 6; i++)
	   *(ptrNum+i) = 0;

	int j = 0;
	bool CheckNumRange = true;	//检测输入数字是否在有效范围
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

    //检查时间范围有效性
	uint16_t year = nowRXbufferToNum.year;
    uint8_t month = nowRXbufferToNum.month;
    uint8_t date =  nowRXbufferToNum.date;

    //判断时分秒范围
    if(((0 <= nowRXbufferToNum.hour && nowRXbufferToNum.hour <= 23) &&
	   (0 <= nowRXbufferToNum.minute && nowRXbufferToNum.minute <= 59) &&
	   (0 <= nowRXbufferToNum.second && nowRXbufferToNum.second <= 59))) {
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

void doCOMWork() {
#if 1		//开启或禁用COM线程
	HAL_UART_Receive_IT(&huart2, aRxTemp, 1); //串口接收中断启动函数
	DBG_PRINT("MCU: Initialized.\r\n");
	for (;;) {
#if DBG_COM_DATE_TIME_ADJ
		  checkCOMDateTimeAdjustAvailable();
#else
		loopUART(HAL_UART_TIMEOUT_VALUE);
		if (rxSaveCounter && (!txBusy)) {
		  taskENTER_CRITICAL();	//有必要吗，防止串口突然又来数据进入中断？
		  checkCOMDateTimeAdjustAvailable();
		  u2Print(aRxSaveBuffer, rxSaveCounter);
		  rxSaveCounter = 0;
		  taskEXIT_CRITICAL();
		}
#endif
		osDelay(500);	//XCOM上位机定时发送数据的周期大于等于osDelay()的时间才不会丢帧
	}
#else
	osDelay(500);
#endif
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
	uint64_t tick_cnt = xTaskGetTickCount();
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
	/*电源使能保持*/
	//usb_printf("Power On: Waiting...\r\n");
	uint32_t time = xTaskGetTickCount();
	while (xTaskGetTickCount() - time < ms) {
		HAL_Delay(10);	//注意这里不能用osDelay(),因为TIM17的中断好像还未被FreeRTOS配置到FreeRTOS的心跳节拍中同步计数
						//若在这里使用osDelay会导致按下z中键开机黑屏或oled显示一部分UI死机
						//切记！，在每个线程的setup之前不能用osDelay()
	}
	HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_SET);
}

void loopPowerOffDetect(uint16_t msShutDown) {
	static uint64_t tick_cnt_old = 0;
	static int64_t tick_sum = 0;

	uint64_t tick_cnt = xTaskGetTickCount();
	if (tick_cnt - tick_cnt_old > 100) {
		if (HAL_GPIO_ReadPin(KEY_OK_GPIO_Port, KEY_OK_Pin)
				== GPIO_PIN_RESET) {
			tick_sum += 100;
			tick_cnt_old = tick_cnt;
		} else {
			// 定义unsigned int a=0，a-1的结果不�????-1，�?�是4294967295（无符号�????0xFFFFFFFF），这是�????么原因呢�????
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
	if (tick_sum > msShutDown)
		HAL_GPIO_WritePin(PW_HOLD_GPIO_Port, PW_HOLD_Pin, GPIO_PIN_RESET);
}

void setupGUI() {
	u8g2.begin();
	//u8g2.setPowerSave(0);
	u8g2.setDrawColor(1);
	u8g2.setBitmapMode(0);	//0是无色也覆盖下层的bitmap，无需u8g2.clearBuffer();
	//u8g2.drawBitmap(0, 0, bitmap_height_TH_UI_Test, bitmap_width_TH_UI_Test,  bitmap_TH_UI_Test);
}

void loopGUI() {
//		u8g2.clearBuffer();
//绘制温湿度信息

		buttons = getButtonState();
	switch (buttons) {
	case BUTTON_NONE:
		break;
	case BUTTON_BOTH:
		break;
	case BUTTON_A_LONG:
		break;
	case BUTTON_B_LONG:
		break;
	case BUTTON_B_SHORT:
		break;
	case BUTTON_A_SHORT:
		break;
	case BUTTON_OK_SHORT:
		break;
	case BUTTON_OK_LONG:
		break;
	default:
		break;
	}


	if (th_MeasurementUpload) {
		th_MeasurementUpload = false;

		u8g2.setDrawColor(1);
		uint16_t busZNum1, busZNum2, busPNum1;
		// th_C_X10 = 234  实际23.4摄氏度
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
	if(shouldBeSleeping(true))
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
			(xTaskGetTickCount() - secondPionthalf1s >= 500))
	{
		secondPionthalf1s = xTaskGetTickCount(); //变相同步了 os系统滴答计时的0.5秒 与 RTC的1秒的 每秒时间
		secondPrev = now.second();

		u8g2.setFont(u8g2_font_IPAandRUSLCD_tf);	//8pixel字体
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
//		systemSettings.Sensitivity = 0;
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
		LIS3DH::getAxisData(axData);	//先读一次3轴向数据
		rotation = LIS3DH::getOrientation();	//再读一次姿态
	}
}

void setupMOV() {
	// 检测加速度计型号，并做寄存器初始化配置
	detectAccelerometerVersion();
	osDelay(50); // wait ~50ms for setup of accel to finalise

	// 清零没运动的计时标记值（用于判断超时休眠）
	lastMovementTime = 0;
}
void loopMOV() {
	static int16_t datax[MOVFilter] = { 0 };
	static int16_t datay[MOVFilter] = { 0 };
	static int16_t dataz[MOVFilter] = { 0 };
	static uint8_t currentPointer = 0;	//本次task
	static Orientation rotation = ORIENTATION_FLAT;	//默认姿态：平坦
	static int32_t threshold = map(10, 0, 100, 0, 2048);	//映射0~100到触发阈值0~2048范围
	//								^百分之10 临时使用

	/*******************for*******************************/
	readAccelerometer(&axData, rotation);
//
//		if (systemSettings.OrientationMode == 2) {
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
		lastMovementTime = xTaskGetTickCount();	//更新动作时间
		usb_printf("Movement detected!\r\n");		//若触发移动则打印
	}
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
  if (rxDone || (rxBusy && xTaskGetTickCount() - rxTick > ms)) {
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
//
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
      rxTick = xTaskGetTickCount();
      aRxBuffer[rxCounter++] = aRxTemp[0];
      if (rxCounter > RX_BUFFER_SIZE) {
        rxDone = 1;
      }
    }
    HAL_UART_Receive_IT(&huart2, aRxTemp, 1);
  }
}
//
//volatile uint8_t blinkState = 0;
//const uint16_t blinkDelay = 500;
//void loopBlink(uint16_t ms) {
//  static uint32_t tickBlink;
//    if (xTaskGetTickCount() - tickBlink >= ms) {
//    	HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, !HAL_GPIO_ReadPin(LED_GPIO_Port, LED_Pin));
//      tickBlink = xTaskGetTickCount();
//      //u2Print((uint8_t *)"\nBlink\n", 18);
//
//    }
//}

/**
 * 笔记
/ /sprintf 左对齐补零
 	把整数123 打印成一个字符串保存在s 中。
	sprintf(s, "%d", 123); //产生"123"可以指定宽度，不足的左边补空格：
	sprintf(s, "%8d%8d", 123, 4567); //产生：" 123 4567"当然也可以左对齐：
	sprintf(s, "%-8d%8d", 123, 4567); //产生："123 4567"

	也可以按照16 进制打印：
	sprintf(s, "%8x", 4567); //小写16 进制，宽度占8 个位置，右对齐
	sprintf(s, "%-8X", 4568); //大写16 进制，宽度占8 个位置，左对齐

	这样，一个整数的16 进制字符串就很容易得到，但我们在打印16 进制内容时，通常想要一种左边补0 的等宽格式，那该怎么做呢？很简单，在表示宽度的数字前面加个0 就可以了。
	sprintf(s, "%08X", 4567); //产生："000011D7"
	上面以"%d"进行的10 进制打印同样也可以使用这种左边补0 的方式。
 */

/**
 * *AHT20的傻瓜测试代码
			const uint8_t num_buffer = 9; //最多5个数字（100.00%RH）+小数点+正负号+结束空字符
			char buffer[num_buffer] ={0};

			DBG_PRINT("temperature(-40~85 C):\r\n");
			memset(buffer, 0, strlen(buffer));
//	 Get temp in Celsius (℃), range -40-80℃
			dtostrf(aht20.getTemperature_C(), 5, 2, buffer, num_buffer); // Convert floating point to char
			DBG_PRINT("%s C\r\n", buffer);
			DBG_PRINT("%d C\r\n", (uint16_t)(aht20.getTemperature_C()));
			memset(buffer, 0, strlen(buffer));
//	 Get temp in Fahrenheit (F)
			dtostrf(aht20.getTemperature_F(), 5, 2, buffer, num_buffer); // Convert floating point to char
		    DBG_PRINT("%s F\r\n", buffer);

		    DBG_PRINT("humidity(0~100):\r\n");
		    memset(buffer, 0, strlen(buffer));
//	 Get relative humidity (%RH), range 0-100%RH
			dtostrf(aht20.getHumidity_RH(), 6, 2, buffer, num_buffer); // Convert floating point to char
		    DBG_PRINT("%s \%RH\r\n", buffer);
		    DBG_PRINT("%d \%RH\r\n", (uint16_t)(aht20.getHumidity_RH()));
 */
