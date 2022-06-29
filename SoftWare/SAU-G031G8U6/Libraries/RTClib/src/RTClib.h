/**************************************************************************/
/*!
  @file     RTClib.h

  Original library by JeeLabs http://news.jeelabs.org/code/, released to the
  public domain

  License: MIT (see LICENSE)

  This is a fork of JeeLab's fantastic real time clock library for Arduino.

  For details on using this library with an RTC module like the DS1307, PCF8523,
  or DS3231, see the guide at:
  https://learn.adafruit.com/ds1307-real-time-clock-breakout-board-kit/overview

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  @Modify OldGerman
  	  2022/05/19
  	  	  匹配STM32 HAL库，可以进行RTC时间设置，但使用掩码更改寄存器指定位的函数存在地址错位
  	  2022/05/31
  	  	  RTC的地址指针主动自增的特性会导致I2C_Wrapper的bit和bits读写函数产生地址移位的Bug，
  	  	  因为这几个函数最终都会调用HAL_I2C_MemWrite和HAL_I2C_MemRead，于是在父类RTC_I2C内
  	  	  重写了read_bit、read_bits、write_bit、write_bits...最终都会调用用Tramsmit和Recieve
*/
/**************************************************************************/

#ifndef _RTCLIB_H_
#define _RTCLIB_H_

#include "stdio.h" 		//提供sprintf()
#include "Arduino.h"	//来源于FAST_SHIFT适配STM32的arduinoAPI，但经过我裁剪和修改，提供某些东西
#include "RTC_defines.h"
#include "stdint.h"
#include "I2C_Wrapper.h"
#ifdef __cplusplus
class TimeSpan;
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/

/* 私有包含 Private includes -------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* 导出类型 Exported types ---------------------------------------------------*/
/* USER CODE BEGIN ET */
/*
 * 用uint8_t整形存储时间
 * 例如成员分别为 {22 05 22 18 19 00}	表示 2022年5月22日18点19分00秒
 */
struct uintDateTime{
	  uint16_t yOff; ///< Year offset from 2000
	  uint16_t m;    ///< Month 1-12
	  uint16_t d;    ///< Day 1-31
	  uint16_t hh;   ///< Hours 0-23
	  uint16_t mm;   ///< Minutes 0-59
	  uint16_t ss;   ///< Seconds 0-59
};
/* USER CODE END ET */

/* 导出常量 Exported constants -----------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* 导出的宏 Exported macro ---------------------------------------------------*/
/* USER CODE BEGIN EM */
#define PROGMEM 				//	修改为空宏
#define SECONDS_PER_DAY 86400L ///< 60 * 60 * 24
#define SECONDS_FROM_1970_TO_2000                                              \
  946684800 ///< Unixtime for 2000-01-01 00:00:00, useful for initialization
/* USER CODE END EM */

/* 导出函数原型 Exported functions prototypes --------------------------------*/

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* 私有定义 Private defines --------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}

/**************************************************************************/
/*!
    @brief  Simple general-purpose date/time class (no TZ / DST / leap
            seconds).

    This class stores date and time information in a broken-down form, as a
    tuple (year, month, day, hour, minute, second). The day of the week is
    not stored, but computed on request. The class has no notion of time
    zones, daylight saving time, or
    [leap seconds](http://en.wikipedia.org/wiki/Leap_second): time is stored
    in whatever time zone the user chooses to use.

    The class supports dates in the range from 1 Jan 2000 to 31 Dec 2099
    inclusive.
*/
/**************************************************************************/
class DateTime {
public:
  DateTime(uint32_t t = SECONDS_FROM_1970_TO_2000);
  DateTime(uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0,
           uint8_t min = 0, uint8_t sec = 0);
  DateTime(const DateTime &copy);
  DateTime(const char *date, const char *time);
  DateTime(const __FlashStringHelper *date, const __FlashStringHelper *time);
  DateTime(const char *iso8601date);
  bool isValid() const;
  char *toString(char *buffer) const;

  /*!
      @brief  Return the year.
      @return Year (range: 2000--2099).
  */
  uint16_t year() const { return 2000U + yOff; }
  /*!
      @brief  Return the month.
      @return Month number (1--12).
  */
  uint8_t month() const { return m; }
  /*!
      @brief  Return the day of the month.
      @return Day of the month (1--31).
  */
  uint8_t day() const { return d; }
  /*!
      @brief  Return the hour
      @return Hour (0--23).
  */
  uint8_t hour() const { return hh; }

