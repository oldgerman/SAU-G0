/*
 * IRQ.c
 *
 *  Created on: 30 May 2020
 *      Author: Ralim
 *      Modify:OldGerman
 */

#include "IRQ.h"
#include "I2C_Wrapper.h"
#include "cppports.h"
#include <stdio.h>	//提供 __unused 宏
#include "BSP.h"	//提供	lastMovementTime;

volatile bool intFromRTC = false;	//RTC中断标记
bool shouldDataCollect = false;	//
void HAL_GPIO_EXTI_Falling_Callback(uint16_t GPIO_Pin){
#if 1
	/*在本回调函数中读这个引脚的值来判断当前系统是STOP1还是RUN模式
	 *
	 * PW_HOLD = 1: G031在RUN模式
	 * PW_HOLD = 0: G031在STOP1模式
	 */
	if(HAL_GPIO_ReadPin(PW_HOLD_GPIO_Port, PW_HOLD_Pin) == GPIO_PIN_SET){
		//在RUN模式接收到PB5的RTC下降沿中断
		if(GPIO_Pin == INT_RTC_Pin){
			intFromRTC = true;
		}
		//在RUN模式接收到PA0的加速度计下降沿中断
		else if(GPIO_Pin == INT_IMU_Pin){
			lastMovementTime = HAL_GetTick();	//更新动作时间
		}
		else if(GPIO_Pin == KEY_OK_Pin){
			;
		}
	}else{
		STOP1_to_RUN();
		recoverFromSTOP1 = true; //修改从stop1模式恢复标记为true，用于防止再次初始化RTC
		//在STOP1模式接收到下降沿中断，恢复系统配置
		if(GPIO_Pin == INT_RTC_Pin){
			intFromRTC = true;
		}
		else if(GPIO_Pin == INT_IMU_Pin){
			;
		}
		else if(GPIO_Pin == KEY_OK_Pin){
			;
		}
	}
#elif 0
#endif
}
