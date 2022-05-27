## 2022/05/27  15:40

去掉FreeRTOS，改用X-TRACK里的[MillisTaskManager](https://github.com/FASTSHIFT/X-TRACK/tree/main/Software/X-Track/Libraries/MillisTaskManager)库

### 内存分配及占用

RAM包含堆栈各0x400（1KB）

![2022-0527-1540-内存分配及占用_RAM包含堆栈各0x400（1KB）](2022-0527-1540-内存分配及占用_RAM包含堆栈各0x400（1KB）.png)

### 同时运行

![同时运行的任务](同时运行的任务.png)

单按键开机、长按关机正常

运动检测正常

温湿度正常

RTC正常

串口修改RTC时间正常

按键 Buttons所有状态测试正常

所有多级菜单`vector<Colum>`创建正常，成功跑起来多级菜单

### Page类

ODGIRON烙铁 旧版本代码 跑三个以上的栏会有bug

改成支持不限制多少个的了

## 2022/05/27  21:04

### 内存分配及占用

RAM包含堆栈各0x400（1KB）

![2022-0527-2104_内存分配及占用_RAM包含堆栈各0x400（1KB）](2022-0527-2104_内存分配及占用_RAM包含堆栈各0x400（1KB）.png)

### 功能正常

### Page类

添加遮罩矩形，用于Colum的str与val和unit显示重叠时的情况，可以在修改值时遮罩住一部分str中文字符，非修改模式，可以同时长短或长按左右键，快速显示本页由于str太长而不显示的val和unit部分