  uint8_t twelveHour() const;
  /*!
      @brief  Return whether the time is PM.
      @return 0 if the time is AM, 1 if it's PM.
  */
  uint8_t isPM() const { return hh >= 12; }
  /*!
      @brief  Return the minute.
      @return Minute (0--59).
  */
  uint8_t minute() const { return mm; }
  /*!
      @brief  Return the second.
      @return Second (0--59).
  */
  uint8_t second() const { return ss; }

  uint8_t dayOfTheWeek() const;

  /* 32-bit times as seconds since 2000-01-01. */
  uint32_t secondstime() const;

  /* 32-bit times as seconds since 1970-01-01. */
  uint32_t unixtime(void) const;

  /*!
      Format of the ISO 8601 timestamp generated by `timestamp()`. Each
      option corresponds to a `toString()` format as follows:
  */
  enum timestampOpt {
    TIMESTAMP_FULL, //!< `YYYY-MM-DDThh:mm:ss`
    TIMESTAMP_TIME, //!< `hh:mm:ss`
    TIMESTAMP_DATE  //!< `YYYY-MM-DD`
  };
  char* timestamp(timestampOpt opt = TIMESTAMP_FULL) const;

  DateTime operator+(const TimeSpan &span) const;
  DateTime operator-(const TimeSpan &span) const;
  TimeSpan operator-(const DateTime &right) const;
  bool operator<(const DateTime &right) const;

  /*!
      @brief  Test if one DateTime is greater (later) than another.
      @warning if one or both DateTime objects are invalid, returned value is
        meaningless
      @see use `isValid()` method to check if DateTime object is valid
      @param right DateTime object to compare
      @return True if the left DateTime is later than the right one,
        false otherwise
  */
  bool operator>(const DateTime &right) const { return right < *this; }

  /*!
      @brief  Test if one DateTime is less (earlier) than or equal to another
      @warning if one or both DateTime objects are invalid, returned value is
        meaningless
      @see use `isValid()` method to check if DateTime object is valid
      @param right DateTime object to compare
      @return True if the left DateTime is earlier than or equal to the
        right one, false otherwise
  */
  bool operator<=(const DateTime &right) const { return !(*this > right); }

  /*!
      @brief  Test if one DateTime is greater (later) than or equal to another
      @warning if one or both DateTime objects are invalid, returned value is
        meaningless
      @see use `isValid()` method to check if DateTime object is valid
      @param right DateTime object to compare
      @return True if the left DateTime is later than or equal to the right
        one, false otherwise
  */
  bool operator>=(const DateTime &right) const { return !(*this < right); }
  bool operator==(const DateTime &right) const;

  /*!
      @brief  Test if two DateTime objects are not equal.
      @warning if one or both DateTime objects are invalid, returned value is
        meaningless
      @see use `isValid()` method to check if DateTime object is valid
      @param right DateTime object to compare
      @return True if the two objects are not equal, false if they are
  */
  bool operator!=(const DateTime &right) const { return !(*this == right); }

protected:
  uint8_t yOff; ///< Year offset from 2000
  uint8_t m;    ///< Month 1-12
  uint8_t d;    ///< Day 1-31
  uint8_t hh;   ///< Hours 0-23
  uint8_t mm;   ///< Minutes 0-59
  uint8_t ss;   ///< Seconds 0-59
};

/**************************************************************************/
/*!
    @brief  Timespan which can represent changes in time with seconds accuracy.
 	 	 	Timespan可以以秒为单位表示时间变化的时间跨度
    @note	注意时间戳内的数据成员可以为负
*/
/**************************************************************************/
class TimeSpan {
public:
  TimeSpan(int32_t seconds = 0);
  TimeSpan(int16_t days, int8_t hours, int8_t minutes, int8_t seconds);
  TimeSpan(const TimeSpan &copy);

