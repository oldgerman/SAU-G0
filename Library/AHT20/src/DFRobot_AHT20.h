/*!
 * @file DFRobot_AHT20.h
 * @brief This AHT20 temperature & humidity sensor employs digital output and I2C interface, through which users can read the measured temperature and humidity. Based on the AHT20 chip, it offers the following features:
 * @n 1. Collect ambient temperature, unit Celsius (℃), range -40-85℃, resolution: 0.01, error: ±0.3-±1.6℃
 * @n 2. Collect ambient relative humidity, unit: %RH, range 0-100%RH, resolution 0.024%RH, error: when the temprature is 25℃, error range is ±2-±5%RH
 * @n 3. Use I2C interface, I2C address is default to be 0x38
 * @n 4. uA level sensor, the measuring current supply is less than 200uA.
 * @n 5. Power supply range 3.3-5V
 *
 * @copyright   Copyright (c) 2010 DFRobot Co.Ltd (http://www.dfrobot.com)
 * @license     The MIT License (MIT)
 * @author [Arya](xue.peng@dfrobot.com)
 * @version  V1.0
 * @date  2022-02-09
 * @url https://github.com/DFRobot/DFRobot_AHT20
 *
 * @Modify: OldGerman
 * 			2022/05/16
 * 				改为G031的HAL库
 * 			2022/05/21
 * 				过长的测量流程函数startMeasurementReady()改为状态机measurementFSM()
 */
#ifndef DFROBOT_AHT20_H
#define DFROBOT_AHT20_H

#ifdef __cplusplus
union uStatusBit_t;
extern "C" {
#endif
#include "main.h"			//引入STM32 HAL库
#ifdef __cplusplus
#include "I2C_Wrapper.h"	//引入I2C读写API
class DFRobot_AHT20{
public:
  /**
   * @fn DFRobot_AHT20
   * @brief DFRobot_AHT20 constructor
   * @param wire TwoWire class object reference
   * @return NONE
   */
  DFRobot_AHT20(FRToSI2C *pFRToSI2C);
  /**
   * @fn begin
   * @brief Initialize AHT20 Sensor
   * @return Init status value
   * @retval  0    Init succeeded
   * @retval  1    _pWire is NULL, please check if the constructor DFRobot_AHT20 has correctly uploaded a TwoWire class object reference
   * @retval  2    Device not found, please check if the connection is correct
   * @retval  3    If the sensor init fails, please check if there is any problem with the sensor, you can call the reset function and re-initialize after restoring the sensor
   */
  uint8_t begin();
  /**
   * @fn reset
   * @brief Sensor soft reset, restore the sensor to the initial status
   * @return NONE
   */
  void reset();
#if 0
  /**
   * @fn startMeasurementReady
   * @brief Start measurement and determine if it's completed.
   * 去掉crcEn参数，默认开启CRC
   * @param crcEn Whether to enable check during measurement
   * @n     false  Measure without check (by default)
   * @n     true   Measure with check
   * @return Whether the measurement is done
   * @retval true  If the measurement is completed, call a related function such as get* to obtain the measured data.
   * @retval false If the measurement failed, the obtained data is the data of last measurement or the initial value 0 if the related function such as get* is called at this time.
   */
  bool startMeasurementReady();
#endif
  /**
   * @fn startMeasurementFSM
   * @brief 测量逻辑的有限状态机
   * 		注意如有必要的临界段处理, 不要有延时会关闭TIM17中断导致系统节拍停止程序卡死
   * @return Whether the measurement is done
   * @retval true  If the measurement is completed, call a related function such as get* to obtain the measured data.
   * @retval false If the measurement failed, the obtained data is the data of last measurement or the initial value 0 if the related function such as get* is called at this time.
   */
  bool measurementFSM();
  /**
   * @fn getTemperature_F
   * @brief Get ambient temperature, unit: Fahrenheit (F).
   * @return Temperature in F
   * @note  AHT20 can't directly get the temp in F, the temp in F is calculated according to the algorithm: F = C x 1.8 + 32
   * @n Users must call the startMeasurementReady function once before calling the function to start the measurement so as to get the real-time measured data,
   * @n otherwise what they obtained is the initial data or the data of last measurement.
   */
  uint32_t getTemperature_F();
  /**
   * @fn getTemperature_C
   * @brief Get ambient temperature, unit: Celsius (℃).
   * @return Temperature in ℃, it's normal data within the range of -40-85℃, otherwise it's wrong data
   * @note Users must call the startMeasurementReady function once before calling the function to start the measurement so as to get the real-time measured data,
   * @n otherwise what they obtained is the initial data or the data of last measurement.
   */
  uint32_t getTemperature_C();
  /**
   * @fn getHumidity_RH
   * @brief Get ambient relative humidity, unit: %RH.
   * @return Relative humidity, range 0-100
   * @note Users must call the startMeasurementReady function once before calling the function to start the measurement so as to get the real-time measured data,
   * @n otherwise what they obtained is the initial data or the data of last measurement.
   */
  uint32_t getHumidity_RH();
protected:
  bool checkCRC8(uint8_t crc8, uint8_t *pData, uint8_t len);
  bool ready();
  bool init();
  uint8_t getStatusData();
  bool writeCommand(uint8_t cmd);
  bool writeCommand(uint8_t cmd, uint8_t args1, uint8_t args2);
  bool readData(uint8_t cmd, void* pBuf, size_t size);
private:
  FRToSI2C *_pFRToSI2C;
  uint8_t _addr;
  uint32_t _temperature;   ///< Last reading's temperature (C)
  uint32_t _humidity;      ///< Last reading's humidity (percent)
  bool _init;

  //测量状态机状态
  enum measurement_state {
      MEASCheckStatus,
      MEASWaitProcess,
      MEASReadData,
      MEASCalculate,
    };
  measurement_state state;
  enum measurement_state meas_check_status();
  enum measurement_state meas_wait_process();
  enum measurement_state meas_read_data(uint8_t* pData, uStatusBit_t *status);
  enum measurement_state meas_calculate(uint8_t* pData, bool *measCompleted);
};
/* extern C++特性的东西 begin*/
/* extern C++特性的东西 end*/
}
#endif
#endif
