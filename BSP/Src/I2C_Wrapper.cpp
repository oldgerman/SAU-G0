/*
 * FRToSI2C.cpp
 *
 *  Created on: 14Apr.,2018
 *      Author: Ralim
 *      Modify: OldGerman
 *
 *
 *      readBytes() -> I2C_Unstick()
 *      writeBytes() -> I2C_Unstick()
 */
#include "I2C_Wrapper.h"
#include "BSP.h"

/*根据使用的hi2cx取消注释*/
#if(defined(SCL1_Pin) && defined(SCL1_Pin))
FRToSI2C FRToSI2C1(&hi2c1);
#endif
#if(defined(SCL2_Pin) && defined(SCL2_Pin))
FRToSI2C FRToSI2C2(&hi2c2);
#endif
#if(defined(SCL3_Pin) && defined(SCL3_Pin))
FRToSI2C FRToSI2C3(&hi2c3);
#endif

void FRToSI2C::FRToSInit() {
#if RTOS_EN
	if (_I2CSemaphore == nullptr) {
		_I2CSemaphore = xSemaphoreCreateMutexStatic(&(_xSemaphoreBuffer));
//		unlock(); //同xSemaphoreGive(_I2CSemaphore);//释放信号量
		xSemaphoreGive(_I2CSemaphore);
	}
#else
	_I2CSemaphore = 1;
	unlock();
#endif

}

/*
 * 若使用RTOS，这个函数放到 非阻塞模式（中断和DMA）中使用的I2C IRQHandler和回调（对__weak重写）中
 * void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }	//主接收完成
 * void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }	//主发送完成
 * void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
 * void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
 * void HAL_I2C_AbortCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
 * void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c) { FRToSI2C::CpltCallback(hi2c); }
 *
 * 被I2C非阻塞模式(Interrupt and DMA) 的回调函数调用
 * 功能：释放I2C信号量
 * 原子FreeRTOS指南 P236
 */
void FRToSI2C::CpltCallback(I2C_HandleTypeDef *I2C_Handle) {
	//强制状态重置（即使发送错误）
	// Force state reset (even if tx error)
	I2C_Handle->State = HAL_I2C_STATE_READY;
#if RTOS_EN
	if ((I2C_Handle == FRToSI2C1.getI2C_Handle())
			&& (*FRToSI2C1.getI2CSemaphore()))
		xSemaphoreGiveFromISR(*FRToSI2C1.getI2CSemaphore(), NULL);
	if ((I2C_Handle == FRToSI2C2.getI2C_Handle())
			&& (*FRToSI2C2.getI2CSemaphore()))
		xSemaphoreGiveFromISR(*FRToSI2C2.getI2CSemaphore(), NULL);
#else
//	if (I2C_Handle == FRToSI2C1.getI2C_Handle())
//	;
#endif
}

/**
 * @brief  阻塞模式下向1个寄存器写入1个8bit数据
 * 		   使用场景：在FRToSI2C::writeRegistersBulk()内用于传输
 * 		   			 FRToSI2C::I2C_REG结构体数组的数据
 * @param  address:  I2C从设备地址
 * @param  reg: 	 寄存器地址
 * @param  data:     向寄存器写入的值
 * @retval bool
 */
bool FRToSI2C::I2C_RegisterWrite(uint8_t address, uint8_t reg, uint8_t data) {
	return writeBytes(address, reg, 1, &data);	//传输1个8bit数据
//	return Mem_Write(address, reg, &data, 1);	//传输1个8bit数据
}

/**
 * @brief  阻塞模式下从1个寄存器读出1个8bit数据
 * 		   使用场景：
 * @param  add: 		I2C从设备地址
 * @param  reg: 		寄存器地址
 * @retval 8bit寄存器值
 */
uint8_t FRToSI2C::I2C_RegisterRead(uint8_t add, uint8_t reg) {
	uint8_t tx_data[1];
	readBytes(add, reg, 1, tx_data);
	return tx_data[0];
}

