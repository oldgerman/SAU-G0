/*!
 * @file DFRobot_AHT20.cpp
 * @brief This AHT20 temperature & humidity sensor employs digital output and I2C interface, through which users can read the measured temperature and humidity. The AHT20 chip-based sensor offers the following features:
 * @n 1. Collect ambient temperature, unit Celsius (℃), range -40-85℃, resolution: 0.01, error: ±0.3-±1.6℃
 * @n 2. Collect ambient relative humidity, unit: %RH, range 0-100%RH, resolution 0.024%RH, error: when the temprature is 25℃, error range is ±2-±5%RH
 * @n 3. Use I2C interface, I2C address is default to be 0x38
 * @n 4. uA level sensor, the measured value is up to 200uA.
 * @n 5. Power supply range 3.3-5V
 *
 * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author [Arya](xue.peng@dfrobot.com)
 * @version  V1.0
 * @date  2022-02-09
 * @url https://github.com/DFRobot/DFRobot_AHT20
 *
 * @Modify: OldGerman 2022/05/16
 */

#include "DFRobot_AHT20.h"
#include "string.h"		//提供memset()
#include "stdlib.h"		//提供malloc()

//初始化状态机状态
//DFRobot_AHT20::measurement_state  DFRobot_AHT20::state = MEASCheckStatus;

// DBG修改为DBG_AHT
// DBG与STM32g031xx.h的冲突 #define DBG              ((DBG_TypeDef *) DBG_BASE)
/*
 * 注意 搞 DBG_AHT 时，函数里加入的打印字符串会显著消耗任务栈空间的
 * 分配的TaskBuffer 128字的空间 不开 DBG_AHT足够，但开了就不够，执行到
 * hardfault_hander(), 看msp指针回退RAM到flash地址发现还是执行到configASSERT( xRerun != errCOULD_NOT_ALLOCATE_REQUIRED_MEMORY);
 * 加大TaskBuffer到256字就正常了
 */
#ifndef DBG_AHT
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_AHT usb_printf
#else
	#define DBG_AHT(...)
	#endif
#endif

#define AHT20_CRC_EN  1 //< Change 0 to 1 to Enable AHT20 CRC
#if AHT20_CRC_EN
#define CMD_MEASUREMENT_DATA_LEN     7     ///< Return length when the measurement command is without CRC check.
#else
#define CMD_MEASUREMENT_DATA_LEN     6     ///< Return data length when the measurement command is with CRC check.
#endif

#define AHT20_DEF_I2C_ADDR           0x38  ///< Default I2C address of AHT20 sensor 
#define CMD_INIT                     0xBE  ///< Init command
#define CMD_INIT_PARAMS_1ST          0x08  ///< The first parameter of init command: 0x08
#define CMD_INIT_PARAMS_2ND          0x00  ///< The second parameter of init command: 0x00
#define CMD_INIT_TIME                20    ///< Waiting time for init completion: 20ms
#define CMD_MEASUREMENT              0xAC  ///< Trigger measurement command
#define CMD_MEASUREMENT_PARAMS_1ST   0x33  ///< The first parameter of trigger measurement command: 0x33
#define CMD_MEASUREMENT_PARAMS_2ND   0x00  ///< The second parameter of trigger measurement command: 0x00
#define CMD_MEASUREMENT_TIME         80    ///< Measurement command completion time: 80ms
#define CMD_SOFT_RESET               0xBA  ///< Soft reset command
#define CMD_SOFT_RESET_TIME          20    ///< Soft reset time: 20ms
#define CMD_STATUS                   0x71  ///< Get status word command

union uStatusBit_t {
	struct {
		uint8_t rsv0 :3; /**< reserve bits */
		uint8_t calEnable :1; /**< calibrate enble bit: 1 - calibrated, 0 - not calibrated */
		uint8_t rsv1 :3; /**< reserve bits */
		uint8_t busy :1; /**< Busy indication 1 - the device is busy, in measurement status, 0 - the device is idle, in hibernation status */
	};
	uint8_t status; /**< status bit */
};

DFRobot_AHT20::DFRobot_AHT20(FRToSI2C *pFRToSI2C)
//成员初始化列表
:_pFRToSI2C(pFRToSI2C), _addr(AHT20_DEF_I2C_ADDR), _temperature(0.0), _humidity(
				0.0), _init(false) {
	state = MEASCheckStatus;
}

