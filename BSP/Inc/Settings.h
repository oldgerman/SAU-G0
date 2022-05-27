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
//固件版本
#define FW_VERSION 100U	//BIN
//数据采集
	//待补充
//日期时间：%Y-%m-%d %H:%M:%S  yyyy-mm-dd hh:mm:ss  2018-02-21 12:00:00
#define DATE_TIME_yy	0U	//22年 需要+2000，注意特别将yyyy写为yy以区分
#define DATE_TIME_MM	1U	//1月
#define DATE_TIME_dd	1U	//1日
#define DATE_TIME_hh	0U	//0时
#define DATE_TIME_mm	0U	//0分
#define DATE_TIME_ss	0U	//0秒
//显示设置
#define SCREEN_BRIGHTNESS 50U	//50% 亮度
#define PW_ON_SHOW_LOGO 1U		//true 开机显示logo
//熄屏唤醒
#define SENSITIVITY		5U		//5%
#define SLEEP_EN		1U		//true 开启休眠
#define SLEEP_TIME		60U		//60S 后进入休眠

/*
 * 用于储存不经常修改的Uni-Sensor设置信息，存在STM32片内Flash
 * 此结构必须是2bytes(16bit)的倍数(例如uint16_t int16_t float)，因为它是以uint16_t块的形式在flash里保存/加载
 * This struct must be a multiple of 2 bytes as it is saved / restored from
 * flash in uint16_t chunks
 */
typedef struct {
	//版本信息
	uint16_t FWversion;
	//数据采集
		//没有需要存储在Flash里的数据;
	//日期时间
	uint16_t yy;	//0~99
	uint16_t MM;
	uint16_t dd;
	uint16_t hh;
	uint16_t mm;
	uint16_t ss;
	//显示设置
	uint16_t ScreenBrightness;	// 0~100% 屏幕亮度
	uint16_t PowerOnShowLogo;	// bool	  开机logo
	//熄屏唤醒
	uint16_t Sensitivity;     	// 0~100% 动作阈值
	uint16_t SleepEn;			// bool 启用休眠
	uint16_t SleepTime;     	// 0~999S 亮屏时间
	//辅助功能
		//没有需要存储在Flash里的数据;
} systemSettingsType;

/*
 * 用于储存频繁擦写的数据采集信息，存在片外 EEPROM
 * 此结构必须是1bytes(16bit)的类型，若单个类型大于1byte需要使用union进行特殊转换操作
 */
typedef struct {
	//大于1byte，需要union处理：
	uint32_t TimeRUN;				// 累计运行时间--RUN
	uint32_t TimeLPW_RUN;			// 累计运行时间--LPW_RUN
	uint32_t TImeSTOP1;				// 累计运行时间--STOP1
	uint16_t NumOfDataCollected;	// 已采集的数据组个数(也用于下次写EEPROM地址的指针偏移)
	uint16_t NumOfDataWillCollect;	// 将采集的数据组个数
	//任务开始日期
	uint8_t STyy;
	uint8_t STMM;
	uint8_t STdd;
	uint8_t SThh;
	uint8_t STmm;
	uint8_t STss;
	//任务采集周期（+ 任务开始日期，可以配合RTClib的opertor算出结束日期）
	uint8_t Thh;	//>24小时后, 换算为天
	uint8_t Tmm;
	uint8_t Tss;
	//每个周期采集样本数（给滤波器的处理为一组数据，不会存未经滤波的多个数据组）
	uint8_t TSamples;						//暂时不支持单独设置某一对象的样本数
	//采集对象
	uint8_t BinCodeOfEnCollect;				//B00000000; 使用1byte位数组充当8个bool类型
											// 76543210
											// bit[0]: （LSB）RTC时间完整性标志位
											// bit[1]: 温湿度计--温度
											// bit[2]: 温湿度计--湿度
											// bit[3]: 大气压计--气压（例如BME280或MS5611）
											// bit[4]: 环境光传感器--光照度
											// bit[5]: 电池电压（等价于采集电量百分比）
											// bit[6]: 本次采集周期的运行时间--LPW_RUN
											// bit[7]: （MSB）使能采集任务, 默认不使能
} eepromSettingsType;

extern systemSettingsType systemSettings;
extern eepromSettingsType eepromSettings;
void saveSettings();
bool restoreSettings();
uint8_t lookupVoltageLevel();
uint16_t lookupHallEffectThreshold();
void calibrationReset();
void resetSettings();
#endif
#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */

/*
 * 宏常量，数字后面U， L， F的意思
 * U：unsigned int;
 * L：long
 * F：float
 */