/** Read a single bit from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-7)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
//uint8_t FRToSI2C::readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t *pData) {
//    uint8_t b;
//    uint8_t count = readByte(devAddr, regAddr, &b);
//    *pData = b & (1 << bitNum);
//    return count;
//}

/** Read a single bit from an 8-bit device register.
 * 比如要读bit[6]是1,给pData指向的地址赋的值是类似0100,0000这种形式，不是1
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param mask 掩码
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
uint8_t FRToSI2C::readBit(uint8_t devAddr, uint8_t regAddr, uint8_t mask, uint8_t *pData) {
    uint8_t b;
    uint8_t count = readByte(devAddr, regAddr, &b);
    *pData = b & mask;
    return count;
}


/** Read a single bit from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-15)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
uint8_t FRToSI2C::readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t *pData) {
    uint8_t b;
    uint8_t count = readWord(devAddr, regAddr, &b);
    *pData = b & (1 << bitNum);
    return count;
}

/** Read multiple bits from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
uint8_t FRToSI2C::readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *pData) {
    // 01101001 read byte
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    //    010   masked
    //   -> 010 shifted
    uint8_t count, b;
    if ((count = readByte(devAddr, regAddr, &b)) != 0) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        b &= mask;
        b >>= (bitStart - length + 1);
        *pData = b;
    }
    return count;
}

/** Read multiple bits from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param mask 掩码
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
uint8_t FRToSI2C::readBits(uint8_t devAddr, uint8_t regAddr, uint8_t mask, uint8_t *pData) {
    // 01101001 read byte
    // 00001000   masked
    uint8_t count, b;
    if ((count = readByte(devAddr, regAddr, &b)) != 0) {
        b &= mask;
        *pData = b;
    }
    return count;
}

/** Read multiple bits from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-15)
 * @param length Number of bits to read (not more than 16)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (1 = success, 0 = failure, -1 = timeout)
 */
uint8_t FRToSI2C::readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *pData) {
    // 1101011001101001 read byte
    // fedcba9876543210 bit numbers
    //    xxx           args: bitStart=12, length=3
    //    010           masked
    //           -> 010 shifted
    uint8_t count;
    uint16_t w;
    if ((count = readWord(devAddr, regAddr, (uint8_t*)&w)) != 0) {
        uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        w &= mask;
        w >>= (bitStart - length + 1);
        *pData = w;
    }
    return count;
}


/** Read single byte from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for byte value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
uint8_t FRToSI2C::readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *pData) {
    return readBytes(devAddr, regAddr, 1, pData);
}

/** Read single word from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for word value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Status of read operation (true = success)
 */
uint8_t FRToSI2C::readWord(uint8_t devAddr, uint8_t regAddr, uint8_t *pData) {
    return readWords(devAddr, regAddr, 1, pData);
}

/** Read multiple bytes from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * 				注意，如果是7bit寄存器地址，需要自己进行 SUB_ADDR | 0x80 操作以更改MSB为1 开启寄存器指针自动递增
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Number of bytes read (-1 indicates failure)
 */
bool FRToSI2C::readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *pData) {
	if (!lock())		//尝试获取_I2CSemaphore
		return false;

	if (HAL_I2C_Mem_Read(_I2C_Handle, devAddr, regAddr,
			I2C_MEMADD_SIZE_8BIT, pData, length, 500) != HAL_OK) {

		I2C_Unstick();	//传输出错会执行
		unlock();		//释放_I2CSemaphore
		return false;
	}

	unlock();			//释放_I2CSemaphore
	return true;
}

bool FRToSI2C::Mem_Read(uint16_t DevAddress, uint16_t MemAddress,
		uint8_t *pData, uint16_t Size) {

	if (!lock())		//尝试获取_I2CSemaphore
		return false;

	if (HAL_I2C_Mem_Read(_I2C_Handle, DevAddress, MemAddress,
			I2C_MEMADD_SIZE_8BIT, pData, Size, 500) != HAL_OK) {

		I2C_Unstick();	//传输出错会执行
		unlock();		//释放_I2CSemaphore
		return false;
	}

	unlock();			//释放_I2CSemaphore
	return true;
}

/**
 * @brief  阻塞模式下发送大量数据
 * 		   使用场景：在FRToSI2C::I2C_RegisterWrite()内用于传输1个8bit数据
 * @param  DevAddress:  Target device address: The device 7 bits address value
 *         				in datasheet must be shifted to the left before calling the interface
 * @param  MemAddress:  Internal memory address
 * @param  pData: 		Pointer to data buffer
 * @param  Size: 		Amount of data to be sent
 * @retval bool
 */
