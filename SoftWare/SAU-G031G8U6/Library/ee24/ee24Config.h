#ifndef	_EE24CONFIG_H
#define	_EE24CONFIG_H

#ifndef DBG_EE24
#if 0
#define DBG_EE24 usb_printf
#else
#define DBG_EE24(...)
#endif
#endif

#define I2C_DEVICESIZE_24LC1024    131072
#define I2C_DEVICESIZE_24LC512      65536
#define I2C_DEVICESIZE_24LC256      32768
#define I2C_DEVICESIZE_24LC128      16384
#define I2C_DEVICESIZE_24LC64        8192
#define I2C_DEVICESIZE_24LC32        4096
#define I2C_DEVICESIZE_24LC16        2048
#define I2C_DEVICESIZE_24LC08        1024
#define I2C_DEVICESIZE_24LC04         512
#define I2C_DEVICESIZE_24LC02         256
#define I2C_DEVICESIZE_24LC01         128



#define		_EEPROM_USE_FREERTOS        0
#define		_EEPROM_ADDRESS             0xA0
#define		_EEPROM_USE_WP_PIN          0
#define    _EEPROM_EXC_PINS            1
#if (_EEPROM_USE_WP_PIN==1)
#define		_EEPROM_WP_GPIO								EE_WP_GPIO_Port
#define		_EEPROM_WP_PIN								EE_WP_Pin
#endif

#endif

