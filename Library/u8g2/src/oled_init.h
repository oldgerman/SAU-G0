/*
 * oled_init.h
 *
 *  Created on: Jan 17, 2021
 *  Author：eziya/STM32_HAL_U8G2_OLED
 *  		https://github.com/eziya/STM32_HAL_U8G2_OLED
 *  Modify: OldGerman
 */

#ifndef __OLED_INIT_H
#define __OLED_INIT_H

#include "u8g2.h"
#include "U8g2lib.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define U8X8_PIN_NONE 255
#define TX_TIMEOUT 100
#define OLED_WIDTH 64
#define OLED_HEIGHT 48

uint8_t u8x8_byte_stm32_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_stm32_gpio_and_delay(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);

#ifdef __cplusplus

class U8G2_SSD1306_128X32_UNIVISION_HW_I2C : public U8G2 {
  public: U8G2_SSD1306_128X32_UNIVISION_HW_I2C(const u8g2_cb_t *rotation, uint8_t reset = U8X8_PIN_NONE)
  : U8G2()
  {
// 	  初始化 u8g2 结构体
	  u8g2_Setup_ssd1306_64x48_er_f(&u8g2, rotation, u8x8_byte_stm32_hw_spi, u8x8_stm32_gpio_and_delay);
  }
};

extern U8G2_SSD1306_128X32_UNIVISION_HW_I2C u8g2;
}
#endif

#endif // __OLED_INIT_H