  /*!
      @brief  Number of days in the TimeSpan
              e.g. 4
      @return int16_t days
  */
  int16_t days() const { return _seconds / 86400L; }
  /*!
      @brief  Number of hours in the TimeSpan
              This is not the total hours, it includes the days
              e.g. 4 days, 3 hours - NOT 99 hours
      @return int8_t hours
  */
  int8_t hours() const { return _seconds / 3600 % 24; }
  /*!
      @brief  Number of minutes in the TimeSpan
              This is not the total minutes, it includes days/hours
              e.g. 4 days, 3 hours, 27 minutes
      @return int8_t minutes
  */
  int8_t minutes() const { return _seconds / 60 % 60; }
  /*!
      @brief  Number of seconds in the TimeSpan
              This is not the total seconds, it includes the days/hours/minutes
              e.g. 4 days, 3 hours, 27 minutes, 7 seconds
      @return int8_t seconds
  */
  int8_t seconds() const { return _seconds % 60; }
  /*!
      @brief  Total number of seconds in the TimeSpan, e.g. 358027
      @return int32_t seconds
  */
  int32_t totalseconds() const { return _seconds; }

  TimeSpan operator+(const TimeSpan &right) const;
  TimeSpan operator-(const TimeSpan &right) const;

protected:
  int32_t _seconds;  //  以有符号32位整数存储时间，估计是为了方便运算符重载函数计算小的减去大的时间得到负数后好转换为时差
  	  	  	  	  	 //  从Unix时间戳1970年开始, 约68年溢出 ///< Actual TimeSpan value is stored as seconds
  	  	  	  	  	 //  以无正负号的32位整数（unsigned int32）存储 POSIX 时间，溢出错误会被延后到 2106 年
  	  	  	  	  	 //  65535*65535/3600/24/365 ≈136年
  	  	  	  	  	 //  所以所有类似 time - oldtime 计算差时的
};

/**************************************************************************/
/*!
    @brief  A generic I2C RTC base class. DO NOT USE DIRECTLY
*/
/**************************************************************************/
class RTC_I2C {
public:
	RTC_I2C(FRToSI2C * PtrFRToSI2C, uint8_t I2C_ADD)
		:pFRToSI2C(PtrFRToSI2C), i2cAddr(I2C_ADD)
	{}
	FRToSI2C * pFRToSI2C;
	uint8_t i2cAddr;	//RTC I2C 7bit地址左移一位
	bool ret = 0;			//临时存放I2C API的读写返回状态
protected:
	/*!
      @brief  Convert a binary coded decimal value to binary. RTC stores
    time/date values as BCD.
      @param val BCD value
      @return Binary value
	 */
	static uint8_t bcd2bin(uint8_t val) { return val - 6 * (val >> 4); }
	/*!
      @brief  Convert a binary value to BCD format for the RTC registers
      @param val Binary value
      @return BCD value
	 */
	static uint8_t bin2bcd(uint8_t val) { return val + 6 * (val / 10); }
	//  Adafruit_I2CDevice *i2c_dev = NULL; ///< Pointer to I2C bus interface
	//  I2C_HandleTypeDef 	*hi2c = nullptr;

	void read_byte(uint8_t regAddr, uint8_t *ptrData){
		ret = pFRToSI2C->Master_Transmit(i2cAddr, &regAddr, 1);
		ret &= pFRToSI2C->Master_Receive(i2cAddr, ptrData, 1);
	}
	void write_byte(uint8_t regAddr, uint8_t data){	//注意参数不能为指针，因为要传宏
		uint8_t bytes[2] = { regAddr, data };
		ret = pFRToSI2C->Master_Transmit(i2cAddr, bytes, 2);
	}
	void write_bit(uint8_t regAddr, uint8_t mask, uint8_t data) {//注意参数不能为指针，因为要传宏
	    uint8_t b;
		read_byte(regAddr, &b);
	    b = (data != 0) ? (b | mask) : (b & ~mask);
	    write_byte(regAddr, b);
	}
	void read_bit(uint8_t regAddr, uint8_t mask, uint8_t *pData) {
	    uint8_t b;
	    read_byte(regAddr, &b);
	    *pData = b & mask;
	}
	void read_bits(uint8_t regAddr, uint8_t mask, uint8_t *pData) {
		uint8_t b;
		read_byte(regAddr, &b);
		b &= mask;
		*pData = b;
	}
	void write_bits(uint8_t regAddr, uint8_t mask, uint8_t data) {
	    uint8_t b;
	    read_byte(regAddr, &b);
		data &= mask; // zero all non-important bits in data
		b &= ~(mask); // zero all important bits in existing byte
		b |= data; // combine data with existing byte
		write_byte(regAddr, b);
	}
	void master_transmit(uint8_t *pData, uint16_t length) {
		ret = pFRToSI2C->Master_Transmit(i2cAddr, pData, length);
	}
	void master_receive(uint8_t *pData, uint16_t length) {
		ret = pFRToSI2C->Master_Receive(i2cAddr, pData, length);
	}
	void transmit_receive(uint8_t *pData_tx, uint16_t Size_tx,
						  uint8_t *pData_rx, uint16_t Size_rx)
	{
		master_transmit(pData_tx, Size_tx);
		master_receive(pData_rx, Size_rx);
	}
};

