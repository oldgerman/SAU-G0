/*
 * Settings.h
 *
 *  Created on: 29 Sep 2016
 *      Author: Ralim
 *		Modify: OldGerman
 *      Houses the system settings and allows saving / restoring from flash
 *
 *      settings_page[]需要通过编译宏和.ld文件的设置，将STM32 Flash的一个最小读写粒度(页或扇区)单独分配用于存储它
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_
#include "main.h"
#include "Binary.h"
//#include "RTClib.h" 	//提供uintDateTime类型
#ifdef __cplusplus
extern "C" {
#include <stdint.h>
extern uint16_t settings_page[20];//暂时划分一个页(2KB)用于存储

/* 默认设置值 */
#define Sec24H		86400		//一天的秒数
//固件版本
#define FW_VERSION_yy 2022U		// 2022
#define FW_VERSION_mm 6U		// 06
#define FW_VERSION_dd 1U		// 01
#define FW_VERSION_vv 10U		// 10 版本号v1.0
//数据采集
	//待补充
//日期时间：%Y-%m-%d %H:%M:%S  yyyy-mm-dd hh:mm:ss  2018-02-21 12:00:00
#define DATE_TIME_yy	2022U	//22年 需要+2000，注意特别将yyyy写为yy以区分
#define DATE_TIME_MM	1U		//1月
#define DATE_TIME_dd	1U		//1日
#define DATE_TIME_hh	0U		//0时
#define DATE_TIME_mm	0U		//0分
#define DATE_TIME_ss	0U		//0秒
//显示设置
#define SCREEN_BRIGHTNESS 50U	//50% 亮度
#define PW_ON_SHOW_LOGO 1U		//true 开机显示logo
//熄屏唤醒
#define SENSITIVITY		5U		//5%
#define SLEEP_EN		1U		//true 开启休眠
#define SLEEP_TIME		60U		//60S 后进入休眠

#define sysBits 0
#define colBits 1
typedef struct fw_version {
	uint16_t yy;
	uint8_t mm;
	uint8_t dd;
	uint8_t vv;
}fwVersionType;


struct settings_Bits{
	uint8_t bit0 	:1;
	uint8_t bit1 	:1;
	uint8_t bit2 	:1;
	uint8_t bit3 	:1;
	uint8_t bit4 	:1;
	uint8_t bit5 	:1;
	uint8_t bit6 	:1;
	uint8_t bit7 	:1;
};

typedef union{
	settings_Bits bits;
	uint8_t ctrl;		//Colum对象成员prBits和mask修改bits时使用
}settingsBitsType;

struct orgData{
	//版本信息
	fwVersionType FWversion;
	//数据采集
	uint32_t TimeRUN;				// 累计运行时间--RUN
	uint32_t TimeLPW_RUN;			// 累计运行时间--LPW_RUN
	uint32_t TimeSTOP1;				// 累计运行时间--STOP1
	uint16_t NumDataCollected;		// 已采集的数据组个数(也用于下次写EEPROM地址的指针偏移)
	uint16_t NumDataWillCollect;	// 将采集的数据组个数
	uint16_t NumDataSamples;		// 每个周期采集样本数
	uint16_t NumDataOneDay;			// 每天次数//1~8640//24小时1次~10秒1次
	//任务开始日期
	uint16_t STyy;
	uint16_t STMM;
	uint16_t STdd;
	uint16_t SThh;
	uint16_t STmm;
	uint16_t STss;
	//本次根据任务采集周期计算的下次采集的闹钟时间，闹钟模式：匹配秒、分、时
	//自动计算出，不需要在Colum的AutoVlaue中设置，可以用uint8_t
	uint8_t Ahh;
	uint8_t Amm;
	uint8_t Ass;
	//日期时间
	uint16_t yy;	//0~99
	uint16_t MM;
	uint16_t dd;
	uint16_t hh;
	uint16_t mm;
	uint16_t ss;
	//显示设置
	uint16_t ScreenBrightness;	// 0~100% 屏幕亮度
	//熄屏唤醒
	uint16_t Sensitivity;     	// 0~100% 动作阈值
	uint16_t SleepTime;     	// 0~999S 亮屏时间
	//开关标志
	settingsBitsType settingsBits[2];
};

/*
 * systemStorageType
 * 用于片外 EEPROM 储存设置信息和数据采集信息
 */
typedef union {
	struct orgData data;			//运行时使用
	uint8_t ctrl[sizeof(orgData)];	//向eeprom读写时使用
} systemStorageType;


extern systemStorageType systemSto;

void saveSettings();
bool restoreSettings();
uint8_t lookupVoltageLevel();
uint16_t lookupHallEffectThreshold();
void calibrationReset();
void resetSettings();
#endif
#ifdef __cplusplus
#include "ee24.hpp"
extern EE24 ee24;
}
#endif

#endif /* SETTINGS_H_ */

/*
 * 宏常量，数字后面U， L， F的意思
 * U：unsigned int;
 * L：long
 * F：float
 */

