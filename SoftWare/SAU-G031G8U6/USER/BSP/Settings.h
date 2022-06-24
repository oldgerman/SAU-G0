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
#include "RTClib.h"
//#include "RTClib.h" 	//提供uintDateTime类型
#ifdef __cplusplus
extern "C" {
#include <stdint.h>
extern uint16_t settings_page[20];//暂时划分一个页(2KB)用于存储

/* 默认设置值 */
#define Sec24H		86400		//一天的秒数
//固件版本
#define FW_VERSION_yOff 22U		// 2022
#define FW_VERSION_m 6U		// 06
#define FW_VERSION_d 3U		// 01
#define FW_VERSION_v 10U		// 10 版本号v1.0
//数据采集
	//待补充
//日期时间：%Y-%m-%d %H:%M:%S  yyyy-mm-dd hh:mm:ss  2018-02-21 12:00:00
#define DATE_TIME_yOff	22U	//22年 需要+2000，注意特别将yyyy写为yy以区分
#define DATE_TIME_m		1U		//1月
#define DATE_TIME_d		1U		//1日
#define DATE_TIME_hh	0U		//0时
#define DATE_TIME_mm	0U		//0分
#define DATE_TIME_ss	0U		//0秒
//显示设置
#define SCREEN_BRIGHTNESS 50U	//50% 亮度
#define PW_ON_SHOW_LOGO 1U		//true 开机显示logo
//熄屏唤醒
#define SENSITIVITY		10U		//设置默认触发阈值208mg , FS=2g, 1LSb = 16mg, 208mg = 13LSb, INT1_THS bit[0:6] 0~127, 13/127*100% 约 10%
#define SLEEP_EN		1U		//true 开启休眠
#define SLEEP_TIME		60U		//60S 后进入休眠

#define sysBits 0
#define colBits 1
typedef struct fw_version {
	uint8_t yOff;
	uint8_t m;
	uint8_t d;
	uint8_t v;
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


typedef union{
	uint16_t uint_16;
	uint8_t ctrl[2];
}byteX2Type;

struct orgData{
	//数据采集
	byteX2Type NumDataCollected;	// 已采集的数据组个数(也用于下次写EEPROM地址的指针偏移)，放在第一个好操作
	//版本信息
	fwVersionType FWversion;
	//累计运行时间
	uint32_t TimeRUN;				// 累计运行时间--RUN
	uint32_t TimeLPW_RUN;			// 累计运行时间--LPW_RUN
	uint32_t TimeSTOP1;				// 累计运行时间--STOP1
	//数据采集
	uint16_t NumDataWillCollect;	// 将采集的数据组个数
	uint16_t NumDataSamples;		// 每个周期采集样本数
	uint16_t NumDataOneDay;			// 每天次数//1~8640//24小时1次~10秒1次
	//任务开始日期
	uintDateTime dtStartCollect;
	//本次根据任务采集周期计算的下次采集的闹钟时间，闹钟模式：匹配秒、分、时
	//自动计算出，不需要在Colum的AutoVlaue中设置，可以用uint8_t
	uint8_t Ahh;
	uint8_t Amm;
	uint8_t Ass;

	//显示设置
	uint16_t ScreenBrightness;	// 0~100% 屏幕亮度
	//熄屏唤醒
	uint16_t Sensitivity;     	// 0~100% 动作阈值, 0表示关闭
	uint16_t SleepTime;     	// 0~999S 亮屏时间, 0表示关闭
	//开关标志
	settingsBitsType settingsBits[2];
	//锂电池电压
	uint16_t batVoltage100;	//满电电压
	uint16_t batVoltage0;	//保护电压
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
extern uintDateTime dtSys;
void saveSettings();
bool restoreSettings();
uint8_t lookupVoltageLevel();
uint16_t lookupHallEffectThreshold();
void calibrationReset();
void resetSettings();
void resetDataCollectSettings();
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