bool FRToSI2C::Mem_Write(uint16_t DevAddress, uint16_t MemAddress,
		uint8_t *pData, uint16_t Size) {

	if (!lock())
		return false;
	if (HAL_I2C_Mem_Write(_I2C_Handle, DevAddress, MemAddress,
			I2C_MEMADD_SIZE_8BIT, pData, Size, 500) != HAL_OK) {

		I2C_Unstick();
		unlock();
		return false;
	}

	unlock();
	return true;
}
/** Read multiple words from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of words to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in FRToSI2C::readTimeout)
 * @return Number of words read (-1 indicates failure)
 */
bool FRToSI2C::readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *pData) {
	if (!lock())		//尝试获取_I2CSemaphore
		return false;

	if (HAL_I2C_Mem_Read(_I2C_Handle, devAddr, regAddr,
			I2C_MEMADD_SIZE_16BIT, pData, length, 500) != HAL_OK) {

		I2C_Unstick();	//传输出错会执行
		unlock();		//释放_I2CSemaphore
		return false;
	}

	unlock();			//释放_I2CSemaphore
	return true;
}

/** write a single bit in an 8-bit device register. 向设备的8bit寄存器的指定位写入0或1
 * @param devAddr I2C slave device address			从设备地址
 * @param regAddr Register regAddr to write to		寄存器地址
 * @param bitNum Bit position to write (0-7)		第几个bit位
 * @param value New bit value to write				写到这个bit位的值 0 or 1
 * @return Status of operation (true = success)
 */
//bool FRToSI2C::writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data) {
//    uint8_t b;
//    readByte(devAddr, regAddr, &b);
//    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
//    return writeByte(devAddr, regAddr, b);
//}

/** write a single bit in an 8-bit device register. 向设备的8bit寄存器的指定位写入0或1
 * @param devAddr I2C slave device address			从设备地址
 * @param regAddr Register regAddr to write to		寄存器地址
 * @param mask
 * @param value New bit value to write				写到这个bit位的值 0 or 1
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t mask, uint8_t data) {
    uint8_t b;
    readByte(devAddr, regAddr, &b);
    b = (data != 0) ? (b | mask) : (b & ~mask);
    return writeByte(devAddr, regAddr, b);
}

/** write a single bit in a 16-bit device register.	向设备的16bit寄存器的指定位写入0或1
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-15)
 * @param value New bit value to write
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data) {
    uint16_t w;
    readWord(devAddr, regAddr, (uint8_t *)&w);
    w = (data != 0) ? (w | (1 << bitNum)) : (w & ~(1 << bitNum));
    return writeWord(devAddr, regAddr, w);
}

/** Write multiple bits in an 8-bit device register. 向设备的8bit寄存器的多个指定位写入0或1
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write			 待写入的值（右对齐）
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data) {
    //      010 value to write
    // 76543210 bit numbers 即 bit[7:0]
    //    xxx   args: bitStart = 4, length = 3
    // 00011100 mask byte 根据传入的bitStart = 4, length = 3 计算掩码（函数内计算掩码, 不用宏事先定义）
    // 10101111 original value (sample)	//这个original值用readByte()先读出来
    // 10100011 original & ~mask
    // 10101011 masked | value
	//    ^^^   010 就是传入的 value to write
    uint8_t b;
    if (readByte(devAddr, regAddr, &b) != 0) {	//	先读出待修改寄存器的原始值
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask; // zero all non-important bits in data
        b &= ~(mask); // zero all important bits in existing byte
        b |= data; // combine data with existing byte
        return writeByte(devAddr, regAddr, b);	// 在不更改寄存器其他位原始值的情况下修改指定bits
    } else {
        return false;
    }
}

/** Write multiple bits in an 8-bit device register. 向设备的8bit寄存器的多个指定位写入0或1
 * @param devAddr: I2C slave device address
 * @param regAddr: Register regAddr to write to
 * @param mask:    直接传mask byte，不需要函数内计算mask byte
 * @param data:    Right-aligned value to write		待写入的值（不进行另一个重载函数根据bitStart 和 length进行移动data位的操作）
 * @return Status of operation (true = success)
 */