/**************************************************************************/
/*!
    @brief  RTC based on the PCF212x chip connected via I2C and the Wire library
*/
/**************************************************************************/
class RTC_PCF212x : RTC_I2C {
public:
	RTC_PCF212x(FRToSI2C * PtrFRToSI2C, bool isPCF2127 = false)
	:RTC_I2C(PtrFRToSI2C, PCF212x_ADDRESS), is_pcf2127(isPCF2127)
	{}

  bool begin();
  bool adjust(const DateTime &dt);
  bool lostPower(void);
  bool sourceClockRun(void);
  DateTime now();
  Pcf212xSqwPinMode readSqwPinMode();
  bool writeSqwPinMode(Pcf212xSqwPinMode mode);
  bool setTimeAlarm(const DateTime *dt, Pcf212xAlarmMode alarm_mode);
  bool setIntAlarm(bool enable);
  bool clearFlagAlarm();//清除Alarm中断（中断回调函数里设置标记后在主循环里使用）
  bool alarmFired();	//Alarm是否产生警报（以轮询检测中断）
//  void enable32K(void);
//  void disable32K(void);
//  bool isEnabled32K(void);
//  float getTemperature(); // in Celsius degree
  /*!
      @brief  Convert the day of the week to a representation suitable for
              storing in the DS3231: from 1 (Monday) to 7 (Sunday).
      @param  d Day of the week as represented by the library:
              from 0 (Sunday) to 6 (Saturday).
      @return the converted value
  */
  static uint8_t dowToPCF212x(uint8_t d) { return d == 0 ? 7 : d; }
  bool is_pcf2127;
};


/**************************************************************************/
/*!
    @brief  RTC based on the PCF8563 chip connected via I2C and the Wire library
*/
/**************************************************************************/
class RTC_PCF8563 : RTC_I2C {
public:
  RTC_PCF8563(FRToSI2C * PtrFRToSI2C)
	:RTC_I2C(PtrFRToSI2C, PCF8563_ADDRESS){}

  bool begin();
  bool lostPower(void);
  void adjust(const DateTime &dt);
  DateTime now();
  void start(void);
  void stop(void);
  uint8_t isrunning();
  Pcf8563SqwPinMode readSqwPinMode();
  void writeSqwPinMode(Pcf8563SqwPinMode mode);
  bool setTimeAlarm(const DateTime *dt, PCF8563AlarmMode alarm_mode);
  bool setIntAlarm(bool enable);
  bool clearFlagAlarm();
  bool alarmFired();
};

/**************************************************************************/
/*!
    @brief  RTC using the internal millis() clock, has to be initialized before
   use. NOTE: this is immune to millis() rollover events.
*/
/**************************************************************************/
class RTC_Millis {
public:
  /*!
      @brief  Start the RTC
      @param dt DateTime object with the date/time to set
  */
  void begin(const DateTime &dt) { adjust(dt); }
  void adjust(const DateTime &dt);
  DateTime now();

protected:
  /*!
      Unix time from the previous call to now().

      This, together with `lastMillis`, defines the alignment between
      the `millis()` timescale and the Unix timescale. Both variables
      are updated on each call to now(), which prevents rollover issues.
  */
  uint32_t lastUnix;
  /*!
      `millis()` value corresponding `lastUnix`.

      Note that this is **not** the `millis()` value of the last call to
      now(): it's the `millis()` value corresponding to the last **full
      second** of Unix time preceding the last call to now().
  */
  uint32_t lastMillis;
};

