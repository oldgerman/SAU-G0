/*
 * BSP_ADC.cpp
 *
 *  Created on: Jun 24, 2022
 *      Author: OldGerman
 */

#include "BSP.h"

#define ADC_MODE_DMA 1	//是否使用ADC DMA模式
#define ADC_SAMPLES 16	//取2的幂次(移位快速计算平均值)

uint16_t ADCReadings[ADC_SAMPLES] = {0};

/**
 * @brief 校准ADC
 */
static void calibrationADC(){
	//	__ExecuteFuncWithTimeout();
	while (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
		;
}

/**
 * @brief 初始化ADC
 */
void ADC_Start(){
	calibrationADC();
	HAL_ADC_Start_DMA(&hadc1,(uint32_t *)&ADCReadings,ADC_SAMPLES);
}


/**
 * @brief 获取ADC数据，注意数据来源于DMA更新的缓冲区
 */
uint16_t ADC_Get(){
#if ADC_MODE_DMA
	/*
	 *sum 是否会溢出?
	 *	ADC_SAMPLES * 65535 = 1048560
	 *	uint32_t 0~4294967295
	 */
	uint32_t sum = 0;
	for (uint8_t i = 0; i < ADC_SAMPLES; i++) {
		sum += ADCReadings[i];
	}
	return sum >> 4;
#else

#endif
}

void ADC_Stop(){
	HAL_ADC_Stop_DMA(&hadc1);
}

/**
 * @brief 更新ADC采集（若使用DMA无需此函数，否则在while loop中调用）
 */
void ADC_Update(){
#if ADC_MODE_DMA
	;
#else
	//规则模式采集一个通道
	//暂未写代码
#endif
}
