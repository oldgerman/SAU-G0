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
 * 		2022/06/24
 * 			添加抬头原作者的Github和博客文章链接
 * 			添加一些中文注释
 * 		2022/06/26
 * 			修复BUG: 在NUM_PIXELS为1时，编译器警告rgb_arr越界访问
 * 			加入亮度调节支持rgb_brightness,范围0~255，LED电流不随亮度线性变化：
 * 			亮度255，实测2020 2812RGB 色轮旋转变化颜色时（最多两个色彩通道255），电流峰值24mA，与厂家手册给出的每通道12mA一致
 * 			亮度30， 实测2020 2812RGB 色轮旋转变化颜色时（最多两个色彩通道30）， 电流峰值1.5mA
 */

#ifndef _LED_DRIVER_SK6812
#define _LED_DRIVER_SK6812

#include <stdint.h>

// LED parameters
/**
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
 *
 * 	如果i处的位为0，则((rgb_arr[0] << i) & 0x80) > 0为0。所以wr_buf[i]等于
 * 	PWM_LO << 0。移位 0 是乘以 1，所以wr_buf[i]现在保持19
 * 	如果i处的位为1，则((rgb_arr[0] << i) & 0x80) > 0为1。所以wr_buf[i]等于
 * 	PWM_LO << 1。移位 1 是乘以 2，所以wr_buf[i]现在保持38，等于PWM_HI
 */
//Timer Counter period为20-1,即htim1.Instance->ARR
//const uint8_t PWM_HI = (htim1.Instance->ARR+1) *625/1250;	//10 --> logical 1
//const uint8_t PWM_LO = (htim1.Instance->ARR+1) *315/1250;  //5.04 --> logical 0
#define PWM_LO (5)

#define NUM_PIXELS (1)	// LED个数
#define NUM_BPP (3) 	// WS2812B	//不带白光通道
// #define NUM_BPP (4)  // SK6812	//带白光通道
#define NUM_BYTES (NUM_BPP * NUM_PIXELS)

void led_set_RGB(uint8_t index, uint8_t r, uint8_t g, uint8_t b);
void led_set_RGBW(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void led_set_all_RGB(uint8_t r, uint8_t g, uint8_t b);
void led_set_all_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void led_render();
uint32_t hsl_to_rgb(uint8_t h, uint8_t s, uint8_t l);
#endif
