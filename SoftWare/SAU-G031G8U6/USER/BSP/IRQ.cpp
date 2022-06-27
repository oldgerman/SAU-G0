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
#if 0
	intFromRTC = true;
	loopDataCollet();
#elif 0	//仅用于测试从STOP1唤醒，实际不能使用这个，需要保持RAM数据
//使用 NVIC_SystemReset() 函数可以软复位系统，最好配合关闭全局中断使用，以免出现意外操作
//	__set_FAULTMASK(1);//关闭所有中断
	__disable_irq();
	NVIC_SystemReset();
#elif 1
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
#endif

}
/*
 * 非阻塞模式（中断和DMA）中使用的I2C IRQHandler和回调（对__weak重写）
 * I2C IRQHandler and Callbacks used in non blocking modes (Interrupt and DMA)
 */
//void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }	//主接收完成
//void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }	//主发送完成
//void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
//void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
//void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
//void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }

#if 0
/* 外部GPIO中断组 EXTI9_5_IRQn 的回调函数。Pins.h中定义的加速度计和FUSB的中断共用的回调函数
 * 实际上仅处理FUSB302B的中断
 * #define INT_Movement_Pin          GPIO_PIN_5		//PB5: ACCEL_IN2
 * #define INT_Movement_GPIO_Port    GPIOB
 * #define INT_PD_Pin                GPIO_PIN_9		//PA9: FUSB302 INT pin
 * #define INT_PD_GPIO_Port          GPIOA
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  //将无用变量强制转换为void的方式, 消除编译器警告 与上面的__unused效果相同
  (void)GPIO_Pin;
  //usb_printf("HAL_GPIO_EXTI_Callback-------------------------\r\n");
  InterruptHandler::irqCallback();	//class InterruptHander 专门用于处理GPIO EXTI5_9
}



bool ADC_Injected_Callback_Mark = false;
/*
 * Catch the IRQ that says that the conversion is done on the temperature
 * readings coming in Once these have come in we can unblock the PID so that it
 * runs again
 * 注入模式的转换完成的中断的回调函数，一旦进入温度读数
 * 我们就可以解除PID的任务里由if判断通知是否来的代码块的阻塞，以便它再次运行
 *
 * 该函数由注入序列完成时触发
 */
/* ADC IRQHandler and Callbacks used in non-blocking modes (Interruption) */
void HAL_ADCEx_InjectedConvCpltCallback(ADC_HandleTypeDef *hadc) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  if (hadc == &hadc1) {
	  //这里ADC中断完成时PID线程会get到这个任务通知，这个直接影响PID计算周期
    if (pidTaskNotification) {
    	// 17.1 任务通知简介 (正点原子STM32 FreeRTOS开发手册)
    	// xTaskNotifyGive(): 发送通知，不带通知值并且不保留接收任务的通知值，此 函数会将接收任务的通知值加一，用于任务中。
    	// vTaskNotifyGiveFromISR(): 发送通知，函数 xTaskNotifyGive()的中断版本
      vTaskNotifyGiveFromISR(pidTaskNotification, &xHigherPriorityTaskWoken);
      //					 ^~~~~~~发送给PIDTask
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
  }
}



void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc) {
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  //测试用
  //usb_printf("HAL_ADC_ConvCpltCallback!\r\n");
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc) {
  /* Prevent unused argument(s) compilation warning */
  UNUSED(hadc);
  //测试用
  //usb_printf("HAL_ADC_ConvHalfCpltCallback!\r\n");
}
#endif