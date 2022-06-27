#ifndef	_EE24CONFIG_H
#define	_EE24CONFIG_H

#ifndef DBG_EE24
#if 0
#define DBG_EE24 usb_printf
#else
#define DBG_EE24(...)
#endif
#endif

//单位 Kbit
#define I2C_DEVICESIZE_24LC1024    1024
#define I2C_DEVICESIZE_24LC512      512
#define I2C_DEVICESIZE_24LC256      256
#define I2C_DEVICESIZE_24LC128      128
#define I2C_DEVICESIZE_24LC64        64
#define I2C_DEVICESIZE_24LC32        32
#define I2C_DEVICESIZE_24LC16        16
#define I2C_DEVICESIZE_24LC08         8
#define I2C_DEVICESIZE_24LC04         4
#define I2C_DEVICESIZE_24LC02         2
#define I2C_DEVICESIZE_24LC01         1



#define		_EEPROM_USE_FREERTOS        0
#define		_EEPROM_ADDRESS             0xA0
#define		_EEPROM_USE_WP_PIN          0
#define    _EEPROM_EXC_PINS            1
#if (_EEPROM_USE_WP_PIN==1)
#define		_EEPROM_WP_GPIO								EE_WP_GPIO_Port
#define		_EEPROM_WP_PIN								EE_WP_Pin
#endif

#endif

