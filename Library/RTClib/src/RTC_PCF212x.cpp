// SPDX-License-Identifier: GPL-2.0-only
/*
 * An I2C and SPI driver for the NXP PCF212x/29 RTC
 * Copyright 2013 Til-Technologies
 *
 * Author: Renaud Cerrato <r.cerrato@til-technologies.fr>
 *
 * Watchdog and tamper functions
 * Author: Bruno Thomsen <bruno.thomsen@gmail.com>
 *
 * based on the other drivers in this same directory.
 *
 * Datasheet: https://www.nxp.com/docs/en/data-sheet/PCF212x.pdf
 *
 * @modify OldGerman 2022/05/20
 */


#include "RTClib.h"

#ifndef DBG_PCF212X
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PCF212X usb_printf
#else
	#define DBG_PCF212X(...)
	#endif
#endif

#define BIT(nr) (1UL << (nr))
#define PCF212x_REG_OFFSET	  1	/// 偏移寄存器地址
								///	对于所有最终会使用HAL_I2C_Mem_Read()的FRToSI2C API传入参数的寄存器地址减去它

#define PCF212x_ADDRESS 0x51 << 1   ///< I2C address for PCF212x
#define PCF212x_TIME 0x03      		///< Time register     03h to 09h
#define PCF212x_ALARM 0x0A     		///< Alarm  register:  0Ah to 0Eh
#define PCF212x_CONTROL 0x00   		///< Control register: 00h to 02h

/* Control register 1 */
/*
 * What's the meaning of BIT() in this linux kernel macro?
 * #define HID_QUIRK_ALWAYS_POLL   BIT(10)
 * looks like you can find the answer inside the first header file included, i.e. bitops.h!
 * #define BIT(nr) (1UL << (nr))
 * 1UL就是 (unsigned long)1, 例如BIT(3) 就替换为 (unsigned long)1 << 3,
 * 即 00000000,00000000,00000000,00000001 << 3 即 00000000,00000000,00000000,00001000
 * BIT宏将值向左移动1给它的值，所以BIT(10)== (1 << (10))。它可用于从位字段中获取特定的布尔值
 *
 * 所以BIT(x)都是在设置掩码
 */
#define PCF212x_REG_CTRL1		0x00
#define PCF212x_BIT_CTRL1_12_24			BIT(2)		//复位默认是0，以24小时模式计时
#define PCF212x_BIT_CTRL1_POR_OVRD		BIT(3)		//	0000,1000
													//		 ^POR_OVD: Control_1[3]
#define PCF212x_BIT_CTRL1_TSF1			BIT(4)		//	0001,0000
													// 	   ^CTRL1_TSF1: Control_1[4]
