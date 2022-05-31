/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2022 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g0xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_A_Pin GPIO_PIN_1
#define KEY_A_GPIO_Port GPIOA
#define KEY_B_Pin GPIO_PIN_4
#define KEY_B_GPIO_Port GPIOA
#define DISP_RES_Pin GPIO_PIN_5
#define DISP_RES_GPIO_Port GPIOA
#define PW_HOLD_Pin GPIO_PIN_6
#define PW_HOLD_GPIO_Port GPIOA
#define KEY_OK_Pin GPIO_PIN_1
#define KEY_OK_GPIO_Port GPIOB
#define SCL2_Pin GPIO_PIN_11
#define SCL2_GPIO_Port GPIOA
#define SDA2_Pin GPIO_PIN_12
#define SDA2_GPIO_Port GPIOA
#define DISP_CS_Pin GPIO_PIN_15
#define DISP_CS_GPIO_Port GPIOA
#define DISP_DC_Pin GPIO_PIN_4
#define DISP_DC_GPIO_Port GPIOB
#define INT_RTC_Pin GPIO_PIN_5
#define INT_RTC_GPIO_Port GPIOB
#define INT_RTC_EXTI_IRQn EXTI4_15_IRQn
#define SDA1_Pin GPIO_PIN_7
#define SDA1_GPIO_Port GPIOB
#define SCL1_Pin GPIO_PIN_8
#define SCL1_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

#if defined(HAL_UART_TIMEOUT_VALUE)
#undef HAL_UART_TIMEOUT_VALUE
#define HAL_UART_TIMEOUT_VALUE 2	//单位毫秒，必须要再次定义，否则不发�?�接收到的数据了
#endif
extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart2;


void usb_printf(const char *format, ...);
void EXT_I2C1_Init();
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
