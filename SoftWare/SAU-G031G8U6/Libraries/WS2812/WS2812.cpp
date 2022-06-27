// Peripheral usage

#include "WS2812.h"
#include "BSP.h"
#include "stm32g0xx_hal.h"
#include "tim.h"

extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_tim1_ch1;
// LED brightness
uint8_t rgb_brightness = 30;
// LED color buffer
uint8_t rgb_arr[NUM_BYTES] = {0};

// LED write buffer
/*
 * buffer length
 * ws2812: 24bitx2	//一颗LED透传余下24bit，双缓冲
 * SK6812: 32bitx2
 *
 * WS2812 24bit 数据结构 3x8=24bit:
 * G7 G6 G5 G4 G3 G2 G1 G0, R7 R6 R5 R4 R3 R2 R1 R0, B7 B6 B5 B4 B3 B2 B1 B0
 * 注：高位先发，按照 GRB 的顺序发送数据
 */
#define WR_BUF_LEN (NUM_BPP * 8 * 2)
uint8_t wr_buf[WR_BUF_LEN] = {0};
uint_fast8_t wr_buf_p = 0;

static inline uint8_t scale8(uint8_t x, uint8_t scale) {
	return ((uint16_t)x * scale) >> 8;
}

// Set a single color (RGB) to index
void led_set_RGB(uint8_t index, uint8_t r, uint8_t g, uint8_t b) {
#if (NUM_BPP == 4) // SK6812
	rgb_arr[4 * index] = scale8(g, 0xB0); // g;
	rgb_arr[4 * index + 1] = r;
	rgb_arr[4 * index + 2] = scale8(b, 0xF0); // b;
	rgb_arr[4 * index + 3] = 0;
#else // WS2812B
//	rgb_arr[3 * index] = scale8(g, 0xB0); // g;
//	rgb_arr[3 * index + 1] = r;
//	rgb_arr[3 * index + 2] = scale8(b, 0xF0); // b;
	rgb_arr[3 * index] = map(scale8(g, 0xB0), 0, 255, 0, rgb_brightness); // g;
	rgb_arr[3 * index + 1] = map(r, 0, 255, 0, rgb_brightness);
	rgb_arr[3 * index + 2] = map(scale8(b, 0xF0), 0, 255, 0, rgb_brightness); // b;
#endif // End SK6812 WS2812B case differentiation
}

// Set a single color (RGBW) to index
void led_set_RGBW(uint8_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
	led_set_RGB(index, r, g, b);
#if (NUM_BPP == 4) // SK6812
	rgb_arr[4 * index + 3] = w;
#endif // End SK6812 WS2812B case differentiation
}

// Set all colors to RGB
void led_set_all_RGB(uint8_t r, uint8_t g, uint8_t b) {
	for(uint_fast8_t i = 0; i < NUM_PIXELS; ++i) led_set_RGB(i, r, g, b);
}

// Set all colors to RGBW
void led_set_all_RGBW(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
	for(uint_fast8_t i = 0; i < NUM_PIXELS; ++i) led_set_RGBW(i, r, g, b, w);
}

// Shuttle the data to the LEDs!
void led_render() {
	if(wr_buf_p != 0 || hdma_tim1_ch1.State != HAL_DMA_STATE_READY) {
		// Ongoing transfer, cancel!
		for(uint8_t i = 0; i < WR_BUF_LEN; ++i) wr_buf[i] = 0;
		wr_buf_p = 0;
		HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
		return;
	}
	// Ooh boi the first data buffer half (and the second!)
#if (NUM_BPP == 4) // SK6812
	for(uint_fast8_t i = 0; i < 8; ++i) {
		wr_buf[i     ] = PWM_LO << (((rgb_arr[0] << i) & 0x80) > 0);
		wr_buf[i +  8] = PWM_LO << (((rgb_arr[1] << i) & 0x80) > 0);
		wr_buf[i + 16] = PWM_LO << (((rgb_arr[2] << i) & 0x80) > 0);
		wr_buf[i + 24] = PWM_LO << (((rgb_arr[3] << i) & 0x80) > 0);
#if NUM_BYTES > 4
		wr_buf[i + 32] = PWM_LO << (((rgb_arr[4] << i) & 0x80) > 0);
		wr_buf[i + 40] = PWM_LO << (((rgb_arr[5] << i) & 0x80) > 0);
		wr_buf[i + 48] = PWM_LO << (((rgb_arr[6] << i) & 0x80) > 0);
		wr_buf[i + 56] = PWM_LO << (((rgb_arr[7] << i) & 0x80) > 0);
#endif
	}
#else // WS2812B
	for(uint_fast8_t i = 0; i < 8; ++i) {
		wr_buf[i     ] = PWM_LO << (((rgb_arr[0] << i) & 0x80) > 0);
		wr_buf[i +  8] = PWM_LO << (((rgb_arr[1] << i) & 0x80) > 0);
		wr_buf[i + 16] = PWM_LO << (((rgb_arr[2] << i) & 0x80) > 0);
#if NUM_BYTES > 3
		wr_buf[i + 24] = PWM_LO << (((rgb_arr[3] << i) & 0x80) > 0);
		wr_buf[i + 32] = PWM_LO << (((rgb_arr[4] << i) & 0x80) > 0);
		wr_buf[i + 40] = PWM_LO << (((rgb_arr[5] << i) & 0x80) > 0);
#endif
	}
#endif // End SK6812 WS2812B case differentiation

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)wr_buf, WR_BUF_LEN);
	wr_buf_p = 2; // Since we're ready for the next buffer
}

