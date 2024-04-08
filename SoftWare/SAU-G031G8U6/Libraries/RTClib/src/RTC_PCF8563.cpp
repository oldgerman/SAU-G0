#include "RTClib.h"
#include "main.h"
#define BIT(nr) (1UL << (nr))

#ifndef DBG_PCF8563
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PCF8563 usb_printf
#else
#define DBG_PCF8563(...)
#endif
#endif

#define PCF8563_CLKOUT_MASK 0x83   //1000,0011/bit[6:2]未使用/< bitmask for SqwPinMode on CLKOUT pin

//从https://github.com/torvalds/linux/blob/master/drivers/rtc/rtc-pcf8563.c修改
#define PCF8563_REG_ST1		0x00 /* status */
#define PCF8563_BIT_REG_ST1_TEST1   BIT(7)
#define PCF8563_BIT_REG_ST1_STOP    BIT(5)
#define PCF8563_BIT_REG_ST1_TESTC	BIT(3)		//	0000,1000

#define PCF8563_REG_ST2		0x01
#define PCF8563_BIT_REG_ST2_TIE		BIT(0)
#define PCF8563_BIT_REG_ST2_AIE		BIT(1)
#define PCF8563_BIT_REG_ST2_TF		BIT(2)
#define PCF8563_BIT_REG_ST2_AF		BIT(3)
#define PCF8563_BIT_REG_ST2_TI_TP		BIT(4)
#define PCF8563_BITS_REG_ST2_N	(7 << 5)

#define PCF8563_REG_SC		0x02    /* datetime */
#define PCF8563_BIT_SC_VL	BIT(7)	/*时钟完整状态, 0完整, 1不完整*/
#define PCF8563_REG_MN		0x03
#define PCF8563_REG_HR		0x04
#define PCF8563_REG_DM		0x05
#define PCF8563_REG_DW		0x06
#define PCF8563_REG_MO		0x07
#define PCF8563_REG_YR		0x08

#define PCF8563_REG_AMN		0x09 /* alarm */

#define PCF8563_REG_CLKO		0x0D /* clock out */
#define PCF8563_REG_CLKO_FE		BIT(7) /* clock out enabled */
#define PCF8563_REG_CLKO_F_MASK	    3
#define PCF8563_REG_CLKO_F_32768HZ	0x00
#define PCF8563_REG_CLKO_F_1024HZ	0x01
#define PCF8563_REG_CLKO_F_32HZ		0x02
#define PCF8563_REG_CLKO_F_1HZ		0x03

#define PCF8563_REG_TMRC	0x0E /* timer control */
#define PCF8563_REG_TMRC_ENABLE	BIT(7)
#define PCF8563_REG_TMRC_TD_0	BIT(0)
#define PCF8563_REG_TMRC_TD_1	BIT(1)
#define PCF8563_REG_TMRC_TD_MASK		PCF8563_REG_TMRC_TD_0 | PCF8563_REG_TMRC_TD_1 /* frequenc mask */
#define PCF8563_TMRC_4096	0
#define PCF8563_TMRC_64		1
#define PCF8563_TMRC_1		2
#define PCF8563_TMRC_1_60	3
#define PCF8563_TMRC_MASK	3

#define PCF8563_REG_TMR		0x0F /* timer */

#define PCF8563_SC_LV		0x80 /* low voltage */
#define PCF8563_MO_C		0x80 /* century */

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF8563 and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find PCF8563 or false otherwise.
 */