#define PCF212x_REG_CTRL1_STOP 			BIT(5)		//  RTC时钟源使能标记位 0:run 1:stop
/* Control register 2 */
#define PCF212x_REG_CTRL2		0x01
#define PCF212x_BIT_CTRL2_AIE			BIT(1)
#define PCF212x_BIT_CTRL2_TSIE			BIT(2)
#define PCF212x_BIT_CTRL2_AF			BIT(4)
#define PCF212x_BIT_CTRL2_TSF2			BIT(5)
#define PCF212x_BIT_CTRL2_WDTF			BIT(6)
/* Control register 3 */
#define PCF212x_REG_CTRL3		0x02
#define PCF212x_BIT_CTRL3_BLIE			BIT(0)
#define PCF212x_BIT_CTRL3_BIE			BIT(1)
#define PCF212x_BIT_CTRL3_BLF			BIT(2)
#define PCF212x_BIT_CTRL3_BF			BIT(3)
#define PCF212x_BIT_CTRL3_BTSE			BIT(4)
/* Time and date registers */
#define PCF212x_REG_SC			0x03
#define PCF212x_BIT_SC_OSF			BIT(7)
#define PCF212x_REG_MN			0x04
#define PCF212x_REG_HR			0x05
#define PCF212x_REG_DM			0x06
#define PCF212x_REG_DW			0x07
#define PCF212x_REG_MO			0x08
#define PCF212x_REG_YR			0x09
/* Alarm registers */
#define PCF212x_REG_ALARM_SC		0x0A
#define PCF212x_REG_ALARM_MN		0x0B
#define PCF212x_REG_ALARM_HR		0x0C
#define PCF212x_REG_ALARM_DM		0x0D
#define PCF212x_REG_ALARM_DW		0x0E
#define PCF212x_BIT_ALARM_AE			BIT(7)
/* CLKOUT control register */
#define PCF212x_REG_CLKOUT		0x0f			// CLKOUT_ctl
#define PCF212x_REG_CLKOUT_COF0        BIT(0)
#define PCF212x_REG_CLKOUT_COF1        BIT(1)
#define PCF212x_REG_CLKOUT_COF2        BIT(2)
#define PCF212x_BIT_CLKOUT_OTPR		    BIT(5)	// 0001,0000
/* Watchdog registers */
#define PCF212x_REG_WD_CTL		0x10			// Watchdog_tim_ctl
#define PCF212x_BIT_WD_CTL_TF0			BIT(0)
#define PCF212x_BIT_WD_CTL_TF1			BIT(1)
#define PCF212x_BIT_WD_CTL_CD0			BIT(6)	// 标记为 T 的位，必须始终写入0
#define PCF212x_BIT_WD_CTL_CD1			BIT(7)
#define PCF212x_REG_WD_VAL		0x11
/* Tamper timestamp registers */
#define PCF212x_REG_TS_CTRL		0x12
#define PCF212x_BIT_TS_CTRL_TSOFF		BIT(6)
#define PCF212x_BIT_TS_CTRL_TSM			BIT(7)
#define PCF212x_REG_TS_SC		0x13
#define PCF212x_REG_TS_MN		0x14
#define PCF212x_REG_TS_HR		0x15
#define PCF212x_REG_TS_DM		0x16
#define PCF212x_REG_TS_MO		0x17
#define PCF212x_REG_TS_YR		0x18
/*
 * RAM registers
 * PCF212x has 512 bytes general-purpose static RAM (SRAM) that is
 * battery backed and can survive a power outage.
 * PCF2129 doesn't have this feature.
 */
#define PCF212x_REG_RAM_ADDR_MSB	0x1A
#define PCF212x_REG_RAM_WRT_CMD		0x1C
#define PCF212x_REG_RAM_RD_CMD		0x1D

/* Watchdog timer value constants */
#define PCF212x_WD_VAL_STOP		0
#define PCF212x_WD_VAL_MIN		2
#define PCF212x_WD_VAL_MAX		255
#define PCF212x_WD_VAL_DEFAULT		60

/* Mask for currently enabled interrupts */
#define PCF212x_CTRL1_IRQ_MASK (PCF212x_BIT_CTRL1_TSF1)
#define PCF212x_CTRL2_IRQ_MASK ( \
		PCF212x_BIT_CTRL2_AF | \
		PCF212x_BIT_CTRL2_WDTF | \
		PCF212x_BIT_CTRL2_TSF2)

