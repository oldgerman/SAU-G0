 /*
 * BSP.h
 *
 *  Created on: May 16, 2022
 *      Author: OldGerman
 */

#include "main.h"
#include "I2C_Wrapper.h"
#include <stdbool.h>
#include <stdint.h>
#include "RTClib.h"	//提供uintDateTime

/*RTC编号表，无需修改，若修改，请保证每一个宏值不一样*/
#define RTC_IC_PCF212x 0
#define RTC_IC_PCF8563 1

/*设置你想用的RTC芯片*/
#define RTC_IC RTC_IC_PCF212x
//#define RTC_IC RTC_IC_PCF8563
//RTC的闹钟最小粒度
#if (RTC_IC == RTC_IC_PCF212x)		//PCF212x可支持秒级闹钟
#define RTC_IC_ALARM_SUPPORT_S 1
#elif (RTC_IC == RTC_IC_PCF8563)	//PCF8563只支持到分钟级闹钟
#define RTC_IC_ALARM_SUPPORT_S 0
#endif


/*
 * BSP.h -- Board Support
 *
 * This exposes functions that are expected to be implemented to add support for different hardware
 */

#ifndef BSP_BSP_H_
#define BSP_BSP_H_
#ifdef __cplusplus
extern FRToSI2C FRToSI2C1;
extern FRToSI2C FRToSI2C2;
extern "C" {
void usb_printf(const char *format, ...);
void usb_printf_IT(const char *format, ...);
#endif

extern bool firstPwrOffToRUN;
void preSetupInit(void);
void selfCheck();
void resetWatchdog();
void FRToI2CxSInit();
void unstick_I2C(I2C_HandleTypeDef *);
bool waitTime(uint32_t *timeOld, uint32_t wait);
/*ADC*/
void ADC_Start();
void ADC_Update();
uint16_t ADC_Get();
void ADC_Stop();
/*IMU*/
void IMU_Init();
void IMU_Update();
void IMU_SetThreshold();
extern uint32_t lastMovementTime;
/*TH*/
void TH_Init();
void TH_Update();
bool TH_DataUpdated();
int16_t TH_GetDataC_X10();
uint8_t TH_GetDataRH_X1();
/*RGB*/
void RGB_Update();
void RGB_TurnOff();
/*Contrast*/
void Contrast_Init();
void Contrast_SetVal();
void Contrast_SetUpperAndVal();
void Contrast_Darken();
void Contrast_Brighten();
/*PWR*/
bool shouldBeSleeping();
bool powerOffDetect(uint16_t ms);
void powerOn();
void actionStateTime_Reset();
void Power_Init();
void Power_AutoShutdownUpdate();
bool Power_IsCharging();
void STOP1_to_RUN();
extern bool recoverFromSTOP1;
/*USART*/
void USART_Init();
void USART_Update();
bool USART_DateTimeUpdated();
/*RTC*/
const char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
void RTC_Init();
void RTC_Update();
uint8_t RTC_GetNowSceond();
bool RTC_AlarmWillTrigger();

#ifdef __cplusplus
/*Contrast*/
void Contrast_Update(void (*FunPtr)(void) = nullptr);
/*USART*/
uintDateTime& USART_GetDateTime();
/*RTC*/
bool RTC_CheckUintDateTime(uintDateTime *dt);
DateTime& RTC_GetNowDateTime();
uint8_t RTC_GetNowSecond();
#if (RTC_IC == RTC_IC_PCF212x)
extern RTC_PCF212x rtc;
#elif (RTC_IC == RTC_IC_PCF8563)
extern RTC_PCF8563 rtc;
#endif
}
#endif
#endif /* BSP_BSP_H_ */
