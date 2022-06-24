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

void BSPInit(void);
void resetWatchdog();
void FRToI2CxSInit(); //释放信号量以解锁I2C
void unstick_I2C(I2C_HandleTypeDef *);
bool waitTime(uint32_t *timeOld, uint32_t wait);

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
/*Contrast*/
void Contrast_Set(uint16_t val);
void Contrast_Darken();
void Contrast_Brighten();
void Contrast_Update(void (*FunPtr)(void));
/*PWR*/
bool shouldBeSleeping();
bool powerOffDetect(uint16_t ms);
void Power_Init();
void Power_AutoShutdownUpdate();
void STOP1_to_RUN();
extern bool recoverFromSTOP1;
/*USART*/
void USART_Init();
void USART_Update();
bool USART_DateTimeUpdated();
/*RTC*/
void RTC_Init();
void RTC_Update();

#ifdef __cplusplus
/*USART*/
bool RTC_CheckUintDateTime(uintDateTime *dt);
uintDateTime& USART_GetDateTime();
}
#endif
#endif /* BSP_BSP_H_ */
