
#include <ee24.hpp>
#include "main.h"	//引入HAL库

#if (_EEPROM_USE_FREERTOS == 1)		//是否使用RTOS
#include "cmsis_os.h"
#define ee24_delay(x)   osDelay(x)
#else
#define ee24_delay(x)   HAL_Delay(x)
#endif



uint8_t ee24_lock = 0;

void EE24::autoInit(bool autoDetermine){
	if(autoDetermine) {
		determineMemSize();	//初始化 _sizeMemKbit
		determinePageSize();//初始化 _sizePageByte
	}
	//需要根据size把I2C write和byte的读写参数确定下来，防止每次while循环都判断一连串if--else
	if (((_sizeMemKbit==1) || (_sizeMemKbit==2))) {
		_addrBitAndDev = 0x0000;	//0000就不会影响I2C地址
		_addrBitRightOffSet = 0;
		_addrBitAndMem =  0xff;
	}
	else if (_sizeMemKbit==4) {
		_addrBitAndDev = 0x0100;
		_addrBitRightOffSet = 7;
		_addrBitAndMem = 0xff;		//
	}
	else if (_sizeMemKbit==8) {
		_addrBitAndDev = 0x0300;
		_addrBitRightOffSet= 7;
		_addrBitAndMem = 0xff;		//
	}
	else if (_sizeMemKbit==16) {
		_addrBitAndDev = 0x0700;
		_addrBitRightOffSet= 7;
		_addrBitAndMem = 0xff;		//
	}
	else if (_sizeMemKbit == 1024) {
		_addrBitAndDev = 0x10000;	// AT24C1024又有P位了
		_addrBitRightOffSet= 15;
		_addrBitAndMem = 0x1ffff;	// 1,1111,1111,1111,1111	//只bit[31:17]为0
	}
	else  {//AT24C32 ~ AT24C512 没有P位
		_addrBitAndDev = 0x0000; //0000就不会影响I2C地址
		_addrBitRightOffSet = 0;
		_addrBitAndMem =  0x1ffff;
	}

	if((_sizeMemKbit * 128) >= I2C_DEVICESIZE_24LC32)
		_memAddSize = I2C_MEMADD_SIZE_16BIT;
	else
		_memAddSize = I2C_MEMADD_SIZE_8BIT;
}

/*
 * EEPROM I2C地址 转换
 * @param address 地址指针地址
 */
uint8_t EE24::devAddrConv(uint32_t address)
{
	return _i2cAddr | ((address & _addrBitAndDev) >> _addrBitRightOffSet);
}

/*
 * 地址指针地址 转换
 */
uint32_t EE24::memAddrConv(uint32_t address)
{
	return address & _addrBitAndMem;
}


bool EE24::isConnected(void)
{
#if (_EEPROM_USE_WP_PIN==1)
	HAL_GPIO_WritePin(_EEPROM_WP_GPIO,_EEPROM_WP_PIN,GPIO_PIN_SET);
#endif
	if (HAL_I2C_IsDeviceReady(_hi2c, _i2cAddr, 2, 100)==HAL_OK)
		return true;
	else
		return false;
}

bool EE24::writeByte(uint32_t address, uint8_t *data)
{ return writeBytes(address, data, 1); }


bool EE24::writeBytes(uint32_t address, uint8_t *data, size_t len)
{
  if (ee24_lock == 1)
    return false;
  ee24_lock = 1;
  uint16_t w;
  uint32_t startTime = HAL_GetTick();
  #if	(_EEPROM_USE_WP_PIN==1)
  HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN,GPIO_PIN_RESET);
  #endif
  while (1)
  {
    w = _sizePageByte - (address  % _sizePageByte);
    if (w > len)
      w = len;
    if (HAL_I2C_Mem_Write(_hi2c, devAddrConv(address), memAddrConv(address), _memAddSize, data, w, _timeout) == HAL_OK)
    {
      ee24_delay(10);
      len -= w;
      data += w;
      address += w;
      if (len == 0)
      {
        #if (_EEPROM_USE_WP_PIN==1)
        HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
        #endif
        ee24_lock = 0;
        return true;
      }
      if (HAL_GetTick() - startTime >= _timeout)
      {
        ee24_lock = 0;
        return false;
      }
    }
    else
    {
      #if (_EEPROM_USE_WP_PIN==1)
      HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
      #endif
      ee24_lock = 0;
      return false;
    }
  }
}