/**************************************************************************/
/*!
    @brief  Start I2C for the PCF212x and test succesful connection
    @param  wireInstance pointer to the I2C bus
    @return True if Wire can find PCF212x or false otherwise.
*/
/**************************************************************************/
bool RTC_PCF212x::begin(FRToSI2C * ptrFRToSI2C, bool isPCF2127) {
	pFRToSI2C = ptrFRToSI2C;
	is_pcf2127 = isPCF2127;

	uint8_t ret = 0;
	uint8_t val;		//存储1byte寄存器值

	ret = pFRToSI2C->probe(PCF212x_ADDRESS);
	if (ret == 0)
		return ret;
	if(is_pcf2127)
	;	//RAM r/w is not available yet

//	/*
//	 * 设置PCF212x_REG_CTRL1寄存器的12_24位, 以24小时计时
//	 * 复位后的默认位值1是24，但为了保险再执行一次
//	 */
//	ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_CTRL1,
//			PCF212x_BIT_CTRL1_12_24, RESET);
//	if (ret == 0)
//		return ret;


	/*
	 * 设置PCF212x_REG_CTRL1寄存器的PORO位
	 *“上电复位覆盖”功能可防止 RTC 进行复位
	 * 开机后。对于正常操作，必须禁用 PORO
	 * The "Power-On Reset Override" facility prevents the RTC to do a reset
	 * after power on. For normal operation the PORO must be disabled.
	 */
	ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_CTRL1,
			PCF212x_BIT_CTRL1_POR_OVRD, RESET);
	if (ret == 0)
		return ret;
	/*
	 * 设置PCF212x_REG_CLKOUT寄存器的OTPR位
	 */
	//读取 0Fh地址的CLKOUT_ctl寄存器值存到val变量
	ret = pFRToSI2C->readByte(PCF212x_ADDRESS, PCF212x_REG_CLKOUT - PCF212x_REG_OFFSET, &val);
	if (ret < 0)
		return ret;
	//如果val = 0000,0000, 那么val & PCF212x_BIT_CLKOUT_OTPR 运算就会判定OTPR位是0（对应手册8.3.2清除OTPR位操作）
	//那么表示没有进行OTP刷新，接着调用regmap_set_bits()设置OTPR位为1，执行OTP刷新
	//(注意OTPR寄存器表RESET VALUE标的是X，表明上电未定义，如果val & PCF212x_BIT_CLKOUT_OTPR是1就不用执行if语句内刷新OTPR的操作了)，
	if (!(val & PCF212x_BIT_CLKOUT_OTPR))
	{
		ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_CLKOUT,
				PCF212x_BIT_CLKOUT_OTPR, SET);
		if (ret < 0)
			return ret;

		HAL_Delay(100);	//进行手册8.3.2描述的等待100ms，
						//切记此时还在线程的setup部分不能使用osDelay()
	}


	/*
	 * 配置PCF212x_REG_WD_CTL寄存器
	 *
	 * 看门狗定时器启用和复位引脚/RST 超时时激活。
	 * 为看门狗定时器选择 1Hz 时钟源。
	 * 注意：倒数计时器已禁用且不可用。
	 * 对于 pca2129、pcf2129，只有 bit[7] 用于 Symbol WD_CD
	 * 寄存器 watchdg_tim_ctl。bit[6] 被标记为 T
	 * 标记为 T 的位必须始终写为逻辑 0。
	 * 而对于 PCF212x  bit[7:6] 都用于 Symbol WD_CD
	 */
#if 1
	ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_WD_CTL,	//	配置为 0110,0010
				//mask（掩码）：以下按位与后得:    1100,0011
				 PCF212x_BIT_WD_CTL_CD1 |		// 1000,0000
				 PCF212x_BIT_WD_CTL_CD0 |		// 0100,0000
				 PCF212x_BIT_WD_CTL_TF1 |		// 0000,0010
				 PCF212x_BIT_WD_CTL_TF0,		// 0000,0001
				//val：以下按位与后得: 		   				    1100,0010	//就是说将掩码对应的位设置位1或0，你看val 0110,0010与mask 0110,0011三个1一样，但最后一个1是0，就是说val的前三个1直接用掩码的1与运算就行（掩码原来还可以这么用吗！），掩码的最后一个1（标记为1）是按val对应的位值0设置
				 (PCF212x_BIT_WD_CTL_CD1 & RESET) |			 // 1000,0000 & 00000000 //这个是使能看门狗的，我暂时禁用它，复位默认配置是0
				 (is_pcf2127 ? PCF212x_BIT_WD_CTL_CD0 : 0) | // 0100,0000(PCF212x) or 0000,0000(pcf2129)	//这个 T 位怎么理解？为啥还有设置为1的情况？
				 PCF212x_BIT_WD_CTL_TF1);					 // 0000,0010
	if (ret == 0) {
		DBG_PCF212X("PCF212x: watchdog config (wd_ctl) failed\r\n");
		return ret;
	}
