/*
 * RTC_defines.h
 *
 *  Created on: May 20, 2022
 *      Author: OldGerman
 */

#ifndef RTCLIB_SRC_RTC_DEFINES_H_
#define RTCLIB_SRC_RTC_DEFINES_H_

#define PCF212x_ADDRESS 0x51 << 1   ///< I2C address for PCF212x
#define PCF8563_ADDRESS 0x51 << 1   ///< I2C address for PCF8563

//这些enum都是对应设置位段的值
/** DS1307 SQW pin mode settings */
enum Ds1307SqwPinMode {
  DS1307_OFF = 0x00,            // Low
  DS1307_ON = 0x80,             // High
  DS1307_SquareWave1HZ = 0x10,  // 1Hz square wave
  DS1307_SquareWave4kHz = 0x11, // 4kHz square wave
  DS1307_SquareWave8kHz = 0x12, // 8kHz square wave
  DS1307_SquareWave32kHz = 0x13 // 32kHz square wave
};

/** DS3231 SQW pin mode settings */
enum Ds3231SqwPinMode {
  DS3231_OFF = 0x1C,            /*0001,1100*< Off */
  	  	  	  	  	  	  	  	//    	^使能位
  DS3231_SquareWave1Hz = 0x00,  /*0000,0000*<  1Hz square wave */
  DS3231_SquareWave1kHz = 0x08, /*0000,1000*<  1kHz square wave */
  DS3231_SquareWave4kHz = 0x10, /*0001,0000*<  4kHz square wave */
  DS3231_SquareWave8kHz = 0x18  /*0001,1000*<  8kHz square wave */
};

//DS3231有两个独立闹钟:
//手册Table 2. Alarm Mask Bits(比特掩码)，有两个表，一个Alarm1，一个Alarm2
/** DS3231 Alarm modes for alarm 1 */
enum Ds3231Alarm1Mode {
  DS3231_A1_PerSecond = 0x0F, /* 0000,1111 *< Alarm once per second */
  DS3231_A1_Second = 0x0E,    /* 0000,1110 *< Alarm when seconds match */
  DS3231_A1_Minute = 0x0C,    /* 0000,1100 *< Alarm when minutes and seconds match */
  DS3231_A1_Hour = 0x08,      /* 0000,1000 *< Alarm when hours, minutes
                                   	   	   	  and seconds match */
  DS3231_A1_Date = 0x00,      /* 0000,0000 *< Alarm when date (day of month), hours,
                                              minutes and seconds match */
  DS3231_A1_Day = 0x10        /* 0001,0000*< Alarm when day (day of week), hours,
                                              minutes and seconds match */
};
/** DS3231 Alarm modes for alarm 2 */
enum Ds3231Alarm2Mode {
  DS3231_A2_PerMinute = 0x7, /**< Alarm once per minute
                                  (whenever seconds are 0) */
  DS3231_A2_Minute = 0x6,    /**< Alarm when minutes match */
  DS3231_A2_Hour = 0x4,      /**< Alarm when hours and minutes match */
  DS3231_A2_Date = 0x0,      /**< Alarm when date (day of month), hours
                                  and minutes match */
  DS3231_A2_Day = 0x8        /**< Alarm when day (day of week), hours
                                  and minutes match */
};
/** PCF8523 INT/SQW pin mode settings */
enum Pcf8523SqwPinMode {
  PCF8523_OFF = 7,             /**< Off */
  PCF8523_SquareWave1HZ = 6,   /**< 1Hz square wave */
  PCF8523_SquareWave32HZ = 5,  /**< 32Hz square wave */
  PCF8523_SquareWave1kHz = 4,  /**< 1kHz square wave */
  PCF8523_SquareWave4kHz = 3,  /**< 4kHz square wave */
  PCF8523_SquareWave8kHz = 2,  /**< 8kHz square wave */
  PCF8523_SquareWave16kHz = 1, /**< 16kHz square wave */
  PCF8523_SquareWave32kHz = 0  /**< 32kHz square wave */
};

