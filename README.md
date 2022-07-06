[TOC]

## 1. 简介

SAU-G0（Sensor Acquisition Uinit（传感器采集单元），基于G031G8U6），是本人学习电池供电型低功耗应用的第一个作品

PCB采用叠板设计，分为主控板与拓展板，具体应用取决于拓展板上的传感器和存储芯片

![](Images/MindMap_main.png)

## 2. 极限续航参数

使用CubeMX tools工具粗略计算（未引入唤醒瞬间的电流）：1%的时间运行，99%的时间休眠，每天运行864秒（屏幕亮度为1%），其他时间休眠，那么100mA锂电池极限续航2个月7天

如果每10分钟采集温湿度数据一次，每天采集144次，每次5秒，每天耗时720秒，还剩余144秒可支持每天以1%的亮度亮屏2分钟

每组带两位小数的温湿度数据占用4byte（16bit + 16bit），每天储存576byte数据，2个月采集温湿度数据8640组，占用34560byte空间（需使用24C512 EEPROM（65536byte）存储）

![续航计算1(只要运行OLED一直点亮)](Images/续航计算1(只要运行OLED一直点亮).png)

## 3. PCB

### SAU-G0（主控单元）

- 硬件：STM32G031G8U6 + CH343P + LIS3DH + TPS63000 / TPS63001 + TP4054 / MCP73831-2ATI/OT

| ![SAU-G0_3Dview_top](Images/SAU-G0_3Dview_top.png) | ![SAU-G0_3Dview_bottom](Images/SAU-G0_3Dview_bottom.png) | ![](Images/SAU-G0_2Dview_top.png) |
| -------------------------------------------------- | -------------------------------------------------------- | --------------------------------- |

- USB：由于只有一个Type-C，且休眠时（STOP1模式）固件没有禁用SWD接口功能（为了防止变砖，虽然STOP1模式用不了SWD），实测如果SWD接口用排阻连CH343的D+和D-会消耗200uA以上的电流，所以Type-C采用正反插不同功能，一面USB转TTL串口，另一面面是G031的SWD接口，这样既保留功能也避免了电流泄露
- 串口芯片：可选焊接板载的CH343P，若想外接串口芯片节约成本，可焊接上图CH343P右边的两个焊接跳线，那么G031的USART2就与Type-C直连了
- 有源晶振：默认不焊接，使用内部振荡器，预留给更高精度的时钟应用，例如不使用拓展板的外置RTC，而使用G031内部的RTC，时钟源使用外部32.768Khz有源晶振

### SAU-G0-EX_AHT-GL（拓展版-AHT温湿度计-普通级）

- 硬件：AHT20 + 24Cxx + PCF8563 + WS2812

| ![SAU-G0-EX_AHT-GL_3Dview_2](Images/SAU-G0-EX_AHT-GL_3Dview_2.png) | ![SAU-G0-EX_AHT-GL_3Dview_1](Images/SAU-G0-EX_AHT-GL_3Dview_1.png) | ![SAU-G0-EX_AHT-GL_2Dview_1](Images/SAU-G0-EX_AHT-GL_2Dview_1.png) |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |

### SAU-G0-EX_AHT-PL（拓展版-AHT温湿度计-精准级）

- 硬件：AHT20 + 24Cxx + PCF2129 + WS2812

| ![SAU-G0-EX_AHT-PL_3Dview_2](Images/SAU-G0-EX_AHT-PL_3Dview_2.png) | ![SAU-G0-EX_AHT-PL_3Dview_1](Images/SAU-G0-EX_AHT-PL_3Dview_1.png) | ![SAU-G0-EX_AHT-PL_2Dview_1](Images/SAU-G0-EX_AHT-PL_2Dview_1.png) |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |

### SAU-G0-EX_microSD（拓展版-microSD）

- 硬件：microSD卡或NOR Flash + 12mm蜂鸣片
- 备注：无传感器和RTC芯片，整活儿专用

| ![SAU-G0-EX_microSD_3Dview_2](Images/SAU-G0-EX_microSD_3Dview_2.png) | ![SAU-G0-EX_microSD_3Dview_1](Images/SAU-G0-EX_microSD_3Dview_1.png) | ![SAU-G0-EX_microSD_2Dview_1](Images/SAU-G0-EX_microSD_2Dview_1.png) |
| ------------------------------------------------------------ | ------------------------------------------------------------ | ------------------------------------------------------------ |

## 4. 焊接

SAU-G0主控单元有80多个元件，元件很密集（0402、扁平无引脚封装），十分建议开钢网

## 5. BOM

交互BOM，注意OLED外围电路的几个电容标注了耐压要求，其他的电容默认6.3V耐压就行（最好10V）

## 6. 软件

### 搭建环境

打包的STM32CubeIDE1.7.0工程，需要先安装暗黑主题（[教程](https://blog.csdn.net/qq_42038029/article/details/99735688?utm_medium=distribute.pc_relevant.none-task-blog-title-2&spm=1001.2101.3001.4242)），然后随便折腾

### 功能进度

目前固件已完成对AHT-GL和AHT-PL拓展板适配

- [x] 屏幕渐亮渐暗
- [x] 串口修改时间
- [x] 串口导出EEPROM储存的采集数据
- [x] 多级菜单
- [x] 自定义采集温湿度任务
- [x] 自动检测EEPROM容量：已测试24C02、24C128、24C512
- [x] 休眠：支持RTC、加速度计、按键三种方式唤醒
- [ ] 串口命令解释器

## 7. Acknowledgments

- 感谢豆老板谈下1.6元电子烟主板车车，提供了G0芯片、屏幕以及FPC按键
- 感谢JLC制造的高品质PCB
- 感谢稚晖君Type-C正反插实现调试接口和USB两用的灵感