#endif
	/*
	 * 禁用电池电量低/切换时间戳和中断。
	 * 清除可以阻止新触发事件的电池中断标志。
	 * 注意：这是默认的芯片行为，但添加以确保正确的篡改 时间戳 和中断功能
	 * Disable battery low/switch-over timestamp and interrupts.
	 * Clear battery interrupt flags which can block new trigger events.
	 * Note: This is the default chip behaviour but added to ensure
	 * correct tamper timestamp and interrupt function.
	 */
	ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_CTRL3,//	配置为 0000,0000
				// 0001,0011
				 PCF212x_BIT_CTRL3_BTSE |		// 0001,0000
				 PCF212x_BIT_CTRL3_BIE |		// 0000,0010
				 PCF212x_BIT_CTRL3_BLIE, 		// 0000,0001
				 //0000,0000
				 RESET);
	if (ret == 0) {
		DBG_PCF212X("%s: interrupt config (ctrl3) failed\r\n");
		return ret;
	}

	/*
	 * 关闭时间戳功能。不储存第一次触发的时间戳
	 * 启用 时间戳功能 并存储第一次触发的时间戳，直到 TSF1 和 TSF2 中断标志​​被清除。
	 * Enable timestamp function and store timestamp of first trigger
	 * event until TSF1 and TSF2 interrupt flags are cleared.
	 */
	ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_TS_CTRL, //	配置为 1000,0000
				 //1100,0000
				 PCF212x_BIT_TS_CTRL_TSOFF |	//0100,0000
				 PCF212x_BIT_TS_CTRL_TSM,		//1000,0000
				 //1100,0000
				 PCF212x_BIT_TS_CTRL_TSOFF |	//0100,0000
				 PCF212x_BIT_TS_CTRL_TSM);		//1000,0000
	if (ret == 0) {
		DBG_PCF212X("%s: tamper detection config (ts_ctrl) failed\r\n");
		return ret;
	}

	/*
	 * 关闭时间戳中断
	 * 在设置 TSF1 或 TSF2 时间戳标志时启用中断生成。
	 * 中断信号是开漏输出，如果不使用可以悬空。
	 * Enable interrupt generation when TSF1 or TSF2 timestamp flags
	 * are set. Interrupt signal is an open-drain output and can be
	 * left floating if unused.
	 */
	ret = pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_CTRL2 - PCF212x_REG_OFFSET,//	配置为  0000,0100
				 // 0000,0100
				 PCF212x_BIT_CTRL2_TSIE,
				 // 0000,0100
				 RESET);//PCF212x_BIT_CTRL2_TSIE);
	if (ret == 0) {
		DBG_PCF212X("%s: tamper detection config (ctrl2) failed\r\n");
		return ret;
	}

  return ret;
}

/**************************************************************************/
/*!
    @brief  Check RTC source clock runs or stop
    @return True if the bit is set (RTC source stopped) or false if it is
   running
*/
/**************************************************************************/
bool RTC_PCF212x::sourceClockRun(void) {
  uint8_t mode;
  pFRToSI2C->readBit(PCF212x_ADDRESS, PCF212x_REG_CTRL1 - PCF212x_REG_OFFSET, PCF212x_REG_CTRL1_STOP, &mode);
  return !mode;
}


/**************************************************************************/
/*!
	@brief  检查03h寄存器的 bit[7]：振荡器停止标志
    @return 		 = 0  说明保证时钟完整性
					 = 1   PCF212x因停电而停止发生过复位
*/
/* 典型使用场景
	if (rtc.lostPower()) {
	  Serial.println("RTC lost power, let's set the time!");
	  // When time needs to be set on a new device, or after a power loss, the
	  // following line sets the RTC to the date & time this sketch was compiled
	  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
	  // This line sets the RTC with an explicit date & time, for example to set
	  // January 21, 2014 at 3am you would call:
	  // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
	}
 */
/**************************************************************************/
bool RTC_PCF212x::lostPower(void) {
	uint8_t data;
	pFRToSI2C->readBit(PCF212x_ADDRESS, PCF212x_REG_SC - PCF212x_REG_OFFSET, PCF212x_BIT_SC_OSF, &data);
  return data;
}