/**************************************************************************/
bool RTC_PCF8563::begin() {
	ret = pFRToSI2C->probe(PCF8563_ADDRESS);
	if (ret == 0){
		return ret;
	}
	/**********PCF8563初始化说明**********/
	/* 计时方式只有24小时制
	 * 没有看门狗
	 * 没有时间戳寄存器组
	 * 没有校准bit
	 * 以上无需配置
	 **********************************/
	/*
	 * 设置Control_status_1寄存器的TESTC位
	 * “上电复位覆盖”功能可防止 RTC 进行复位
	 */
	uint8_t val;
	ret = HAL_I2C_Mem_Read(&hi2c1, PCF8563_ADDRESS, PCF8563_REG_ST1, 1, &val, 1, 100);
	ret = HAL_I2C_Mem_Read(&hi2c1, PCF8563_ADDRESS, PCF8563_REG_ST1, 1, &val, 1, 100);


	write_bit(PCF8563_REG_ST1, PCF8563_BIT_REG_ST1_TESTC, RESET);
	if (ret == 0)
		return ret;


	/* Set timer to lowest frequency to save power (ref Haoyu datasheet)
	 * 这些位决定倒计数定时器的源时钟；不使用时，TD[1:0]应设置为 1/60 Hz，以节省电源。
	 * */
	write_bits(PCF8563_REG_TMRC,
			PCF8563_REG_TMRC_ENABLE|PCF8563_REG_TMRC_TD_MASK,
			RESET|PCF8563_TMRC_1_60);
	if (ret == 0) {
		DBG_PCF8563("%s: write error\n", "PCF8563");
		return ret;
	}

	/* Clear flags and disable interrupts
	 * 清除所有的中断标志和中断使能位*/
	write_byte(PCF8563_REG_ST2, RESET);
	if (ret == 0) {
		DBG_PCF8563("%s: write error\n", "PCF8563");
		return ret;
	}

	return ret;
}

/**************************************************************************/
/*!
    @brief  Check the status of the VL bit in the VL_SECONDS register.
    @details The PCF8563 has an on-chip voltage-low detector. When VDD drops
     below Vlow, bit VL in the VL_seconds register is set to indicate that
     the integrity of the clock information is no longer guaranteed.
    @return True if the bit is set (VDD droped below Vlow) indicating that
    the clock integrity is not guaranteed and false only after the bit is
    cleared using adjust()
 */
/**************************************************************************/
bool RTC_PCF8563::lostPower(void) {
	uint8_t data;
	read_bit(PCF8563_REG_SC, PCF8563_BIT_SC_VL, &data);
	return data;
}

/**************************************************************************/
/*!
    @brief  Set the date and time
    @param dt DateTime to set
 */
/**************************************************************************/
void RTC_PCF8563::adjust(const DateTime &dt) {
	uint8_t buffer[8] = {
			PCF8563_REG_SC, // start at location 2, VL_SECONDS
			bin2bcd(dt.second()),
			bin2bcd(dt.minute()),
			bin2bcd(dt.hour()),
			bin2bcd(dt.day()),
			bin2bcd(0), // skip weekdays
			bin2bcd(dt.month()),
			bin2bcd(dt.year() - 2000U)
	};
	master_transmit(buffer, 8);
}

/**************************************************************************/
/*!
    @brief  Get the current date/time
    @return DateTime object containing the current date/time
 */
/**************************************************************************/
DateTime RTC_PCF8563::now() {
	uint8_t buffer[7];
	buffer[0] = PCF8563_REG_SC; // start at location 2, VL_SECONDS
	transmit_receive(buffer, 1, buffer, 7);

	return DateTime(
			bcd2bin(buffer[6]) + 2000U,
			bcd2bin(buffer[5] & 0x1F),
			bcd2bin(buffer[3] & 0x3F),
			bcd2bin(buffer[2] & 0x3F),
			bcd2bin(buffer[1] & 0x7F),
			bcd2bin(buffer[0]) & 0x7F);
}

/**************************************************************************/
/*!
    @brief  Resets the STOP bit in register Control_1
 */
/**************************************************************************/
void RTC_PCF8563::start(void) {
	uint8_t bit;
	read_bit(PCF8563_REG_ST1, PCF8563_BIT_REG_ST1_STOP, &bit);
	if (bit == SET)
		write_bit(PCF8563_REG_ST1, PCF8563_BIT_REG_ST1_STOP, RESET);
}

/**************************************************************************/
/*!
    @brief  Sets the STOP bit in register Control_1
 */
/**************************************************************************/
void RTC_PCF8563::stop(void) {
	uint8_t bit;
	read_bit(PCF8563_REG_ST1, PCF8563_BIT_REG_ST1_STOP, &bit);
	if (bit == RESET)
		write_bit(PCF8563_REG_ST1, PCF8563_BIT_REG_ST1_STOP, SET);
}

/**************************************************************************/
/*!
    @brief  Is the PCF8563 running? Check the STOP bit in register Control_1
    @return 1 if the RTC is running, 0 if not
 */
/**************************************************************************/
uint8_t RTC_PCF8563::isrunning() {
	uint8_t bit;
	read_bit(PCF8563_REG_ST1, PCF8563_BIT_REG_ST1_STOP, &bit);
	return !bit;
}

