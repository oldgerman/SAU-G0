 /*
 * BSP.cpp
 *
 *  Created on: May 16, 2022
 *      Author: OldGerman
 */

#include "BSP.h"
#include "DFRobot_AHT20.h"

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

//释放信号量以解锁I2C
void FRToI2CxSInit()
{
	FRToSI2C1.FRToSInit();
	FRToSI2C2.FRToSInit();
}

void unstick_I2C(I2C_HandleTypeDef * I2C_Handle) {
#if 1
  GPIO_InitTypeDef GPIO_InitStruct;
  int              timeout     = 100;
  int              timeout_cnt = 0;
  uint32_t SCL_Pin;
  uint32_t SDA_Pin;
#ifdef STM32G0
#include "stm32g0xx.h"
#elif STM32F1
#include "stm32f103xb.h"
#elif defined(STM32F401xC)
#include "stm32f401xc.h"
#else
#error  "No Matching STM32xxx in unstick_I2C()"
#endif

  GPIO_TypeDef  * SCL_GPIO_Port;
  GPIO_TypeDef  * SDA_GPIO_Port;

  // 1. Clear PE bit.
  I2C_Handle->Instance->CR1 &= ~(0x0001);
  /**I2C1 GPIO Configuration
   PB6     ------> I2C1_SCL
   PB7     ------> I2C1_SDA
   */
  //  2. Configure the SCL and SDA I/Os as General Purpose Output Open-Drain, High level (Write 1 to GPIOx_ODR).
  GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

#if(defined(SCL1_Pin) && defined(SCL1_Pin))
  if(I2C_Handle == &hi2c1)
  {
	  SCL_Pin = SCL1_Pin;
	  SDA_Pin = SDA1_Pin;
	  SCL_GPIO_Port = SCL1_GPIO_Port;
	  SDA_GPIO_Port = SDA1_GPIO_Port;
  }
#endif
 #if(defined(SCL2_Pin) && defined(SCL2_Pin))
  if(I2C_Handle == &hi2c2)
  {
	  SCL_Pin = SCL2_Pin;
	  SDA_Pin = SDA2_Pin;
	  SCL_GPIO_Port = SCL2_GPIO_Port;
	  SDA_GPIO_Port = SDA2_GPIO_Port;
  }
#endif
#if(defined(SCL3_Pin) && defined(SCL3_Pin))
  if(I2C_Handle == &hi2c3)
  {
	  SCL_Pin = SCL3_Pin;
	  SDA_Pin = SDA3_Pin;
	  SCL_GPIO_Port = SCL3_GPIO_Port;
	  SDA_GPIO_Port = SDA3_GPIO_Port;
  }
#endif
  GPIO_InitStruct.Pin = SCL_Pin;
  HAL_GPIO_Init(SCL_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = SDA_Pin;
  HAL_GPIO_Init(SDA_GPIO_Port, &GPIO_InitStruct);
  HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET);

  while (GPIO_PIN_SET != HAL_GPIO_ReadPin(SDA_GPIO_Port, SDA_Pin)) {
    // Move clock to release I2C
    HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_RESET);
    asm("nop");
    asm("nop");
    asm("nop");
    asm("nop");
    HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET);

    timeout_cnt++;
    if (timeout_cnt > timeout)
      return;
  }

  // 12. Configure the SCL and SDA I/Os as Alternate function Open-Drain.
  GPIO_InitStruct.Mode  = GPIO_MODE_AF_OD;
  GPIO_InitStruct.Pull  = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

  GPIO_InitStruct.Pin = SCL_Pin;
  HAL_GPIO_Init(SCL_GPIO_Port, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = SDA_Pin;
  HAL_GPIO_Init(SDA1_GPIO_Port, &GPIO_InitStruct);

  HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET);

  // 13. Set SWRST bit in I2Cx_CR1 register.
  I2C_Handle->Instance->CR1 |= 0x8000;

  asm("nop");

  // 14. Clear SWRST bit in I2Cx_CR1 register.
  I2C_Handle->Instance->CR1 &= ~0x8000;

  asm("nop");

  // 15. Enable the I2C peripheral by setting the PE bit in I2Cx_CR1 register
  I2C_Handle->Instance->CR1 |= 0x0001;

  // Call initialization function.
  HAL_I2C_Init(I2C_Handle);
#endif
}
