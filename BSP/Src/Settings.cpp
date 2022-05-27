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
//#include "flash.h"
#include "string.h"
#include "BSP.h"

/*这个变量指定.ld文件分配了Flash地址，程序运行时由HAL Flash API擦写它*/
//uint16_t settings_page[512] __attribute__((section(".settings_page")));
systemSettingsType systemSettings;
eepromSettingsType eepromSettings;

void saveSettings() {
	// First we erase the flash
//  flash_save_buffer((uint8_t *)&systemSettings, sizeof(systemSettingsType));
}

/*将Flash settings_page区的值读给RAM中的systemSettings，用于恢复设置值*/
bool restoreSettings() {
	// We read the flash
	/**
	 * 整片Flash擦除后，首次烧入BOOTLDR，和用户APP后，
	 * 0x0800C000起的setting_page未储存数据，每一个16bit块值为0xFFFF
	 * 这时，从0x0800C000读到RAM中systemSettings的所有数据成员值为FF...
	 */
//	flash_read_buffer((uint8_t *)&systemSettings, sizeof(systemSettingsType));
#if	1
	// if the version is correct were done
	// if not we reset and save
	/*
	 * 那么判断满足，强制将内存块的systemSettings值恢复为默认值，并写入默认值到0x0800C000
	 * 下一次时会加载0x0800C000保存过一次的值，不会再次resetSettings();
	 */
	if (systemSettings.FWversion != FW_VERSION) {
		resetSettings();
//    resetWatchdog();
		saveSettings();
		return true;
	}
#endif
	return false;
}

void resetSettings() {
//	memset((void *)&systemSettings, 0, sizeof(systemSettingsType));
//	memset((void *)&eepromSettings, 0, sizeof(eepromSettingsType));	//暂时都置为0

	systemSettings.FWversion 		= FW_VERSION;
	systemSettings.yy 				= DATE_TIME_yy;
	systemSettings.MM 				= DATE_TIME_mm;
	systemSettings.dd				= DATE_TIME_dd;
	systemSettings.hh 				= DATE_TIME_hh;
	systemSettings.mm 				= DATE_TIME_mm;
	systemSettings.ss 				= DATE_TIME_ss;
	//显示设置
	systemSettings.ScreenBrightness = SCREEN_BRIGHTNESS;
	systemSettings.PowerOnShowLogo 	= PW_ON_SHOW_LOGO;
	//熄屏唤醒
	systemSettings.Sensitivity 		= SENSITIVITY;
	systemSettings.SleepEn 			= SLEEP_EN;
	systemSettings.SleepTime 		= SLEEP_TIME;


	//大于1byte，需要union处理：
	eepromSettings.TimeRUN					= 0;				// 累计运行时间--RUN
	eepromSettings.TimeLPW_RUN				= 0;				// 累计运行时间--LPW_RUN
	eepromSettings.TImeSTOP1				= 0;				// 累计运行时间--STOP1
	eepromSettings.NumOfDataCollected		= 0;	// 已采集的数据组个数(也用于下次写EEPROM地址的指针偏移)
	eepromSettings.NumOfDataWillCollect		= 0;	// 将采集的数据组个数
	//任务开始日期
	eepromSettings.STyy						= 22;
	eepromSettings.STMM						= 1;
	eepromSettings.STdd						= 1;
	eepromSettings.SThh						= 0;
	eepromSettings.STmm						= 0;
	eepromSettings.STss						= 0;
	//任务采集周期（+ 任务开始日期，可以配合RTClib的opertor算出结束日期）
	eepromSettings.Thh						= 0;	//>24小时后, 换算为天
	eepromSettings.Tmm						= 0;
	eepromSettings.Tss						= 0;
	//每个周期采集样本数（给滤波器的处理为一组数据，不会存未经滤波的多个数据组）
	eepromSettings.TSamples					= 1;						//暂时不支持单独设置某一对象的样本数
	//采集对象
	eepromSettings.BinCodeOfEnCollect		= B00000111; //使用1byte位数组充当8个bool类型
											// 76543210
											// bit[0]: （LSB）RTC时间完整性标志位
											// bit[1]: 温湿度计--温度
											// bit[2]: 温湿度计--湿度
											// bit[3]: 大气压计--气压（例如BME280或MS5611）
											// bit[4]: 环境光传感器--光照度
											// bit[5]: 电池电压（等价于采集电量百分比）
											// bit[6]: 本次采集周期的运行时间--LPW_RUN
											// bit[7]: （MSB）使能采集任务, 默认不使能
}