/**************************************************************************/
/*!
    @brief  Read the mode of the CLKOUT pin on the PCF8563
    @return CLKOUT pin mode as a #Pcf8563SqwPinMode enum
 */
/**************************************************************************/
Pcf8563SqwPinMode RTC_PCF8563::readSqwPinMode() {
	uint8_t mode;
	read_byte(PCF8563_REG_CLKO,&mode);
	return static_cast<Pcf8563SqwPinMode>(mode & PCF8563_CLKOUT_MASK);
}

/**************************************************************************/
/*!
    @brief  Set the CLKOUT pin mode on the PCF8563
    @param mode The mode to set, see the #Pcf8563SqwPinMode enum for options
 */
/**************************************************************************/
void RTC_PCF8563::writeSqwPinMode(Pcf8563SqwPinMode mode) {
	write_byte(PCF8563_REG_CLKO, mode);
}


/**************************************************************************/
/*!
    @brief  Set alarm for PCF8563
        @param 	dt DateTime object
        @param 	alarm_mode Desired mode, see Ds3231Alarm1Mode enum
    @return False if control register is not set, otherwise true
*/
/**************************************************************************/
bool RTC_PCF8563::setTimeAlarm(const DateTime *dt, PCF8563AlarmMode alarm_mode) {
/*
	PCF8563_A_Minute = 0x1C,    // 0001,1100 < 报警：当匹配设定的分
	PCF8563_A_Hour = 0x18,      // 0001,1000 < 报警：当匹配设定的分、时
	PCF8563_A_Date = 0x10,      // 0001,0000 < 报警：当匹配设定的分、时、日期
	PCF8563_A_Day = 0x08,       // 0000,1000 < 报警：当匹配设定的分、时、星期
 */
  //将枚举配置的8bit位映射到每一个时间单位寄存器的MSB，即使能位，若不使能，那么闹钟匹配条件会忽略这个时间单位
  uint8_t MA = (alarm_mode & 0x02) << 6; // Minutes bit 7.	// 0000,0010 // 分寄存器 	bit[7] 匹配枚举 bit[1]
  uint8_t HA = (alarm_mode & 0x04) << 5; // Hour bit 7.		// 0000,0100 // 时寄存器 	bit[7] 匹配枚举 bit[2]
  uint8_t DA = (alarm_mode & 0x08) << 4; // Day bit 7.		// 0000,1000 // 日期寄存器 	bit[7] 匹配枚举 bit[3]
  uint8_t WA = (alarm_mode & 0x10) << 3; // Date bit 7. 	// 0001,0000 // 星期寄存器 	bit[6] 匹配枚举 bit[4]

  uint8_t buffer[5] = { PCF8563_REG_AMN,							//Alarm1寄存器组起始地址
                        uint8_t(bin2bcd(dt->minute()) | MA),//转换BCD编码值，并按位与标志位
                        uint8_t(bin2bcd(dt->hour()) | HA),
						uint8_t(bin2bcd(dt->day()) | DA),
                        uint8_t(bin2bcd(dt->dayOfTheWeek())| WA)};
  master_transmit(buffer, 5);
  return ret;
}

//闹钟中断使能或关闭
bool RTC_PCF8563::setIntAlarm(bool enable)
{
	write_bit(PCF8563_REG_ST2, PCF8563_BIT_REG_ST2_AIE,
				enable ? PCF8563_BIT_REG_ST2_AIE : RESET);
	return ret;
}


/*
 * 清除Alarm Flag (AF)
 * PCF8563_BIT_CTRL2_AF
 * 0: no alarm interrupt generated
 * 2: flag set when alarm triggered; (flag must be cleared to clear interrupt)
 */
bool RTC_PCF8563::clearFlagAlarm() {
	write_bit(PCF8563_REG_ST2,//	配置为 0000,0000
			PCF8563_BIT_REG_ST2_AF, RESET);
	return ret;
}

/**************************************************************************/
/*!
    @brief  Get status of alarm
        @param 	alarm_num Alarm number to check status of
        @return True if alarm has been fired otherwise false
*/
/**************************************************************************/
bool RTC_PCF8563::alarmFired() {
	  uint8_t mode;
	  read_bit(PCF8563_REG_ST2, PCF8563_BIT_REG_ST2_AF, &mode);
	  return mode;
}


