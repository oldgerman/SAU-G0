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

#endif

/* Includes ------------------------------------------------------------------*/

/* 私有包含 Private includes -------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* 导出类型 Exported types ---------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* 导出常量 Exported constants -----------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* 导出的宏 Exported macro ---------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* 导出函数原型 Exported functions prototypes --------------------------------*/

/* USER CODE BEGIN EFP */

void setup();
void loop();
void MIX_Update();
/* USER CODE END EFP */

/* 私有定义 Private defines --------------------------------------------------*/
/* USER CODE BEGIN Private defines */


/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* INC_CPPPORTS_H_ */
