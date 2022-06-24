/*
 * BSP_USART.cpp
 *
 *  Created on: 2022年6月24日
 *      Author: OldGerman
 */

#include "BSP.h"
#include "RTClib.h"
#include <stdio.h>   	//提供vsnprintf()
#include <stdarg.h> 	//提供va_start(); va_end();

#ifndef DBG_PRINT_USART
#if 0  //< Change 0 to 1 to open debug macro and check program debug information
#define DBG_PRINT_USART usb_printf
#else
#define DBG_PRINT_USART(...)
#endif
#endif

#define DBG_COM_DATE_TIME_ADJ 0					////< Change 0 to 1 to open debug
#if DBG_COM_DATE_TIME_ADJ
char testSaveBuffer[17] = {
		'2', '2', '/', 						    //2022年
		'0', '5','/', '2', '2', '/', 			//5月22日
		'2', '1','/', '2', '6', '/', '4', '7'	//21时26分47秒
};
#endif

#define TX_BUFFER_SIZE 64
#define RX_BUFFER_SIZE 64

uint8_t u2Print(uint8_t *sendBuff, uint16_t length);
void usb_printf_IT(const char *format, ...);
void usb_printf(const char *format, ...);
void i2cScan(I2C_HandleTypeDef *hi2c, uint8_t i2cBusNum);
void I2cScan_Update(uint16_t ms);
void USART_CheckDateTime();
static void uart_service(uint16_t ms);

extern const char daysOfTheWeek[7][12];
bool nowChangedByUSART;
uintDateTime  nowRXbufferToNum;
uint8_t aRxTemp[2];
uint8_t aRxBuffer[RX_BUFFER_SIZE];
uint8_t TxBuffer[TX_BUFFER_SIZE];
uint8_t aRxSaveBuffer[RX_BUFFER_SIZE];
volatile uint8_t rxSaveCounter = 0;
volatile uint8_t txBusy = 0;
volatile uint8_t rxDone = 0;
volatile uint8_t rxBusy = 0;
volatile uint8_t rxCounter = 0;
volatile uint32_t rxTick = 0;

/**
 * usb_printf()的非阻塞版本
 */
void usb_printf_IT(const char *format, ...) {
	va_list args;
	uint32_t length;
	va_start(args, format);
	length = vsnprintf((char*) TxBuffer, TX_BUFFER_SIZE, (char*) format, args);
	va_end(args);

	u2Print(TxBuffer, length);
}

/**
 * 从https://blog.csdn.net/u010779035/article/details/104369515修改
 */
void usb_printf(const char *format, ...) {
	va_list args;
	uint32_t length;
	va_start(args, format);
	length = vsnprintf((char*) TxBuffer, TX_BUFFER_SIZE, (char*) format, args);
	va_end(args);
	HAL_UART_Transmit(&huart2, TxBuffer, length, 1000);
}

/**
 * @brief 以非阻塞模式发送数据到USRAT2
 * 			会回调重写的HAL_UART_TxCpltCallback()回调函数
 * 			length通常是用rxSaveCounter计数器的值
 * 			单独使用此函数也行，但一般配合串口接收中断用
 * 			单独使用建议使用usb_printf_IT()
 * 			Send message to UART2 peripheral.
 * @param {uint8_t}*sendBuff Buffer to send
 * @param {uint16_t}length Message length, 0 for const
 * @return {uint8_t} error code 1 for uart and Double Buffering busy, 2 for
 * empty or too long message.
 */
uint8_t u2Print(uint8_t *sendBuff, uint16_t length) {
	if (length == 0)
		return 2;
	if (!txBusy) {
		txBusy = 1;
		HAL_UART_Transmit_IT(&huart2, sendBuff, length);
	} else
		return 1;
	return 0;
}

bool USART_DateTimeUpdated(){
	return nowChangedByUSART;
}

uintDateTime& USART_GetDateTime(){
	return nowRXbufferToNum;
}

void USART_Init()
{
	nowChangedByUSART = false;
	HAL_UART_Receive_IT(&huart2, aRxTemp, 1); //串口接收中断启动函数
}

void USART_Update()
{
#if DBG_COM_DATE_TIME_ADJ
	checkCOMDateTimeAdjustAvailable();
#else
	nowChangedByUSART = false;
	uart_service(HAL_UART_TIMEOUT_VALUE);
	if (rxSaveCounter && (!txBusy)) {
		USART_CheckDateTime();
		u2Print(aRxSaveBuffer, rxSaveCounter);
		rxSaveCounter = 0;
	}
#endif
}




//检查串口键入时间的有效性
void USART_CheckDateTime()
{
#if DBG_COM_DATE_TIME_ADJ
	char* ptr = testSaveBuffer;
#else
	char* ptrBuf = (char*)aRxSaveBuffer;
#endif

	uint16_t* ptrNum = (uint16_t *) &(nowRXbufferToNum.yOff);
	for(int i = 0; i < 6; i++)
		*(ptrNum + i) = 0;

	int j = 0;
	for(int i = 0; i < 17 ; i++)
	{
		if(!isDigit(*ptrBuf))
			j++;
		else
			*(ptrNum + j) =  *(ptrNum + j) * 10 + (*ptrBuf - '0');	//	字符转整形

			//与arduino串口每次接收1个字符不同
		//本程序串口是y用缓冲区接收完整的一个字符串，因此ptr++
		ptrBuf++;
	}

	uint8_t dtIsDigit = 17 - j;	//计算字符串中数字的个数

	bool CheckNumRange = RTC_CheckUintDateTime(&nowRXbufferToNum);

	//检查数字范围和数字个数的有效性
	if(CheckNumRange && dtIsDigit  == 12) {
		usb_printf("Input is valid!\r\n");
		nowChangedByUSART = true;
	}
	else{
		usb_printf("Invalid input! Numbers: %d, the required quantity is 12.\r\n", dtIsDigit);

	}
}

/**
 * @brief uart service, call in while loop
 * @param {*}
 * @return {*}
 */
static void uart_service(uint16_t ms) {
	if (rxDone || (rxBusy && HAL_GetTick() - rxTick > ms)) {
		rxDone = 1;
		if (rxSaveCounter) return;
		memcpy(aRxSaveBuffer, aRxBuffer, rxCounter);
		rxSaveCounter = rxCounter;
		rxCounter = 0;
		rxBusy = 0;
		rxDone = 0;
	}
}

/**
 * @brief  串口发送总断回调函数
 * 			解除busy标签
 * @param {UART_HandleTypeDef} *huart
 * @return {*}
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
	txBusy = 0;
}

/**
 * @brief  Rx Transfer Uploadd callbacks.
 * @param  huart  Pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval None
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	if (huart->Instance == USART2) {
		if (!rxDone) {  //丢弃数据
			rxBusy = 1;
			rxTick = HAL_GetTick();
			aRxBuffer[rxCounter++] = aRxTemp[0];
			if (rxCounter > RX_BUFFER_SIZE) {
				rxDone = 1;
			}
		}
		HAL_UART_Receive_IT(&huart2, aRxTemp, 1);
	}
}