uint8_t DFRobot_AHT20::begin() {
	//step1: Determine whether _pFRToSI2C is valid, if not, return 1, if valid, initialize the bus
	if (!_pFRToSI2C) {
		DBG_AHT("_pFRToSI2C is NULL!\r\n");
		return 1;
	}
	//step2: Determine whether the device is connected, return 2 if not connected
	if (!_init) {
		uint8_t ret = _pFRToSI2C->probe(_addr << 1);
		if (ret == 0) {
			DBG_AHT("Device not found, please check if the device is connected.\r\n");
			return 2;
		}
	}
	//step3: Init, if it failed, return 3
	if (!init()) {
		DBG_AHT("SHT20 initialization fail.\r\n");
		return 3;
	}
	//If AHT20 init succeeded, return 0
	_init = true;
	return 0;

}

void DFRobot_AHT20::reset() {
	uint8_t ret = _pFRToSI2C->probe(_addr << 1);
	if (ret == 0)
		DBG_AHT("Device not found, please check if the device is connected.\r\n");

	uint8_t data = CMD_SOFT_RESET;
	_pFRToSI2C->Master_Transmit(_addr << 1, &data, 1);
	writeCommand(CMD_SOFT_RESET);
	HAL_Delay(CMD_SOFT_RESET_TIME);
	//getStatusData();
	_init = true;
}

DFRobot_AHT20::measurement_state DFRobot_AHT20::meas_check_status() {
	//如果状态是已自动校准并且 准备测量了
	if (!ready()) {
		DBG_AHT("Not cailibration.\r\n");
	}
	else {
		//发送测量命令
		if(writeCommand(CMD_MEASUREMENT, CMD_MEASUREMENT_PARAMS_1ST,
		CMD_MEASUREMENT_PARAMS_2ND))
			return MEASWaitProcess;
	}
	return MEASCheckStatus;
}

DFRobot_AHT20::measurement_state DFRobot_AHT20::meas_wait_process(){
	//等待传感器自己测量的时间80ms
	//HAL_Delay(CMD_MEASUREMENT_TIME);
	osDelay(CMD_MEASUREMENT_TIME);
	return MEASReadData;
}

DFRobot_AHT20::measurement_state DFRobot_AHT20::meas_read_data(uint8_t* pData, uStatusBit_t *status){
	//获取连续多个的测量数据寄存器值
	if(_pFRToSI2C->Master_Receive(_addr << 1, pData, CMD_MEASUREMENT_DATA_LEN)){
		for (int i = 0; i < CMD_MEASUREMENT_DATA_LEN; i++) {
			DBG_AHT("pData = 0x%02X\r\n",pData[i]);
		}
		//获得值后，还是要检查值的状态寄存器的值是不是busy
		status->status = pData[0];
		if (status->busy) {
			DBG_AHT("AHT20 is busy!\r\n");
		}
		else
			return MEASCalculate;
	}
	return MEASCheckStatus;
}

DFRobot_AHT20::measurement_state DFRobot_AHT20::meas_calculate(uint8_t* pData, bool *measCompleted){
		//进行CRC校验
	if (AHT20_CRC_EN && !checkCRC8(pData[6], &pData[0], CMD_MEASUREMENT_DATA_LEN - 1)) {
		DBG_AHT("CRC check failed.\r\n");
	}
	else{
		//CRC校验正确，计算温度
		uint32_t temp;
		temp = pData[1];
		temp <<= 8;
		temp |= pData[2];
		temp <<= 4;
		temp = temp | (pData[3] >> 4);
		_humidity = (uint32_t) (temp * 100) / (uint32_t) 0x100000;

		temp = pData[3] & 0x0F;
		temp <<= 8;
		temp |= pData[4];
		temp <<= 8;
		temp |= pData[5];
		_temperature = (uint32_t) (temp * 2000) / (uint32_t) 0x100000 - 500;

		*measCompleted = true;
	}
	return MEASCheckStatus;
}

//每次任务调用此函数时，状态机执行一种状态对应的函数，多次调用本任务而执行完原本的一套测量流程长任务
bool DFRobot_AHT20::measurementFSM(){
	// Loop based on state
	//状态机循环
	static uint8_t pData[CMD_MEASUREMENT_DATA_LEN] = {0};
	static uStatusBit_t status;
	bool measCompleted = false;

	switch (state) {
		case MEASCheckStatus:
			state = meas_check_status();
			break;
		case MEASWaitProcess:
			state = meas_wait_process();
			break;
		case MEASReadData:
			state = meas_read_data(pData, &status);
			break;
		case MEASCalculate:
			state = meas_calculate(pData, &measCompleted);
			break;
		default:
			state = MEASCheckStatus;
			break;
	}
	return measCompleted;
}

uint32_t DFRobot_AHT20::getTemperature_F() {
//	return _temperature * 1.8 + 32;
	return (_temperature * 18 + 320)/10;
}
uint32_t DFRobot_AHT20::getTemperature_C() {
	return _temperature;
}
uint32_t DFRobot_AHT20::getHumidity_RH() {
	return _humidity;
}

