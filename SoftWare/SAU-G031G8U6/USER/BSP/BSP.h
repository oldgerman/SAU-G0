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
#endif

void BSPInit(void);
void resetWatchdog();
void FRToI2CxSInit(); //释放信号量以解锁I2C
void unstick_I2C(I2C_HandleTypeDef *);
bool waitTime(uint32_t *timeOld, uint32_t wait);

/*IMU*/
void IMU_Init();
void IMU_Update();
extern uint32_t lastMovementTime;
/*Screen*/
void setContrast(uint16_t val);
void shutScreen();
void brightScreen();
void ScreenBK_Update(void (*FunPtr)(void));
/*PWR*/
bool shouldBeSleeping();
bool powerOffDetect(uint16_t ms);
void Power_Init();
void Power_AutoShutdownUpdate();
void STOP1_to_RUN();
extern bool recoverFromSTOP1;
#ifdef __cplusplus
}
#endif
#endif /* BSP_BSP_H_ */