void HAL_TIM_PWM_PulseFinishedHalfCpltCallback(TIM_HandleTypeDef *htim) {
	if(htim->Instance == TIM1){
		// DMA buffer set from LED(wr_buf_p) to LED(wr_buf_p + 1)
		if(wr_buf_p < NUM_PIXELS) {
			// We're in. Fill the even buffer
#if (NUM_BPP == 4) // SK6812
			for(uint_fast8_t i = 0; i < 8; ++i) {
				wr_buf[i     ] = PWM_LO << (((rgb_arr[4 * wr_buf_p    ] << i) & 0x80) > 0);
				wr_buf[i +  8] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 1] << i) & 0x80) > 0);
				wr_buf[i + 16] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 2] << i) & 0x80) > 0);
				wr_buf[i + 24] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 3] << i) & 0x80) > 0);
			}
#else // WS2812B
			for(uint_fast8_t i = 0; i < 8; ++i) {
				wr_buf[i     ] = PWM_LO << (((rgb_arr[3 * wr_buf_p    ] << i) & 0x80) > 0);
				wr_buf[i +  8] = PWM_LO << (((rgb_arr[3 * wr_buf_p + 1] << i) & 0x80) > 0);
				wr_buf[i + 16] = PWM_LO << (((rgb_arr[3 * wr_buf_p + 2] << i) & 0x80) > 0);
			}
#endif // End SK6812 WS2812B case differentiation
			wr_buf_p++;
		} else if (wr_buf_p < NUM_PIXELS + 2) {
			// Last two transfers are resets. SK6812: 64 * 1.25 us = 80 us == good enough reset
			//                               WS2812B: 48 * 1.25 us = 60 us == good enough reset
			// First half reset zero fill
			for(uint8_t i = 0; i < WR_BUF_LEN / 2; ++i) wr_buf[i] = 0;
			wr_buf_p++;
		}
	}
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
	if(htim->Instance == TIM1){
		// DMA buffer set from LED(wr_buf_p) to LED(wr_buf_p + 1)
		if(wr_buf_p < NUM_PIXELS) {
			// We're in. Fill the odd buffer
#if (NUM_BPP == 4) // SK6812
			for(uint_fast8_t i = 0; i < 8; ++i) {
				wr_buf[i + 32] = PWM_LO << (((rgb_arr[4 * wr_buf_p    ] << i) & 0x80) > 0);
				wr_buf[i + 40] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 1] << i) & 0x80) > 0);
				wr_buf[i + 48] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 2] << i) & 0x80) > 0);
				wr_buf[i + 56] = PWM_LO << (((rgb_arr[4 * wr_buf_p + 3] << i) & 0x80) > 0);
			}
#else // WS2812B
			for(uint_fast8_t i = 0; i < 8; ++i) {
				wr_buf[i + 24] = PWM_LO << (((rgb_arr[3 * wr_buf_p    ] << i) & 0x80) > 0);
				wr_buf[i + 32] = PWM_LO << (((rgb_arr[3 * wr_buf_p + 1] << i) & 0x80) > 0);
				wr_buf[i + 40] = PWM_LO << (((rgb_arr[3 * wr_buf_p + 2] << i) & 0x80) > 0);
			}
#endif // End SK6812 WS2812B case differentiation
			wr_buf_p++;
		} else if (wr_buf_p < NUM_PIXELS + 2) {
			// Second half reset zero fill
			for(uint8_t i = WR_BUF_LEN / 2; i < WR_BUF_LEN; ++i) wr_buf[i] = 0;
			++wr_buf_p;
		} else {
			// We're done. Lean back and until next time!
			wr_buf_p = 0;
			HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
		}
	}
}


/* Fast hsl2rgb algorithm: https://stackoverflow.com/questions/13105185/fast-algorithm-for-rgb-hsl-conversion
 * RGB <--> HSL 转换的快速算法、
 * 快速算法（通常仅使用整数）将 RGB 数据转换为 HSL 数据，然后再转换回 RGB 数据。维基百科的转换算法并不快，因为需要浮动操作。
 * 以下是只使用整数的快速算法
 *
 * 三分钟带你快速学习RGB、HSV和HSL颜色空间 - https://zhuanlan.zhihu.com/p/67930839
 * RGB 颜色空间：
 * 	利用三个颜色分量的线性组合来表示颜色，任何颜色都与这三个分量有关，而且这三个分量是高度相关的，
 * 	所以连续变换颜色时并不直观，想对图像的颜色进行调整需要更改这三个分量才行。
 */
uint32_t hsl_to_rgb(uint8_t h, uint8_t s, uint8_t l) {
	if(l == 0) return 0;

	volatile uint8_t  r, g, b, lo, c, x, m;
	volatile uint16_t h1, l1, H;
	l1 = l + 1;
	if (l < 128)    c = ((l1 << 1) * s) >> 8;
	else            c = (512 - (l1 << 1)) * s >> 8;

	H = h * 6;              // 0 to 1535 (actually 1530)
	lo = H & 255;           // Low byte  = primary/secondary color mix
	h1 = lo + 1;

	if ((H & 256) == 0)   x = h1 * c >> 8;          // even sextant, like red to yellow
	else                  x = (256 - h1) * c >> 8;  // odd sextant, like yellow to green

	m = l - (c >> 1);
	switch(H >> 8) {       // High byte = sextant of colorwheel
	case 0 : r = c; g = x; b = 0; break; // R to Y
	case 1 : r = x; g = c; b = 0; break; // Y to G
	case 2 : r = 0; g = c; b = x; break; // G to C
	case 3 : r = 0; g = x; b = c; break; // C to B
	case 4 : r = x; g = 0; b = c; break; // B to M
	default: r = c; g = 0; b = x; break; // M to R
	}

	return (((uint32_t)r + m) << 16) | (((uint32_t)g + m) << 8) | ((uint32_t)b + m);
}

