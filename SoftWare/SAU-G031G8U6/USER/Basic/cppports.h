 /*
 * cppports.h
 *
 *  Created on: May 16, 2022
 *      Author: OldGerman
 */

#ifndef INC_CPPPORTS_H_
#define INC_CPPPORTS_H_

#include "main.h"
#include "BSP.h"
#include "oled_init.h"
#include "IRQ.h"		//提供 intFromRTC 标记
#include "RTClib.h"
#include "Colum.hpp"
#ifdef __cplusplus

extern "C" {

void usb_printf(const char *format, ...);

#endif

/* Includes ------------------------------------------------------------------*/

/* 私有包含 Private includes -------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* 导出类型 Exported types ---------------------------------------------------*/
/* USER CODE BEGIN ET */

#define ADC_SAMPLES 16	//取2的幂次(移位快速计算平均值)
extern uint16_t ADCReadings[ADC_SAMPLES];
/* USER CODE END ET */

/* 导出常量 Exported constants -----------------------------------------------*/
/* USER CODE BEGIN EC */
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

/* USER CODE END EC */

/* 导出的宏 Exported macro ---------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* 导出函数原型 Exported functions prototypes --------------------------------*/

/* USER CODE BEGIN EFP */
void i2cScan(I2C_HandleTypeDef *hi2c, uint8_t i2cBusNum);
//void blink(void) { blinkState = 2; }
//void loopBlink(uint16_t ms);
uint8_t u2Print(uint8_t *, uint16_t);

void loopUART(uint16_t ms);
void loopI2cScan(uint16_t ms);
bool waitTime(uint32_t *timeOld, uint32_t wait);

void setup();
void loop();

void setupGUI();
void loopGUI();

void setupCOM();
void loopCOM();

void setupRTC();
void loopRTC();

void loopMIX();
	void setupAHT20();
	void loopAHT20();
	void DataCollect_Update();

void setupADC();

void brightScreen();
void shutScreen();
void screenBrightAdj();
void setContrast(uint16_t val);
void drawLogoAndVersion();

//void doAPPWork();//低优先级时间片调度任务：用户APP和UI绘图，SPI的发送其实可以改到DMA
//void doRTCWork();//IRQ任务优先级最高：RTC中断输出1Hz方波同步计时，暂时未实现通过IRQ释放信号量解除阻塞
//void doCOMWork();//高优先级后台任务：串口收发，其实可以用接收中断的信号量阻塞任务
//void doMIXWork();//较高优先级后台任务：传感器数据检测
/* USER CODE END EFP */

/* 私有定义 Private defines --------------------------------------------------*/
/* USER CODE BEGIN Private defines */


#ifndef DBG_PRINT
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PRINT usb_printf
#else
	#define DBG_PRINT(...)
	#endif
#endif
#define TX_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64
/* USER CODE END Private defines */

#ifdef __cplusplus
extern RTC_PCF212x rtc;
extern DateTime now;
bool checkDateTimeAdjust(uintDateTime *dt);
extern AutoValue screenBrightness;
}
#endif

#endif /* INC_CPPPORTS_H_ */
