/*
 * Settings.c
 *
 *  Created on: 29 Sep 2016
 *      Author: Ralim
 *		Modify: OldGerman
 *      This file holds the users settings and saves / restores them to the
 * devices flash
 */


#include "Settings.h"
#include "string.h"
#include "BSP.h"

/*这个变量指定.ld文件分配了Flash地址，程序运行时由HAL Flash API擦写它*/
//uint16_t settings_page[512] __attribute__((section(".settings_page")));
systemStorageType systemSto;
EE24 ee24(&hi2c1, 2, 8, _EEPROM_ADDRESS, 100);
void saveSettings() {
//	if(ee24_isConnected())
	ee24.writeBytes(0, systemSto.ctrl, sizeof(systemStorageType));

}

/*将Flash settings_page区的值读给RAM中的systemSto.data，用于恢复设置值*/
bool restoreSettings() {
#if	0
	// if the version is correct were done
	if (systemSto.data.FWversion != FW_VERSION) {
		resetSettings();
		resetWatchdog();
		saveSettings();
		return true;
	}
	else
		return false
#else
//	if(ee24_isConnected())
	ee24.readBytes(0, systemSto.ctrl, sizeof(systemStorageType));
	return true;
#endif
}

void resetDataCollectSettings(){
	systemSto.data.NumDataCollected			= 0;	// 	已采集的数据组个数(也用于下次写EEPROM地址的指针偏移)
	systemSto.data.NumDataWillCollect		= 0;	// 	将采集的数据组个数
	systemSto.data.NumDataSamples			= 1;	//	每个周期采集样本数（给滤波器的处理为一组数据，不会存未经滤波的多个数据组）//暂时不支持单独设置某一对象的样本数
	systemSto.data.NumDataOneDay			= 1;	//	每天多少次
	//任务开始日期
	systemSto.data.dtStartCollect.yOff		= 22;
	systemSto.data.dtStartCollect.m			= 1;
	systemSto.data.dtStartCollect.d			= 1;
	systemSto.data.dtStartCollect.hh			= 0;
	systemSto.data.dtStartCollect.mm			= 0;
	systemSto.data.dtStartCollect.ss			= 0;
	//开关标志
	systemSto.data.settingsBits[colBits].ctrl	= B00000000;
												// 76543210
												// bit[0]: （LSB）RTC振荡器停止标志位OSF，这个一旦变为1就只能手动清除
												// bit[1]: 温湿度计--温度
												// bit[2]: 温湿度计--湿度
												// bit[3]: 大气压计--气压（例如BME280或MS5611）
												// bit[4]: 环境光传感器--光照度
												// bit[5]: 电池电压（等价于采集电量百分比）
												// bit[6]: 预留
												// bit[7]: （MSB）使能采集任务, 默认不使能
}
/**
 * 恢复出厂设置
 * 但，并不会清除历史采集数据，只有在数据采集--清除记录里才能清除
 */
void resetSettings() {
//	memset((void *)&systemSto.ctrl, 0, sizeof(systemStorageType));

	systemSto.data.FWversion.yOff 		= FW_VERSION_yOff;
	systemSto.data.FWversion.m 		= FW_VERSION_m;
	systemSto.data.FWversion.d 		= FW_VERSION_d;
	systemSto.data.FWversion.v 		= FW_VERSION_v;

	systemSto.data.dtSys.yOff 	= DATE_TIME_yOff;
	systemSto.data.dtSys.m 		= DATE_TIME_m;
	systemSto.data.dtSys.d		= DATE_TIME_d;
	systemSto.data.dtSys.hh 	= DATE_TIME_hh;
	systemSto.data.dtSys.mm		= DATE_TIME_mm;
	systemSto.data.dtSys.ss		= DATE_TIME_ss;
	//显示设置
	systemSto.data.ScreenBrightness = SCREEN_BRIGHTNESS;
	//熄屏唤醒
	systemSto.data.Sensitivity 		= SENSITIVITY;
	systemSto.data.SleepTime 		= SLEEP_TIME;
	//大于1byte，需要union处理：
	systemSto.data.TimeRUN					= 0;				// 累计运行时间--RUN
	systemSto.data.TimeLPW_RUN				= 0;				// 累计运行时间--LPW_RUN
	systemSto.data.TimeSTOP1				= 0;				// 累计运行时间--STOP1
	//开关标志
	systemSto.data.settingsBits[sysBits].ctrl	= B00000111;
												// 76543210
												// bit[0]: 显示开机logo
												// bit[1]: 启用休眠
												// bit[2]: 显示运动检测白点
												// bit[3]: 显示年月日
												// bit[4]: 显示任务进度[主屏的粗白条改成bash那种进度条]
												// bit[5]: 显示电量百分比
												// bit[6]: 显示风扇
												// bit[7]:
	resetDataCollectSettings();
}
