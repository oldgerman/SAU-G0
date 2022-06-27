/**
 * @author hey-frnk
 * @brief A lightweight, resource saving driver for SK6812 NeoPixels
 * using STM32 HAL, Timer PWM and DMA.
 *
 * @Github完整示例:
 * 			https://github.com/hey-frnk/STM32_HAL_NeoPixel
 * @Blog article and tutorial:
 * 			https://www.thevfdcollective.com/blog/stm32-and-sk6812-rgbw-led
 * @modify OldGerman
 */

#ifndef _LED_DRIVER_SK6812
#define _LED_DRIVER_SK6812

#include <stdint.h>

#ifdef __cplusplus
extern "C"{
void RGB_Updaate();/**
 * 特别注意：
 * 这个PWM_HI + PWM_LO = Timer Counter period的值+1，
 * 例如我CubeMX里设置为20-1，那么这加起来要为19
 * 若设置不正确,HAL库的两个PWM中断回调函数可能一直不会执行
 * 这个值分别如何取，请参考@Blog的 Figure 2: Timing waveform of the SK6812 ‘NeoPixel’ LED
 * 以及所使用的2812厂家手册给出的输入码型:0码:T0H、1码T1H、容许误差多少us
 * 取值计算若不为整数，那么四舍五入
 * 	例如Timer Counter period为60-1，那么根据2812手册高地电平的占空比可计算：
 * 	#define PWM_HI (38)		//64% x 60 = 38.4 --> logical 1
 * 	#define PWM_LO (19)		//32% x 60 = 19.2 --> logical 0
 */
//Timer Counter period为20-1
#define PWM_HI (38)	//64% x 20 = 12.8 --> logical 1
#define PWM_LO (19)  //32% x 20 = 6.4 --> logical 0


// LED parameters
// #define NUM_BPP (3)  // WS2812B
#define NUM_PIXELS (1)	// LED个数
#define NUM_BPP (3) 	// SK6812
#define NUM_BYTES (NUM_BPP * NUM_PIXELS)

void led_set_RGB(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void led_set_RGBW(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void led_set_all_RGB(uint8_t r, uint8_t g, uint8_t b);
void led_set_all_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void led_render();
void RGB_Update();
uint32_t hsl_to_rgb(uint8_t h, uint8_t s, uint8_t l);
#endif
#ifdef __cplusplus

}

#endif

#endif
