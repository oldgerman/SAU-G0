/*
 * BSP_RGB.cpp
 *
 *  Created on: Jun 24, 2022
 *      Author: OldGerman
 */

#if 1 //#if RGB_LED_EN


#include "BSP.h"
#include "ws2812.h"

static uint8_t angle = 0;//色轮角度
static const uint8_t angle_difference = 11;	//颜色步进

void RGB_Update(){
	if(Power_IsCharging()){
		// Demo code for 1 LED
		for (uint8_t i = 0;
				i < NUM_PIXELS /* Change that to your amount of LEDs */; i++) {
			// Calculate color
			uint32_t rgb_color = hsl_to_rgb(angle + (i * angle_difference), 255,
					127);
			// Set color
			led_set_RGB(i, (rgb_color >> 16) & 0xFF, (rgb_color >> 8) & 0xFF,
					rgb_color & 0xFF);
		}
		// Write to LED
		++angle;
	}
	else {
		led_set_RGB(0, 0, 0, 0);
		angle = 0;
	}
	led_render();
}

void RGB_TurnOff(){
	led_set_RGB(0, 0, 0, 0);
	angle = 0;
	led_render();
}

#endif