bool DFRobot_AHT20::checkCRC8(uint8_t crc8, uint8_t *pData, uint8_t len) {
	//CRC initial value: 0xFF
	//CRC8 check polynomial: CRC[7: 0] = X8 + X5 + X4 + 1  -  0x1 0011 0001 - 0x131
	uint8_t crc = 0xFF;
	for (int pos = 0; pos < len; pos++) {
		crc ^= pData[pos];
		for (int i = 8; i > 0; i--) {
			if (crc & 0x80) {
				crc <<= 1;
				crc ^= 0x31;
			} else {
				crc <<= 1;
			}
		}
	}
	DBG_AHT("crc = 0x%02X\r\n",crc);
	if (crc8 == crc)
		return true;
	return false;
}

bool DFRobot_AHT20::ready() {
	union uStatusBit_t status;
	status.status = getStatusData();
	if (status.busy)
		return false;
	return true;
}

bool DFRobot_AHT20::init() {
	union uStatusBit_t status;
	status.status = getStatusData();
	if (status.calEnable)
		return true;

	writeCommand(CMD_INIT, CMD_INIT_PARAMS_1ST, CMD_INIT_PARAMS_2ND);
	HAL_Delay(CMD_INIT_TIME);
	status.status = getStatusData();
	if (status.calEnable)
		return true;

	return false;
}

uint8_t DFRobot_AHT20::getStatusData() {
	uint8_t status = 0;
	readData(CMD_STATUS, &status, 1);
	DBG_AHT("status = 0x%02X\r\n",status);
	return status;
}

bool DFRobot_AHT20::writeCommand(uint8_t cmd) {
	return _pFRToSI2C->Master_Transmit(_addr << 1, &cmd, 1);
}

bool DFRobot_AHT20::writeCommand(uint8_t cmd, uint8_t args1, uint8_t args2) {
	uint8_t buffer[2];
	buffer[0] = args1;
	buffer[1] = args2;

	return _pFRToSI2C->writeBytes(_addr << 1, cmd, 2, buffer);
}

bool DFRobot_AHT20::readData(uint8_t cmd, void *pBuf, size_t size) {
	if (pBuf == NULL) {
		DBG_AHT("pBuf ERROR!! : null pointer\r\n");
		return 0;
	}
	uint8_t *_pBuf = (uint8_t*) pBuf;
	return _pFRToSI2C->readBytes(_addr << 1, cmd, size, _pBuf);
}

#if 0

//这是一个典型的长任务
bool DFRobot_AHT20::startMeasurementReady() {
	uint8_t recvLen = CMD_MEASUREMENT_DATA_LEN;
	uint8_t pData[recvLen] = {0};
	uint32_t temp = 0;
	uStatusBit_t status;

	//如果状态是已自动校准并且 准备测量了
	if (!ready()) {
		DBG_AHT("Not cailibration.\r\n");
		return false;
	}

	//发送测量命令
	writeCommand(CMD_MEASUREMENT, CMD_MEASUREMENT_PARAMS_1ST,
	CMD_MEASUREMENT_PARAMS_2ND);
	//getStatusData();

	//阻塞！等待测量的时间80ms，靠！
	//到底是HAL还是osDelay，要不要将这个长任务打断？？
//	HAL_Delay(CMD_MEASUREMENT_TIME);
	osDelay(CMD_MEASUREMENT_TIME);

	//获取连续多个的测量数据寄存器值
	_pFRToSI2C->Master_Receive(_addr << 1, pData, recvLen);
	for (int i = 0; i < recvLen; i++) {
		DBG_AHT("pData = 0x%02X\r\n",pData[i]);
	}

	//获得值后，还是要检查值的状态寄存器的值是不是busy
	status.status = pData[0];
	if (status.busy) {
		DBG_AHT("AHT20 is busy!\r\n");
		return false;
	}

	//进行CRC校验
	if (AHT20_CRC_EN && !checkCRC8(pData[6], &pData[0], CMD_MEASUREMENT_DATA_LEN - 1)) {
		DBG_AHT("CRC check failed.\r\n");
		return false;
	}

	//计算温度
	temp = pData[1];
	temp <<= 8;
	temp |= pData[2];
	temp <<= 4;
	temp = temp | (pData[3] >> 4);
	_humidity = (uint32_t) (temp * 100) / (uint32_t) 0x100000;

	temp = pData[3] & 0x0F;
	temp <<= 8;
	temp |= pData[4];
	temp <<= 8;
	temp |= pData[5];
	_temperature = (uint32_t) (temp * 2000) / (uint32_t) 0x100000 - 500;
	return true;
}
#endif
