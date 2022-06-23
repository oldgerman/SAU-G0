#ifndef __MCU_TYPE
#define __MCU_TYPE

#include "stm32g0xx.h"
#include "stm32g0xx_hal_conf.h"	//改为hal库

//#define __STM32F4__
#define F_CPU SystemCoreClock
#define CYCLES_PER_MICROSECOND (F_CPU / 1000000U)

//#define __KEILDUINO__ 290
//
//#define GPIO_HIGH(GPIOx,GPIO_Pin)    (GPIOx->BSRR = GPIO_Pin)
//#define GPIO_LOW(GPIOx,GPIO_Pin)     (GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U)
//#define GPIO_READ(GPIOx,GPIO_Pin)   ((GPIOx->IDR   & GPIO_Pin)!=0)
//
//#define GPIO_NUMBER           16U
//#define GPIO_TOGGLE(GPIOx,GPIO_Pin)  (GPIOx->BSRR = ((GPIOx->ODR & GPIO_Pin) << GPIO_NUMBER) | (~(GPIOx->ODR) & GPIO_Pin))
//
//#define digitalWrite_HIGH(Pin) (GPIO_HIGH  (PIN_MAP[Pin].GPIOx, PIN_MAP[Pin].GPIO_Pin))
//#define digitalWrite_LOW(Pin)  (GPIO_LOW   (PIN_MAP[Pin].GPIOx, PIN_MAP[Pin].GPIO_Pin))
//#define digitalRead_FAST(Pin)  (GPIO_READ  (PIN_MAP[Pin].GPIOx, PIN_MAP[Pin].GPIO_Pin))
//#define togglePin(Pin)         (GPIO_TOGGLE(PIN_MAP[Pin].GPIOx, PIN_MAP[Pin].GPIO_Pin))

#endif