//#define fun(x) (-(x)?(1):(0))
bool FRToSI2C::writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t mask, uint8_t data) {
    // 00001000 value to write
    // 00011100 mask byte
    // 10101111 original value (sample)	//这个original值用readByte()先读出来
    // 10100011 original & ~mask 即 10101111 & 11100011 = 10100011，把mask标记的位都设置为0
    // 10101011 masked | value
	//    ^^^   注意观察这一列的位值变化过程
    uint8_t b;
    if (readByte(devAddr, regAddr, &b) != 0) {	//	先读出待修改寄存器的原始值
        data &= mask; // zero all non-important bits in data
        b &= ~(mask); // zero all important bits in existing byte
        b |= data; // combine data with existing byte
        return writeByte(devAddr, regAddr, b);	// 在不更改寄存器其他位原始值的情况下修改指定bits
    } else {
        return false;
    }
}

/** Write multiple bits in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-15)
 * @param length Number of bits to write (not more than 16)
 * @param data Right-aligned value to write
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data) {
    //              010 value to write
    // fedcba9876543210 bit numbers
    //    xxx           args: bitStart=12, length=3
    // 0001110000000000 mask word
    // 1010111110010110 original value (sample)
    // 1010001110010110 original & ~mask
    // 1010101110010110 masked | value
    uint16_t w;
    if (readWord(devAddr, regAddr, (uint8_t *)&w) != 0) {
        uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask; // zero all non-important bits in data
        w &= ~(mask); // zero all important bits in existing word
        w |= data; // combine data with existing word
        return writeWord(devAddr, regAddr, w);
    } else {
        return false;
    }
}

/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
    return writeBytes(devAddr, regAddr, 1, &data);
}

/** Write single word to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New word value to write
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data) {
    return writeWords(devAddr, regAddr, 1, (uint8_t *)&data);
}

/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t*pData) {
	if (!lock())
		return false;
	if (HAL_I2C_Mem_Write(_I2C_Handle, devAddr, regAddr,
			I2C_MEMADD_SIZE_8BIT, pData, length, 500) != HAL_OK) {

		I2C_Unstick();
		unlock();
		return false;
	}

	unlock();
	return true;
}

/** Write multiple words to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of words to write
 * @param data Buffer to copy new data from
 * @return Status of operation (true = success)
 */
bool FRToSI2C::writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t*pData) {
	if (!lock())		//尝试获取_I2CSemaphore
		return false;

	if (HAL_I2C_Mem_Write(_I2C_Handle, devAddr, regAddr,
			I2C_MEMADD_SIZE_16BIT, pData, length, 500) != HAL_OK) {

		I2C_Unstick();	//传输出错会执行
		unlock();		//释放_I2CSemaphore
		return false;
	}

	unlock();			//释放_I2CSemaphore
	return true;
}

/**
 * @brief  非阻塞模式下使用DMA在发送大量数据
 * 		   唯一使用场景：OLED::refresh()
 * @param  devAddr: 	I2C从设备地址
 * @param  pData: 		SRAM中待传输的数据地址
 * @param  length: 		数据大小
 * @retval bool
 */
bool FRToSI2C::Master_Transmit_DMA(uint16_t devAddr, uint8_t *pData, uint16_t length) {
	if (!lock()) {
		//usb_printf("locked! Can't Transmit\r\n");
		return false;
	}
	if (HAL_I2C_Master_Transmit_DMA(_I2C_Handle, devAddr, pData, length)!= HAL_OK) {
		I2C_Unstick();
		unlock();
		//usb_printf("!= HAL_OK, Can't Transmit\r\n");
		return false;
	}

	//usb_printf("Transmit successed!\r\n");
	return true;
}

/**
 * @brief  非阻塞模式下使用DMA接受大量数据
 * 		   唯一使用场景：OLED::refresh()
 * @param  devAddr: 	I2C从设备地址
 * @param  pData: 		SRAM中待传输的数据地址
 * @param  length: 		数据大小
 * @retval bool
 */
bool FRToSI2C::Master_Receive_DMA(uint16_t devAddr, uint8_t *pData, uint16_t length) {
	if (!lock()) {
		//usb_printf("locked! Can't Transmit\r\n");
		return false;
	}
	if (HAL_I2C_Master_Receive_DMA(_I2C_Handle, devAddr, pData, length)
			!= HAL_OK) {
		I2C_Unstick();
		unlock();
		//usb_printf("!= HAL_OK, Can't Transmit\r\n");
		return false;
	}

	//usb_printf("Transmit successed!\r\n");
	return true;
}