bool EE24::readByte(uint32_t address, uint8_t *data)
{ return  readBytes(address, data, 1); }

bool EE24::readBytes(uint32_t address, uint8_t *data, size_t len)
{
	if (ee24_lock == 1)
		return false;
	ee24_lock = 1;
#if (_EEPROM_USE_WP_PIN==1)
	HAL_GPIO_WritePin(_EEPROM_WP_GPIO, _EEPROM_WP_PIN, GPIO_PIN_SET);
#endif
	if (HAL_I2C_Mem_Read(_hi2c, devAddrConv(address), memAddrConv(address), _memAddSize, data, len, _timeout) == HAL_OK)
	{
		ee24_lock = 0;
		return true;
	}
	else
	{
		ee24_lock = 0;
		return false;
	}
}


bool EE24::eraseChip(void)
{
	//创建擦除数据，元素值全为11111111，总大小等于24C02 的 256Byte
	const uint8_t eraseData[32] = {
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
	};
	uint32_t bytes = 0;	//可以擦24C1024，但要擦512次循环！
	//反复写它以达到全片擦除，写N次32组0xff
	while ( bytes < (_sizeMemKbit * 256))
	{
		if (writeBytes(bytes, (uint8_t*)eraseData, sizeof(eraseData)) == false)	//若写入的地址没有应答成功就结束
			return false;
		bytes += sizeof(eraseData);
	}
	return true;
}


// 检测容量，
// 测试的地址写入会破坏原先位置的数据，建议对空片使用本函数
// returns size in bytes
// returns 0 if not connected
//
//   tested for 2 byte address
//   24LC512     64 KB    YES
//   24LC256     32 KB    YES
//   24LC128     16 KB    YES
//   24LC64       8 KB    YES
//   24LC32       4 KB    YES* - no hardware test, address scheme identical to 24LC64.
//
//   1 byte address (uses part of deviceAddress byte)
//   24LC16       2 KB    YES
//   24LC08       1 KB    YES
//   24LC04      512 B    YES
//   24LC02      256 B    YES
//   24LC01      128 B    YES

uint32_t EE24::determine_memsize()
{
	// try to read a byte to see if connected
	if (! isConnected()) return 0;

	// 设为测试值
	_sizePageByte = 8;
	_addrBitAndDev = 0xff;
	_addrBitRightOffSet = 0;
	_addrBitAndMem =  0x1ffff;
	_memAddSize = I2C_MEMADD_SIZE_8BIT;

	uint8_t patAA = 0xAA;	//1010,1010
	uint8_t pat55 = 0x55; //0101,0101
	/*
	 * 每次倍增size，注意size即作检测的EEPROM大小，也作为地址指针位置
	 * 对于24C01，size = 127是最末尾的byte，size = 128 会循环覆盖到 0
	 * 对于24C02，size = 255是最末尾的byte，size = 256 会循环覆盖到 0
	 * ......
	 */
	uint32_t size;
	uint32_t sizePrev = 0;
	uint32_t sizeLater = 0;
	bool folded;

	for(int j = 0; j < 2; j++){
		if(j == 1)
			_memAddSize = I2C_MEMADD_SIZE_16BIT;

		for (size = 128; size <= 65536; size *= 2)
		{
			folded = false;
//			uint8_t buf;
//			readByte(size,&buf);		//去读地址指针=size处地址的1byte值，临时存起来

			// test folding 测试折叠
			uint8_t cnt = 0;			//统计页的大小，以byte为单位
			writeByte(size,  &pat55); 	//对size地址写入 0101,0101

			uint8_t readBuffer;
			readByte( 0, &readBuffer);

			if (readBuffer == pat55) cnt++;	//如果首地址的值与写入的值相同，说明地址指针溢出后回到第一页地址， 第一页之前的数据将被覆盖，不过没关系，我们事先存了副本到buf中
			writeByte(size, &patAA); 		//对size地址继续写入 1010,1010，地址指针会自增吗？

			readByte( 0,  &readBuffer);
			if (readBuffer == patAA) cnt++;	//如果首地址的值与写入的值相同，说明页发生了折叠

			folded = (cnt == 2);			//如果cnt==2，那么发生了折叠

			readByte(size, &readBuffer);
			DBG_EE24("size = %d,read address size: %d  ", size, readBuffer);
//			writeByte(size, &buf); // restore old values	//恢复原有的数据，也就是说这个测试不会破坏原有的数据，但对8bit地址指针使用16bit搞不一定
			if (folded) {
				return size;
				if(j == 0) {
					if(size > 65535) size = 0;	//说明16bit地址指针溢出
					sizePrev = size;
				}else{ //j == 1
					if(size > 65535) size = 0;	//说明16bit地址指针溢出
					sizeLater = size;
				}
			}
		}
	}

	//如果至少一个size非零, 那肯定发生了折叠，可以确定地址值位数
	if(sizePrev | sizeLater){
		if(sizePrev < sizeLater) {
			_memAddSize = I2C_MEMADD_SIZE_16BIT;
			size = sizeLater;
		}
		else {
			_memAddSize = I2C_MEMADD_SIZE_8BIT;
			size = sizePrev;
		}
		return size / 128;	//以Kbit单位返回，例如24C128返回128
	}
	return 0;
}

