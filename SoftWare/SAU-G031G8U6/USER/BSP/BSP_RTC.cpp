/*
 * BSP_RTC.cpp
 *
 *  Created on: 2022年6月24日
 *      Author: OldGerman
 */

#include "BSP.h"
#include  "RTClib.h"
#include "CustomPage.hpp"

#ifndef DBG_PRINT_RTC
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PRINT_RTC usb_printf
#else
	#define DBG_PRINT_RTC(...)
	#endif
#endif

#if RTC_IC_PCF212x
RTC_PCF212x rtc(&FRToSI2C1);
#elif RTC_IC_PCF8563
RTC_PCF8563 rtc(&FRToSI2C1);
#endif

/*
 *	判断RTC闹钟中断即将来临为ture而进行比较的最小时间，单位s
 *	这时间由"估计长按电源键进入休眠模式的时间"和"估计按电源键从休眠模式唤醒到开始任务调度的时间"相加得到
 *	关于这个时间如何取，请见我写的《低功耗设计笔记》的 "估计长按电源键进入休眠模式的时间" 和 "估计按电源键从休眠模式唤醒到开始任务调度的时间" 部分
 */
const uint8_t RTC_AlarmWillTrigger_MiniumSecond = 5;

//RTC_PCF8563 rtc;
DateTime now;	//now变量即作为打印时间的变量，也作为串口修改的时间字符串存储的变量

DateTime& RTC_GetNowDateTime(){
	return now;
}

uint8_t RTC_GetNowSceond(){
	return now.second();
}

void RTC_Init()
{
	//如果是STOP1模式恢复过来，那么3.3V断过电，但RTC是没有断VBAT电源的，之前的配置仍然有效，所以不需要重新初始化
	//从STOP1模式恢复过来时，
	if(recoverFromSTOP1 == false){
	  if (!rtc.begin()){
		DBG_PRINT_RTC("Couldn't find RTC");

		rtc.setIntAlarm(RESET);	//关闭Alarm中断
		rtc.clearFlagAlarm();	//清除Alarm Flag
	  }
	}
//	输出32.768KHz方波测试，注意需要外部上拉电阻
#if RTC_IC_PCF212x
//	rtc.writeSqwPinMode(PCF212x_SquareWave32768Hz);
#elif RTC_IC_PCF8563
//	rtc.writeSqwPinMode(PCF8563_SquareWave32kHz);
#endif

#if 0
  if (rtc.lostPower()) {
    DBG_PRINT_RTC("RTC source clock is running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
//	    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    rtc.adjust(DateTime(SECONDS_FROM_1970_TO_2000));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
#endif
}

void RTC_Update() {
	if(USART_DateTimeUpdated())
	{
		uintDateTime dt = USART_GetDateTime();

		rtc.adjust(DateTime(
			dt.yOff + 2000U,//补上2000年
			dt.m,
			dt.d,
			dt.hh,
			dt.mm,
			dt.ss
		));
	}
	DBG_PRINT_RTC("%d/%d/%d %d:%d:%d\r\n",
			//2022/12/31 (Wednesday) 21:45:32
			//最多31个ASCII字符 + "\r\n\0" 是34个
			dt.yOff,
			dt.m,
			dt.d,
			dt.hh,
			dt.mm,
			dt.ss
	);

	now = rtc.now();
	DBG_PRINT_RTC("%d/%d/%d (%s) %d:%d:%d\r\n",
			//2022/12/31 (Wednesday) 21:45:32
			//最多31个ASCII字符 + "\r\n\0" 是34个
			now.year(),
			now.month(),
			now.day(),
			daysOfTheWeek[now.dayOfTheWeek()],
			now.hour(),
			now.minute(),
			now.second()
	);
	DBG_PRINT_RTC(" since midnight 1/1/1970 = %d s = %dd", now.unixtime(), now.unixtime() / 86400L);

	// calculate a date which is 7 days, 12 hours, 30 minutes, 6 seconds into the future
	DateTime future (now + TimeSpan(7,12,30,6));
	DBG_PRINT_RTC(" now + 7d + 12h + 30m + 6s: %d/%d/%d %d:%d:%d",
			future.year(),
			future.month(),
			future.day(),
			future.hour(),
			future.minute(),
			future.second()
	);
}

/**
 * @brief 检查uintDateTime合理性
 * @param  uintDateTime*
 * @retval bool
 */
bool RTC_CheckUintDateTime(uintDateTime *dt) {
	bool CheckNumRange = true;	//判断是否在有效范围
    //检查时间范围有效性
	uint16_t year = dt->yOff;
    uint8_t month = dt->m;
    uint8_t date =  dt->d;

    //判断时分秒范围
    if(((0 <= dt->hh && dt->hh <= 23) &&
	   (0 <= dt->mm && dt->mm <= 59) &&
	   (0 <= dt->ss && dt->ss <= 59))) {
		if(0 <= year && year <= 99){														//年份是否有效
			if (1 <= month && month <= 12) { 												//月份是否有效
				if(month==4 || month==6 || month==9 || month==11) { 						//是否为非2月的小月
					if(!(1 <= date && date <= 30)) CheckNumRange = false;					//小月最多30天
				}
				else {																		//为2月或大月
						if(month==2) {														//检查2月
							year += 2000U;													//补上2000年
							//判断闰年 （四年一闰，百年不闰) || 四百年在闰年
							if((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) { 	//是闰年
								if(!(1 <= date && date <= 29))	CheckNumRange = false;		//检查2月日期（闰年 = 29天）
							}else{															//非闰年
								if(!(1 <= date && date <= 28))	CheckNumRange = false; 		//检查2月日期（非闰年 = 28天）
							}
						}
						else { 																//检查大月
							if(!(1 <= date && date <= 31)) CheckNumRange = false;			//日期是否在大月范围
						}
					}
				}else CheckNumRange = false; 												//月份无效
			}else CheckNumRange = false; 													//年份无效
	}else CheckNumRange = false;															//时分秒无效
    return CheckNumRange;
}

/**
 * @brief 检查RTC闹钟是否即将触发
 * @param  None
 * @retval bool
 */
bool RTC_AlarmWillTrigger(){
	DateTime alarmDateTime = getScheduleSetting_NextDateTime();	//得到下次任务时间
	//difference可能为负值
	int32_t difference = alarmDateTime.unixtime() - now.unixtime();
	//下次任务时间要大于当前时间才会有闹钟中断产生
	if(difference > 0){
		if(difference <= RTC_AlarmWillTrigger_MiniumSecond)
			return true;
	}

	return false;
}
