/*
  Author:     Nima Askari
  WebSite:    http://www.github.com/NimaLTD
  Instagram:  http://instagram.com/github.NimaLTD
  Youtube:    https://www.youtube.com/channel/UCUhY7qY1klJm1d2kulr9ckw
  
  Version:    2.2.1
  
  
  Reversion History:
  
  (2.2.1)
  Fix erase chip bug.
  
  (2.2.0)
  Add erase chip function.
  
  (2.1.0)
  Fix write bytes.
  
  (2.0.0)
  Rewrite again.

  @modify OldGerman
	2022/05/29
		增加对AT24C1024的支持
		增加检测EEPROM容量determineSize()
		增加获取EEPROM页大小getPageSize()
	2022/05/30
		打包到 class EE24 中
	2022/05/2?
		将宏定义的EEPROM容量大小、I2C地址、页大小改为变量，可以在我的Page-Colum类实现的多级菜单里配置
	2022/0627
		修复容量检测相关函数的 Kbit Byte单位换算BUG
		验证EE24::determine_memsize()结束时会还原检测时修改的数据
*/

#ifndef	_EE24_HPP
#define	_EE24_HPP


#ifdef __cplusplus
extern "C" {
#endif



#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "ee24Config.h"
#include "main.h"

extern uint8_t exChangeI2C1Pins;



#ifdef __cplusplus
class EE24{
public:
	EE24(I2C_HandleTypeDef *hi2c, uint8_t i2cAddr, uint32_t timeOut)
		:_hi2c(hi2c), _sizeMemKbit(0),  _sizePageByte(0), _i2cAddr(i2cAddr), _timeout(timeOut)
		{}

	EE24(I2C_HandleTypeDef *hi2c, uint32_t sizeMemKbit, uint8_t sizePageByte, uint8_t i2cAddr, uint32_t timeOut)
		:_hi2c(hi2c), _sizeMemKbit(sizeMemKbit),  _sizePageByte(sizePageByte), _i2cAddr(i2cAddr), _timeout(timeOut)
		{}

	~EE24(){}

	void autoInit(bool autoDetermine = false);
	/*
	 * 临时交换I2C引脚，I2C完事后恢复之前的设置
	 * 但不同处理器，GPIO_InitStruct.Alternate使用的 AF x 可能不一样，
	 * 因此你最好用CubeMX对两种管脚映射都自动生成一次，将临时交换下的
	 * HAL_I2C_DeInit()和HAL_I2C_MspDeInit()复制到成员函数exc_I2C_Init()和exc_I2C_DeInit()
	 * 场景：
	 * 	I2C1_SCL： PB8 <-> PB6
	 * 	I2C1_SDA： PB8 <-> PB6
	 * 运行流程：
	 *  执行HAL_I2C_MspDeInit()取消I2C GPIO配置
	 *	用新的引脚配置运行成员函数HAL_I2C_Init(),
	 *	等待I2C完事后，用新的引脚配置运行成员函数HAL_I2C_DeInit(),
	 *	运行HAL_I2C_MspInit()，恢复原先的引脚配置
	 *	只更改底层的管脚映射，全程不需要动上层的MX_I2C2_Init()
	 *
	 *	这个调用有问题，EXT_I2C1_Init()内部调用的HAL_I2C_Init()本身调用了HAL_I2C_MspInit()，又改回去了！
	 */
#if _EEPROM_EXC_PINS
	void  exchangeI2CPins()
	{
		exChangeI2C1Pins = 1;
		HAL_I2C_MspDeInit(_hi2c);
		exc_I2C_Init();
		EXT_I2C1_Init();
	}
	void recoverI2CPins()
	{
		exChangeI2C1Pins = 0;
		exc_I2C_DeInit();
		HAL_I2C_MspInit(_hi2c);
		EXT_I2C1_Init();
	}

#else
#define exchangeI2CPins(...)
#define recoverI2CPins(...)
#endif
	/**
	 * @fn isConnected
	 * @brief detected eeprom i2c AKC
	 * @return bool
	 * @retval  1    connect succeeded
	 * @retval  0    connect failed
	 */
	bool    isConnected(void);

	/**
	 * @fn write
	 * @brief write data to eeprom
	 * @param address 		开始的eeprom内部存储单元的地址
	 * @param data 			需要写入数据的地址
	 * @param lenInBytes 	需要写入数据的byte数
	 * @param timeout 		超时等待时间
	 * @return bool
	 * @retval  1    write succeeded
	 * @retval  0    write failed
	 */
	bool    writeBytes(uint32_t address, uint8_t *data, size_t len);
	bool    writeByte(uint32_t address, uint8_t *data);
	/**
	 * @fn read
	 * @brief read data from eeprom
	 * @param address 		开始的eeprom内部存储单元的地址
	 * @param data 			需要存放读出数据的地址
	 * @param lenInBytes 	存放读出数据的byte数
	 * @param timeout 		超时等待时间
	 * @return bool
	 * @retval  1    write succeeded
	 * @retval  0    write failed
	 */
	bool    readBytes(uint32_t address, uint8_t *data, size_t len);
	bool    readByte(uint32_t address, uint8_t *data);
	/**
	 * @fn eraseChip
	 * @brief erase full chip of eeprom
	 * @return bool
	 * @retval  1    erase succeeded
	 * @retval  0    erase failed
	 */
	bool    eraseChip(uint32_t bytes = 0);
	bool 	write_test_A();

	uint32_t determineMemSize();
	uint16_t determinePageSize();
	uint16_t getPageSizeInByte() { return _sizePageByte; }
	uint16_t getMemSizeInKbit()  { return _sizeMemKbit; }
	uint32_t getMemSizeInByte()  { return _sizeMemKbit * 128; }
	void setMemSizeInKbit(uint16_t sizeMemKbit)  { _sizeMemKbit = sizeMemKbit; }
	void setPageSizeInByte(uint16_t sizePageByte) { _sizePageByte = sizePageByte; }
private:
	void 				exc_I2C_Init();
	void 				exc_I2C_DeInit();
	uint32_t 			determine_memsize();
	uint8_t 			devAddrConv(uint32_t address);
	uint32_t 			memAddrConv(uint32_t address);
	I2C_HandleTypeDef 	*_hi2c;
	uint16_t 			_sizeMemKbit;		//容量，单位Kbit
	uint8_t				_sizePageByte;		//页大小，单位Byte
	uint8_t 			_i2cAddr;			//EEPROM I2C 8bit地址，请将P位置0
	uint32_t	 		_timeout;			//传输超时时间

	//事先在Init()加工，R/W函数会使用
	uint8_t 			_devAddress;
	uint16_t 			_memAddres;
	uint16_t 			_memAddSize;
	uint32_t			_addrBitAndDev;
	uint32_t			_addrBitAndMem;
	uint8_t				_addrBitRightOffSet;
};
}
#endif

#endif