/** PCF8523 Timer Source Clock Frequencies for Timers A and B */
enum PCF8523TimerClockFreq {
  PCF8523_Frequency4kHz = 0,   /**< 1/4096th second = 244 microseconds,
                                    max 62.256 milliseconds */
  PCF8523_Frequency64Hz = 1,   /**< 1/64th second = 15.625 milliseconds,
                                    max 3.984375 seconds */
  PCF8523_FrequencySecond = 2, /**< 1 second, max 255 seconds = 4.25 minutes */
  PCF8523_FrequencyMinute = 3, /**< 1 minute, max 255 minutes = 4.25 hours */
  PCF8523_FrequencyHour = 4,   /**< 1 hour, max 255 hours = 10.625 days */
};

/** PCF8523 Timer Interrupt Low Pulse Width options for Timer B only */
enum PCF8523TimerIntPulse {
  PCF8523_LowPulse3x64Hz = 0,  /**<  46.875 ms   3/64ths second */
  PCF8523_LowPulse4x64Hz = 1,  /**<  62.500 ms   4/64ths second */
  PCF8523_LowPulse5x64Hz = 2,  /**<  78.125 ms   5/64ths second */
  PCF8523_LowPulse6x64Hz = 3,  /**<  93.750 ms   6/64ths second */
  PCF8523_LowPulse8x64Hz = 4,  /**< 125.000 ms   8/64ths second */
  PCF8523_LowPulse10x64Hz = 5, /**< 156.250 ms  10/64ths second */
  PCF8523_LowPulse12x64Hz = 6, /**< 187.500 ms  12/64ths second */
  PCF8523_LowPulse14x64Hz = 7  /**< 218.750 ms  14/64ths second */
};

/** PCF8523 Offset modes for making temperature/aging/accuracy adjustments */
enum Pcf8523OffsetMode {
  PCF8523_TwoHours = 0x00, /**< Offset made every two hours */
  PCF8523_OneMinute = 0x80 /**< Offset made every minute */
};

/** PCF8563 CLKOUT pin mode settings */
/*
 * PCF8563手册：
 * Table 22. CLKOUT_control - CLKOUT control register (address 0Dh) bit description
 * CLKOUT_control寄存器的设定值的5种可能：
 *
 * 注意与连续读写的 按位与 0x80 区分
 * 例如LIS3DH的注释：
 * //CTRL
	#define LIS_CTRL_REG1     0x20 | 0x80
	// 0x80=10000000b 为了连续读取多个字节，SUB字段的MSb必须为“ 1”，
	 * 会是SUB（寄存器地址）自动增加以允许多次数据读/写。
	 * 而SUB（6-0）代表要读取的第一个寄存器的地址。
 */
enum Pcf8563SqwPinMode {
  PCF8563_SquareWaveOFF = 0x00,  /**< Off */				//0000,0000	//bit[7] = 0 //the CLKOUT output is inhibited and CLKOUT output is set high-impedance
  PCF8563_SquareWave1Hz = 0x83,  /**< 1Hz square wave */	//1000,0011	//bit[7] = 1 //the CLKOUT output is activated
  PCF8563_SquareWave32Hz = 0x82, /**< 32Hz square wave */	//1000,0010	//bit[1:0]
  PCF8563_SquareWave1kHz = 0x81, /**< 1kHz square wave */	//1000,0001
  PCF8563_SquareWave32kHz = 0x80 /**< 32kHz square wave */	//1000,0000
};