/**************************************************************************/
/*!
    @brief  设置日期并翻转振荡器停止标志，设置好后才开启振荡器
    8.8 Time and date function
		Most of these registers are coded in the Binary Coded Decimal (BCD) format.
		这些寄存器中的大多数都以二进制编码十进制 (BCD) 格式编码。

    @brief  Set the date and flip the Oscillator Stop Flag
    @param dt DateTime object containing the date/time to set
*/
/**************************************************************************/
void RTC_PCF212x::adjust(const DateTime &dt) {
#if 0
	// 方法1 测试OK
	  uint8_t buffer[7] = {
			  bin2bcd(dt.second()),				// 0~59 以纯二进制编码
			  bin2bcd(dt.minute()),				// 0~59 以纯二进制编码
			  bin2bcd(dt.hour()),				// 初始化函数已配置Control_1的bit[2]会影响该
						   	   	   	   	   	   	    // 小时寄存器的bit[5]恒为0，以24小时模式计小时,
						   	   	   	   	   	   	    // 这样也方便程序，AM/PM模式还要处理一个标志位
			  bin2bcd(dt.day()),				//	与DS3231相反，是先day(其实是Date)后weekday
			  bin2bcd(dowToPCF212x(dt.dayOfTheWeek())),
			  bin2bcd(dt.month()),
			  bin2bcd((dt.year() - 2000U))
	  };
	  pFRToSI2C->writeBytes(PCF212x_ADDRESS, PCF212x_TIME, 7, buffer);	//可以正常更改时间寄存器所有值
#else
	//方法2：测试OK
  uint8_t buffer[8] = {
		  PCF212x_TIME,						// 时间寄存器组起始地址
		  bin2bcd(dt.second()),				// 0~59 以纯二进制编码
		  bin2bcd(dt.minute()),				// 0~59 以纯二进制编码
		  bin2bcd(dt.hour()),				// 初始化函数已配置Control_1的bit[2]会影响该
					   	   	   	   	   	   	    // 小时寄存器的bit[5]恒为0，以24小时模式计小时,
					   	   	   	   	   	   	    // 这样也方便程序，AM/PM模式还要处理一个标志位
		  bin2bcd(dt.day()),				//	与DS3231相反，是先day(其实是Date)后weekday
		  bin2bcd(dowToPCF212x(dt.dayOfTheWeek())),
		  bin2bcd(dt.month()),
		  bin2bcd((dt.year() - 2000U))
  };
  pFRToSI2C->Master_Transmit(PCF212x_ADDRESS, buffer, 8);
#endif
//  //设置时钟完整性标志为0
//  pFRToSI2C->writeBit(PCF212x_ADDRESS, PCF212x_TIME,
//		PCF212x_BIT_SC_OSF,	//bit[7] 1000,0000 掩码
//		RESET);					//       0000,0000 掩码位写0
}

/**************************************************************************/
/*!
    @brief  获取当前时间
    @brief  Get the current date/time
    @return DateTime object with the current date/time
*/
/**************************************************************************/
DateTime RTC_PCF212x::now() {
  uint8_t buffer[7] = {0};
  buffer[0] = PCF212x_TIME;
  pFRToSI2C->TransmitReceive(PCF212x_ADDRESS, buffer, 1, buffer, 7);
  return DateTime(
		  bcd2bin(buffer[6]) + 2000U,//年
		  bcd2bin(buffer[5]),		//月
		  bcd2bin(buffer[3]),		//pcf212x的day是buffer[3]，DS3231是buffer[4]
		  bcd2bin(buffer[2]),		//时
		  bcd2bin(buffer[1]),		//分
		  bcd2bin(buffer[0])& 0x7F);//秒 0111,1111 去除03h寄存器bit[7] OSF的影响
}

/**************************************************************************/
/*!
    @brief  读取方波生成的模式
    @brief  Read the SQW pin mode
    @return Pin mode, see Pcf212xSqwPinMode enum
*/
/**************************************************************************/
Pcf212xSqwPinMode RTC_PCF212x::readSqwPinMode() {
  uint8_t mode;
  pFRToSI2C->readBits(PCF212x_ADDRESS,
		  PCF212x_REG_CLKOUT - PCF212x_REG_OFFSET,
		  PCF212x_REG_CLKOUT_COF0 | PCF212x_REG_CLKOUT_COF1 | PCF212x_REG_CLKOUT_COF2,
		   &mode);

  return static_cast<Pcf212xSqwPinMode>(mode);	//static_cast： C++四种类型转换运算符之一
}

/**************************************************************************/
/*!
    @brief  设置方波引脚的模式
    @brief  Set the SQW pin mode
    @param mode Desired mode, see Pcf212xSqwPinMode enum
*/
/**************************************************************************/
void RTC_PCF212x::writeSqwPinMode(Pcf212xSqwPinMode mode) {
	  pFRToSI2C->writeBits(PCF212x_ADDRESS, PCF212x_REG_CLKOUT - PCF212x_REG_OFFSET, mode, mode);
}

