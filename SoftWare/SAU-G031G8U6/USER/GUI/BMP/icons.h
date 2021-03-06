 /*
 * icons.h
 *
 *  Created on: May 17, 2022
 *      Author: OldGerman
 */
#ifndef __ICONS_H
#define	__ICONS_H

#include "stdint.h"
//#define bitmap_width_TH_UI_Test 48
//#define bitmap_height_TH_UI_Test 8	//8x8 = 64

#define startLogo_W 12
#define startLogo_H 8	//8x8 = 64
/*符号图标\SAU-G0.bmp",0*/
const uint8_t startLogo_SAU_G0[] = {
		0x0F,0xC7,0xC3,0x06,0x00,0x1F,0x01,0xF0,
		0x1F,0xCF,0xE3,0x06,0x00,0x3F,0x83,0xF8,
		0x3F,0xDF,0xF3,0x06,0x00,0x7F,0xC7,0xFC,
		0x38,0x1C,0x73,0x06,0x00,0x71,0xC7,0x1C,
		0x3C,0x18,0x33,0x06,0x00,0x60,0x06,0x0C,
		0x1F,0x1B,0xF3,0x06,0x7E,0x67,0xC6,0x0C,
		0x0F,0x9B,0xF3,0x06,0x7E,0x67,0xC6,0x0C,
		0x03,0xD8,0x33,0x06,0x00,0x60,0xC6,0x0C,
		0x01,0xD8,0x33,0x8E,0x00,0x71,0xC7,0x1C,
		0x3F,0xD8,0x33,0xFE,0x00,0x7F,0xC7,0xFC,
		0x3F,0x98,0x31,0xFC,0x00,0x3F,0x83,0xF8,
		0x3F,0x18,0x30,0xF8,0x00,0x1F,0x01,0xF0
};

#define iconBattery_CNT 3
#define iconBattery_H 8	//8x8 = 64
const uint8_t iconBattery24x8[] = {
		0x3F,0xFF,0x80,
		0x20,0x00,0x80,
		0xED,0xB6,0x80,
		0xAD,0xB6,0x80,
		0xAD,0xB6,0x80,
		0xED,0xB6,0x80,
		0x20,0x00,0x80,
		0x3F,0xFF,0x80
};

/******************************** 8x10 pixels icons ********************************/
#define icon8x10_W 1
#define icon8x10_H 10
//%
const uint8_t icon8x10Percent[] = {
		0xE0,0xA1,0xE2,0x04,0x08,0x10,0x20,0x47,0x85,0x07
};

//℃
const uint8_t icon8x10DegreeC[] = {
		0xE0,0xA7,0xE8,0x08,0x08,0x00,0x08,0x08,0x08,0x07
};

//℉
const uint8_t icon8x10DegreeF[] = {
		0xE0,0xA7,0xE8,0x08,0x08,0x07,0x08,0x08,0x08,0x00
};

/******************************** 8x14 pixels icons ********************************/
#define icon8x14_W 1
#define icon8x14_H 14
//%
const uint8_t icon8x14Percent[] = {
		0x61,0x92,0x92,0x64,0x04,0x08,0x08,0x10,0x10,0x20,0x26,0x49,0x49,0x86
};

//℃
const uint8_t icon8x14DegreeC[] = {
		0x60,0x90,0x90,0x60,0x00,0x0E,0x11,0x20,0x20,0x20,0x20,0x20,0x11,0x0E
};

//℉
const uint8_t icon8x14DegreeF[] = {
		0x60,0x90,0x90,0x60,0x00,0x1F,0x20,0x20,0x20,0x3F,0x20,0x20,0x20,0x20
};

/******************************** other icons ********************************/

//逐帧风扇旋转动画，一共5帧
const uint8_t icon16x16Fan[5][32] = {
		{
				0xFF,0xFF,
				0xFF,0xFF,
				0x3F,0xFF,
				0x1F,0xFF,
				0x1F,0xFF,
				0x3F,0xFF,
				0x7F,0xE7,
				0x3F,0xC2,
				0x01,0xC0,
				0x21,0xFE,
				0x73,0xFF,
				0x7F,0xFE,
				0x7F,0xFC,
				0x7F,0xFC,
				0x7F,0xFE,
				0xFF,0xFF},/*"FAN16x16_1.bmp",0*/
		{0xFF,0xFF,0xFF,0xFF,0xDF,0xFF,0x8F,0xFF,0x8F,0xFF,0x8F,0xE3,0x9F,0xC1,0x3F,0xE0,0x3F,0xFE,0x03,0xFE,0xC1,0xFC,0xE3,0xF8,0xFF,0xF8,0xFF,0xF8,0xFF,0xFD,0xFF,0xFF},/*"FAN16x16_2.bmp",0*/
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xEF,0xFF,0xC7,0xF3,0x87,0xE1,0x8F,0xF1,0x3F,0xF8,0x3F,0xFE,0x0F,0xFE,0xC7,0xF8,0xC3,0xF0,0xE7,0xF1,0xFF,0xFB,0xFF,0xFF,0xFF,0xFF},/*"FAN16x16_3.bmp",0*/
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xF9,0xF7,0xF0,0xE3,0xF8,0xC3,0xFC,0x07,0xFE,0x3F,0xFE,0x3F,0xF0,0x9F,0xE1,0x8F,0xE3,0x87,0xF7,0xCF,0xFF,0xFF,0xFF,0xFF,0xFF},/*"FAN16x16_4.bmp",0*/
		{0xFF,0xFF,0xFF,0xFF,0xFF,0xFC,0x7F,0xFC,0x7F,0xFC,0x7F,0xFE,0xF1,0xFE,0x01,0xFE,0x23,0xE2,0x3F,0xC0,0xBF,0xC7,0x3F,0xFF,0x1F,0xFF,0x1F,0xFF,0x9F,0xFF,0xFF,0xFF} /*"FAN16x16_5.bmp",0*/
};


#endif	/* __ICONS_H */



//这么多const bitmap，若没有用到会不会占用内存？
/*
 * 在C里，const常量总是会分配内存，位于只读数据段
 * 在C++，如果const常量在没有声明为extern，那么就是一个编译时的符号，不占用内存。
 * 这个叫常量折叠。 C++编译器会尽量避免分配const常量的内存。
 * 如果你写const int p=a*2; 那么p和a都不会占用内存，就是将它当作编译时的符号。
 * 不过你一旦使用extern外部引用，或者&解地址等操作时，就会迫使C++编译器给这个const变量分配内存
 * C++ 宏定义 #define 和常量 const 的区别
 *
 */