/**************************************************************************/
/*!
    @brief  RTC using the internal micros() clock, has to be initialized before
            use. Unlike RTC_Millis, this can be tuned in order to compensate for
            the natural drift of the system clock. Note that now() has to be
            called more frequently than the micros() rollover period, which is
            approximately 71.6 minutes.
*/
/**************************************************************************/
class RTC_Micros {
public:
  /*!
      @brief  Start the RTC
      @param dt DateTime object with the date/time to set
  */
  void begin(const DateTime &dt) { adjust(dt); }
  void adjust(const DateTime &dt);
  void adjustDrift(int ppm);
  DateTime now();

protected:
  /*!
      Number of microseconds reported by `micros()` per "true"
      (calibrated) second.
  */
  uint32_t microsPerSecond = 1000000;
  /*!
      Unix time from the previous call to now().

      The timing logic is identical to RTC_Millis.
  */
  uint32_t lastUnix;
  /*!
      `micros()` value corresponding to `lastUnix`.
  */
  uint32_t lastMicros;
};
#endif





#if 0
//待匹配驱动
/**************************************************************************/
/*!
    @brief  RTC based on the DS1307 chip connected via I2C and the Wire library
*/
/**************************************************************************/
class RTC_DS1307 : RTC_I2C {
public:
  bool begin(FRToSI2C * ptrFRToSI2C);
  void adjust(const DateTime &dt);
  uint8_t isrunning(void);
  DateTime now();
  Ds1307SqwPinMode readSqwPinMode();
  void writeSqwPinMode(Ds1307SqwPinMode mode);
  uint8_t readnvram(uint8_t address);
  void readnvram(uint8_t *buf, uint8_t size, uint8_t address);
  void writenvram(uint8_t address, uint8_t data);
  void writenvram(uint8_t address, const uint8_t *buf, uint8_t size);
};

/**************************************************************************/
/*!
    @brief  RTC based on the DS3231 chip connected via I2C and the Wire library
*/
/**************************************************************************/
class RTC_DS3231 : RTC_I2C {
public:
  bool begin(FRToSI2C * ptrFRToSI2C);
  void adjust(const DateTime &dt);
  bool lostPower(void);
  DateTime now();
  Ds3231SqwPinMode readSqwPinMode();
  void writeSqwPinMode(Ds3231SqwPinMode mode);
  bool setAlarm1(const DateTime &dt, Ds3231Alarm1Mode alarm_mode);
  bool setAlarm2(const DateTime &dt, Ds3231Alarm2Mode alarm_mode);
  void disableAlarm(uint8_t alarm_num);
  void clearAlarm(uint8_t alarm_num);
  bool alarmFired(uint8_t alarm_num);
  void enable32K(void);
  void disable32K(void);
  bool isEnabled32K(void);
  float getTemperature(); // in Celsius degree
  /*!
      @brief  Convert the day of the week to a representation suitable for
              storing in the DS3231: from 1 (Monday) to 7 (Sunday).
      @param  d Day of the week as represented by the library:
              from 0 (Sunday) to 6 (Saturday).
      @return the converted value
  */
  static uint8_t dowToDS3231(uint8_t d) { return d == 0 ? 7 : d; }
};

/**************************************************************************/
/*!
    @brief  RTC based on the PCF8523 chip connected via I2C and the Wire library
*/
/**************************************************************************/
class RTC_PCF8523 : RTC_I2C {
public:
  bool begin(FRToSI2C * ptrFRToSI2C);
  void adjust(const DateTime &dt);
  bool lostPower(void);
  bool initialized(void);
  DateTime now();
  void start(void);
  void stop(void);
  uint8_t isrunning();
  Pcf8523SqwPinMode readSqwPinMode();
  void writeSqwPinMode(Pcf8523SqwPinMode mode);
  void enableSecondTimer(void);
  void disableSecondTimer(void);
  void enableCountdownTimer(PCF8523TimerClockFreq clkFreq, uint8_t numPeriods,
                            uint8_t lowPulseWidth);
  void enableCountdownTimer(PCF8523TimerClockFreq clkFreq, uint8_t numPeriods);
  void disableCountdownTimer(void);
  void deconfigureAllTimers(void);
  void calibrate(Pcf8523OffsetMode mode, int8_t offset);
};


#endif


#endif // _RTCLIB_H_