bool FRToSI2C::Master_Transmit(uint16_t devAddr, uint8_t *pData,
		uint16_t length) {
	if (!lock()) {
		//usb_printf("locked! Can't Transmit\r\n");
		return false;
	}
	if (HAL_I2C_Master_Transmit(_I2C_Handle, devAddr, pData, length,
			HAL_MAX_DELAY) != HAL_OK) {
		I2C_Unstick();
		unlock();
		//usb_printf("!= HAL_OK, Can't Transmit\r\n");
		return false;
	}

	//usb_printf("Transmit successed!\r\n");
	return true;
}

bool FRToSI2C::Master_Receive(uint16_t devAddr, uint8_t *pData,
		uint16_t length) {
	if (!lock()) {
		//usb_printf("locked! Can't Transmit\r\n");
		return false;
	}
	if (HAL_I2C_Master_Receive(_I2C_Handle, devAddr, pData, length,
			HAL_MAX_DELAY) != HAL_OK) {
		I2C_Unstick();
		unlock();
//		usb_printf("!= HAL_OK, Can't Transmit\r\n");
		return false;
	}

	//usb_printf("Transmit successed!\r\n");
	return true;
}

/**
 * @brief  探测I2C1指定地址上有没有设备
 * @param  devAddr: Target device address: The device 7 bits address value
 *         in datasheet must be shifted to the left before calling the interface
 *         7bit地址必须要左移一位，示例：
 *         #define MPU6050_ADDRESS (0x68 << 1)
 *         FRToSI2C1.probe(MPU6050_ADDRESS)
 * @retval bool
 */
bool FRToSI2C::probe(uint16_t devAddr) {
	//若获取_I2CSemaphore失败
	if (!lock())
		return false;	//返回false，终止本函数

	//若成功获取_I2CSemaphore则执行以下代码
//	uint8_t buffer[1];
//	bool worked = HAL_I2C_Mem_Read(_I2C_Handle, devAddr, 0x0F,
//			I2C_MEMADD_SIZE_8BIT, buffer, 1, 1000) == HAL_OK;
	bool worked = HAL_I2C_Master_Transmit(_I2C_Handle, devAddr << 1, 0, 0, 200);
	unlock();
	return worked;
}

//若I2C1出错会使用此方法终止当前传输, 并重置I2C
//典型使用场景：if(HAL_I2C通信函数!= HAL_OK)，执行I2C_Unstick()
void FRToSI2C::I2C_Unstick() {
	unstick_I2C(_I2C_Handle);
}

//释放_I2CSemaphore (前提FRToI2C::lock成功获取_I2CSemaphore)
void FRToSI2C::unlock() {
#if RTOS_EN
	xSemaphoreGive(_I2CSemaphore);
#else
	_I2CSemaphore = 1;
#endif
}

//尝试获取_I2CSemaphore
bool FRToSI2C::lock() {
	bool mark = 0;
#if RTOS_EN
	mark = (xSemaphoreTake(_I2CSemaphore, (TickType_t)1000) == pdTRUE);
#else
	mark = _I2CSemaphore;		//HAL_I2C回调函数执行完毕后，_I2CSemaphore = 1
#endif
	return mark;
}

/**
 * @brief  向I2C设备写入FRToSI2C::I2C_REG结构体数组
 * 		   典型场景：初始化I2C设备配置多个寄存器
 * @param  address: 		I2C从设备地址
 * @param  registers: 		数组地址
 * @param  registersLength: 数组元素大小(sizeof()传入)
 * @retval bool
 */
bool FRToSI2C::writeRegistersBulk(const uint8_t address,
		const I2C_REG *registers, const uint8_t registersLength) {
	for (int index = 0; index < registersLength; index++) {
		//I2C_RegisterWrite()在阻塞模式下向1个寄存器写入1个8bit数据
		if (!I2C_RegisterWrite(address, registers[index].reg,
				registers[index].val)) {
			return false;
		}
		if (registers[index].pause_ms)
			HAL_Delay(registers[index].pause_ms);
	}
	return true;
}
