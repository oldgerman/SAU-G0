/*
 * oled_init.h
 *
 *  Created on: Jan 17, 2021
 *  Author：eziya/STM32_HAL_U8G2_OLED
 *  		https://github.com/eziya/STM32_HAL_U8G2_OLED
 *  Modify: OldGerman
 */

#include "oled_init.h"
#include "main.h"


U8G2_SSD1306_128X32_UNIVISION_HW_I2C u8g2(U8G2_R0);


void OLED_Init(void){
	u8g2.begin();
	u8g2.setDisplayRotation(U8G2_R0);
	u8g2.setDrawColor(1);
	u8g2.setBitmapMode(0);	//0是无色也覆盖下层的bitmap，无需u8g2.clearBuffer();
	u8g2.clearBuffer();
}


uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	/* STM32 supports HW SPI, Remove unused cases like U8X8_MSG_DELAY_XXX & U8X8_MSG_GPIO_XXX */
	switch(msg)
	{
	case U8X8_MSG_GPIO_AND_DELAY_INIT:
		/* Insert codes for initialization */
		break;
	case U8X8_MSG_DELAY_MILLI:
		/* ms Delay */
		HAL_Delay(arg_int);
		break;
	case U8X8_MSG_GPIO_CS:
		/* Insert codes for SS pin control */
		HAL_GPIO_WritePin(DISP_CS_GPIO_Port, DISP_CS_Pin, (GPIO_PinState)arg_int);
		break;
	case U8X8_MSG_GPIO_DC:
		/* Insert codes for DC pin control */
		HAL_GPIO_WritePin(DISP_DC_GPIO_Port, DISP_DC_Pin, (GPIO_PinState)arg_int);
		break;
	case U8X8_MSG_GPIO_RESET:
		/* Insert codes for RST pin control */
		HAL_GPIO_WritePin(DISP_RES_GPIO_Port, DISP_RES_Pin, (GPIO_PinState)arg_int);
		break;
	}
	return 1;
}

//注意软件控制CS引脚
uint8_t u8x8_byte_stm32_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr)
{
	switch(msg) {
	case U8X8_MSG_BYTE_SEND:
		/* Insert codes to transmit data */
		if(HAL_SPI_Transmit(&hspi1, (uint8_t *)arg_ptr, arg_int, TX_TIMEOUT) != HAL_OK) return 0;
		break;
	case U8X8_MSG_BYTE_INIT:
		/* Insert codes to begin SPI transmission */
		break;
	case U8X8_MSG_BYTE_SET_DC:
		/* Control DC pin, U8X8_MSG_GPIO_DC will be called */
		u8x8_gpio_SetDC(u8x8, arg_int);
		break;
	case U8X8_MSG_BYTE_START_TRANSFER:
		/* Select slave, U8X8_MSG_GPIO_CS will be called */
		u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
		HAL_Delay(1);
		break;
	case U8X8_MSG_BYTE_END_TRANSFER:
		HAL_Delay(1);
		/* Insert codes to end SPI transmission */
		u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
		break;
	default:
		return 0;
	}
	return 1;
}