//   tested for 2 byte address
//   24LC1024     128 KB    待测试
uint32_t EE24::determineMemSize()
{
	uint32_t size = determine_memsize();
	if(size == I2C_DEVICESIZE_24LC512){
		uint8_t devAddrOld = _devAddress;
		_devAddress = _devAddress | 0x2; //更改P0位以检测24C1024
		if(determine_memsize() == I2C_DEVICESIZE_24LC512)
			size = I2C_DEVICESIZE_24LC1024;
		_devAddress = devAddrOld;	//还原地址
	}
	_sizeMemKbit = size;
	return size;
}

uint16_t EE24::determinePageSize()
{
	uint16_t size;
	// determine page size from device size - based on Microchip 24LCXX data sheets.
	if (_sizeMemKbit <= I2C_DEVICESIZE_24LC02) size = 8;
	else if (_sizeMemKbit <= I2C_DEVICESIZE_24LC16) size = 16;
	else if (_sizeMemKbit <= I2C_DEVICESIZE_24LC64) size = 32;
	else if (_sizeMemKbit <= I2C_DEVICESIZE_24LC256) size = 64;
	else if (_sizeMemKbit <= I2C_DEVICESIZE_24LC512) size = 128;
	// I2C_DEVICESIZE_24LC1024
	else size = 256;
	_sizePageByte = size;
	return size;
}


//请自行配置
void EE24::exc_I2C_Init(){
	GPIO_InitTypeDef GPIO_InitStruct = {0};
	if(_hi2c->Instance==I2C1)
	{
		/* USER CODE BEGIN I2C1_MspInit 0 */

		/* USER CODE END I2C1_MspInit 0 */

		__HAL_RCC_GPIOB_CLK_ENABLE();
		/**I2C1 GPIO Configuration
PB6     ------> I2C1_SCL
PB7     ------> I2C1_SDA
		 */
		GPIO_InitStruct.Pin = GPIO_PIN_6|GPIO_PIN_7;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF6_I2C1;
		HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

		/* Peripheral clock enable */
		__HAL_RCC_I2C1_CLK_ENABLE();
		/* USER CODE BEGIN I2C1_MspInit 1 */

		/* USER CODE END I2C1_MspInit 1 */
	}
}

//请自行配置
void EE24::exc_I2C_DeInit(){
	if(_hi2c->Instance==I2C1)
	{
		/* USER CODE BEGIN I2C1_MspDeInit 0 */

		/* USER CODE END I2C1_MspDeInit 0 */
		/* Peripheral clock disable */
		__HAL_RCC_I2C1_CLK_DISABLE();

		/**I2C1 GPIO Configuration
	PB6     ------> I2C1_SCL
	PB7     ------> I2C1_SDA
		 */
		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_6);

		HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7);

		/* USER CODE BEGIN I2C1_MspDeInit 1 */

		/* USER CODE END I2C1_MspDeInit 1 */
	}
}

#if 0	//用于测试地址算法
bool EE24::write_test_A()
{
	uint32_t address = 0xfff0;
	size_t len = 43;
	uint8_t array[43] = {0};
	uint8_t *data = array;
	int i = 0;
	uint16_t w;
	while (1)
	{
		i;
		w = 16 - (address  % 16);
		if (w > len)
			w = len;
		uint8_t DevAddress = _i2cAddr | ((address & 0x10000) >> 15);
		uint16_t MemAddress = (address);
		data;
		w;
		{
			ee24_delay(10);
			len -= w;
			data += w;
			address += w;
			if (len == 0)
			{

				return true;
			}
		}
		++i;
	}
}
#endif