/** PCF8563 Alarm mode */
enum PCF8563AlarmMode {
	PCF8563_A_Minute = 0x1C,    // 0001,1100 < 报警：当匹配设定的分
	PCF8563_A_Hour = 0x18,      // 0001,1000 < 报警：当匹配设定的分、时
	PCF8563_A_Date = 0x10,      // 0001,0000 < 报警：当匹配设定的分、时、日期
	PCF8563_A_Day = 0x08,       // 0000,1000 < 报警：当匹配设定的分、时、星期
};
/** PCF2129AT **/
/**
 * 	The register at address 0Fh defines the temperature measurement period and the clock out mode.
 * 																		   ^~~
 *  8.3.2 OTP refresh
 * 	也用这个寄存器！！相比PCF8563， PCF2129在设备的生产和测试期间都经过校准。
 * 	校准参数存储在称为一次性可编程 (OTP) 单元的EPROM 单元中。
 * 	建议在上电且振荡器稳定运行后进行一次 OTP 刷新。
 * 	OTP 刷新只需不到 100 毫秒即可完成。

 *  要执行 OTP 刷新，必须清除位 OTPR（设置为逻辑 0），然后再次设置为逻辑 1
	方波输出在CLKOUT_ctl寄存器, COF[2:0], 相比PCF8563，
	PCF2129AT有内部温度传感器可以获取数据对晶振做补偿，
	但这个温度获取周期也在CLKOUT_ctl寄存器，TCR[1:0]位设置

    8.3.3 Clock output
	A programmable square wave is available at pin CLKOUT. Operation is controlled by the
	COF[2:0] control bits in register CLKOUT_ctl. Frequencies of 32.768 kHz (default) down
	to 1 Hz can be generated for use as system clock, microcontroller clock, charge pump input,
	or for calibrating the oscillator.
	CLKOUT is an open-drain output and enabled at power-on. When disabled, the output is high-impedance.

	8.3.1.1 Temperature measurement
	The PCF2129 has a temperature sensor circuit used to perform the temperature compensation of the frequency.
	The temperature is measured immediately after power-on and then periodically with a period set by the
	temperature conversion rate TCR[1:0] in the register CLKOUT_ctl

 */
//以下bit[2:0]是有效的 CLKOUT frequency selection 设置位码
//但bit[7:6] bit[5]还有另外用途，因此操作时务必与当前设定做一个与运算
enum Pcf212xSqwPinMode {
  PCF212x_SquareWaveOFF    = 0x07,  /**< Off */					//0000,0111
  PCF212x_SquareWave1Hz    = 0x06,  /**< 1Hz square wave */		//0000,0110
  PCF212x_SquareWave1024Hz = 0x05, 	/**< 1024Hz square wave */	//0000,0101
  PCF212x_SquareWave2048Hz = 0x04, 	/**< 2048Hz square wave */	//0000,0100
  PCF212x_SquareWave4096Hz = 0x03, 	/**< 4096Hz square wave */	//0000,0011
  PCF212x_SquareWave8192Hz = 0x02, 	/**< 8192Hz square wave */	//0000,0010
  PCF212x_SquareWave16384Hz = 0x01, /**< 16384Hz square wave */	//0000,0001
  PCF212x_SquareWave32768Hz = 0x00, /**< 32768Hz square wave */	//0000,0000
};																//^^^ other settings

/** Pcf212x Alarm mode */
enum Pcf212xAlarmMode {
//	PCF212x_A_PerSecond = 0x1F, // 0001,1111 < 报警：每秒一次 //DS3231的每秒闹钟的枚举在PCF2129中不适用
	PCF212x_A_Second = 0x1E,    // 0001,1110 < 报警：当匹配设定的秒
	PCF212x_A_Minute = 0x1C,    // 0001,1100 < 报警：当匹配设定的秒、分
	PCF212x_A_Hour = 0x18,      // 0001,1000 < 报警：当匹配设定的秒、分、时
	PCF212x_A_Date = 0x10,      // 0001,0000 < 报警：当匹配设定的秒、分、时、日期
	PCF212x_A_Day = 0x08,       // 0000,1000 < 报警：当匹配设定的秒、分、时、星期
//  PCF212x_A1_Date_Day = 0x00,  // 0000,0000 < 报警：当匹配设定的秒、分、时、星期或星期
};
#endif /* RTCLIB_SRC_RTC_DEFINES_H_ */