//
///**************************************************************************/
///*!
//    @brief  Set alarm for PCF212x
//        @param 	dt DateTime object
//        @param 	alarm_mode Desired mode, see Pcf212xAlarmMode enum
//    @return False if control register is not set, otherwise true
//*/
///**************************************************************************/
//bool RTC_PCF212x::setAlarm(const DateTime &dt, Pcf212xAlarmMode alarm_mode) {
//  uint8_t ctrl = read_register(PCF212x_CONTROL);
//  if (!(ctrl & 0x04)) {
//    return false;
//  }
//
//  uint8_t A1M1 = (alarm_mode & 0x01) << 7; // Seconds bit 7.
//  uint8_t A1M2 = (alarm_mode & 0x02) << 6; // Minutes bit 7.
//  uint8_t A1M3 = (alarm_mode & 0x04) << 5; // Hour bit 7.
//  uint8_t A1M4 = (alarm_mode & 0x08) << 4; // Day/Date bit 7.
//  uint8_t DY_DT = (alarm_mode & 0x10)
//                  << 2; // Day/Date bit 6. Date when 0, day of week when 1.
//  uint8_t day = (DY_DT) ? dowToPCF212x(dt.dayOfTheWeek()) : dt.day();
//
//  uint8_t buffer[5] = {PCF212x_ALARM1, uint8_t(bin2bcd(dt.second()) | A1M1),
//                       uint8_t(bin2bcd(dt.minute()) | A1M2),
//                       uint8_t(bin2bcd(dt.hour()) | A1M3),
//                       uint8_t(bin2bcd(day) | A1M4 | DY_DT)};
//  i2c_dev->write(buffer, 5);
//
//  write_register(PCF212x_CONTROL, ctrl | 0x01); // AI1E
//
//  return true;
//}
//
///**************************************************************************/
///*!
//    @brief  Disable alarm
//        @param 	alarm_num Alarm number to disable
//*/
///**************************************************************************/
//void RTC_PCF212x::disableAlarm(uint8_t alarm_num) {
//  uint8_t ctrl = read_register(PCF212x_CONTROL);
//  ctrl &= ~(1 << (alarm_num - 1));
//  write_register(PCF212x_CONTROL, ctrl);
//}
//
///**************************************************************************/
///*!
//    @brief  Clear status of alarm
//        @param 	alarm_num Alarm number to clear
//*/
///**************************************************************************/
//void RTC_PCF212x::clearAlarm(uint8_t alarm_num) {
//  uint8_t status = read_register(PCF212x_STATUSREG);
//  status &= ~(0x1 << (alarm_num - 1));
//  write_register(PCF212x_STATUSREG, status);
//}
//
///**************************************************************************/
///*!
//    @brief  Get status of alarm
//        @param 	alarm_num Alarm number to check status of
//        @return True if alarm has been fired otherwise false
//*/
///**************************************************************************/
//bool RTC_PCF212x::alarmFired(uint8_t alarm_num) {
//  return (read_register(PCF212x_STATUSREG) >> (alarm_num - 1)) & 0x1;
//}
//
///**************************************************************************/
///*!
//    @brief  Enable 32KHz Output
//    @details The 32kHz output is enabled by default. It requires an external
//    pull-up resistor to function correctly
//*/
///**************************************************************************/
//void RTC_PCF212x::enable32K(void) {
//  uint8_t status = read_register(PCF212x_STATUSREG);
//  status |= (0x1 << 0x03);
//  write_register(PCF212x_STATUSREG, status);
//}
//
///**************************************************************************/
///*!
//    @brief  Disable 32KHz Output
//*/
///**************************************************************************/
//void RTC_PCF212x::disable32K(void) {
//  uint8_t status = read_register(PCF212x_STATUSREG);
//  status &= ~(0x1 << 0x03);
//  write_register(PCF212x_STATUSREG, status);
//}
//
///**************************************************************************/
///*!
//    @brief  Get status of 32KHz Output
//    @return True if enabled otherwise false
//*/
///**************************************************************************/
//bool RTC_PCF212x::isEnabled32K(void) {
//  return (read_register(PCF212x_STATUSREG) >> 0x03) & 0x01;
//}
