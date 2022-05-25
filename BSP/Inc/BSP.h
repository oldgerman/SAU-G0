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

// Called once from preRToSInit()
void BSPInit(void);

// Called to reset the hardware watchdog unit
void resetWatchdog();

//释放信号量以解锁I2C
void FRToI2CxSInit();

// This is a work around that will be called if I2C starts to bug out
// This should toggle the SCL line until SDA goes high to end the current transaction
void unstick_I2C(I2C_HandleTypeDef *);

// 加速度计方向的枚举类，用于oled旋转
enum Orientation {
	ORIENTATION_LEFT_HAND = 1,		//左手
	ORIENTATION_RIGHT_HAND = 0,		//右手
	ORIENTATION_FLAT = 3			//平坦的
};
#ifdef __cplusplus
}
#endif
#endif /* BSP_BSP_H_ */
