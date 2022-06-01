/*
 * CustomPageObject.cpp
 *
 *  Created on: May 25, 2022
 *      Author: OldGerman (过气德国佬)
 */
#include "CustomPage.hpp"
#include <string.h>
#include "stdio.h"
#include "dtostrf.h"

//#pragma GCC push_options
//#pragma GCC optimize ("Os")	//Os优化无Bug

/******************** 构造二级或三级菜单Colum或Page对象 ********************/
//数据采集-->开始日期
std::vector<Colum> columsDataCollectSTDateTime = {
		Colum("总览", columsDataCollect_ScheduleSetting_STDateTime, LOC_ENTER),
		Colum("年", &systemSto.data.STyy, 2, 99, 0, 1, 10),
		Colum("月", &systemSto.data.STMM, 2, 12, 1, 1, 10),
		Colum("日", &systemSto.data.STdd, 2, 31, 1, 1, 10),
		Colum("时", &systemSto.data.SThh, 2, 23, 0, 1, 10),
		Colum("分", &systemSto.data.STmm, 2, 59, 0, 1, 10),
		Colum("秒", &systemSto.data.STss, 2, 59, 0, 1, 10)
};

Page pageDataCollectSTDateTime(&columsDataCollectSTDateTime);

//数据采集-->任务设置-->采集对象
std::vector<Colum> columsDataCollectObj = {
		Colum("停止标志", 	&systemSto.data.settingsBits[colBits], B00000001),	// RTC OSF (OSC STOP FLAG)
		Colum("温度", 		&systemSto.data.settingsBits[colBits], B00000010),
		Colum("湿度", 		&systemSto.data.settingsBits[colBits], B00000100),
		Colum("气压", 		&systemSto.data.settingsBits[colBits], B00001000),
		Colum("照度", 		&systemSto.data.settingsBits[colBits], B00010000),
		Colum("电池电压", 	&systemSto.data.settingsBits[colBits], B00100000)
};
Page pageDataCollectObj(&columsDataCollectObj);

//数据采集-->任务设置
std::vector<Colum> columsDataCollect_ScheduleSetting = {
		Colum("采集对象", &pageDataCollectObj),
		Colum("样本数", &systemSto.data.NumDataSamples, 2, 99, 1, 1, 10),
		Colum("每天次数", &systemSto.data.NumDataOneDay, 4, Sec24H / 10, 1, 1, 10, nullptr, columsDataCollect_ScheduleSetting_NumDataOneDay, LOC_CHANGE),						//后台约束对齐到整除点
		Colum("总次数", &systemSto.data.NumDataWillCollect, 4, 9999, 1, 1, 10, nullptr, columsDataCollect_ScheduleSetting_NumDataWillCollect, LOC_CHANGE),				//实时约束到最大容量
		Colum("开始日期", &pageDataCollectSTDateTime),
		Colum("结束日期", columsDataCollect_ScheduleSetting_EDDateTime, LOC_ENTER)			//	自动计算出
};

Page pageDataCollect_ScheduleSetting(&columsDataCollect_ScheduleSetting);

//数据采集
std::vector<Colum> columsDataCollect = {
		Colum("任务设置", &pageDataCollect_ScheduleSetting),
		Colum("任务进度", columsDataCollected_Schedule),
		Colum("任务开关", &systemSto.data.settingsBits[colBits], B10000000),
		Colum("删除任务", colum_FeaturesUnrealized),
		Colum("清除记录", colum_FeaturesUnrealized)		//ee24_eraseChip();	//不能全部清除，只清空EEPROM的采集数据开始的位到最后的地址
};

//日期时间
std::vector<Colum> columsDateTime = {
		Colum("年", &systemSto.data.yy, 2, 99, 0, 1, 10),
		Colum("月", &systemSto.data.MM, 2, 12, 1, 1, 10),
		Colum("日", &systemSto.data.dd, 2, 31, 1, 1, 10),
		Colum("时", &systemSto.data.hh, 2, 23, 0, 1, 10),
		Colum("分", &systemSto.data.mm, 2, 59, 0, 1, 10),
		Colum("秒", &systemSto.data.ss, 2, 59, 0, 1, 10),
		Colum("更改时间", columsDateTime_ChangeDateTime, LOC_ENTER)	//这里也要检查时间有效性
};

//显示设置
/* 这个页的Coulms比正常的3少一个*/
std::vector<Colum> columsDisplaySettings = {
		Colum("屏幕亮度", &systemSto.data.ScreenBrightness, 3, 100, 1, 1, 10, "%", columsScreenSettings_Brightness, LOC_CHANGE),
		Colum("开机图标", &systemSto.data.settingsBits[sysBits], B10000000)
};

//休眠唤醒
std::vector<Colum> columsScreenOffAndWKUP = {
		Colum("动作阈值", &systemSto.data.Sensitivity, 3, 100, 1, 1, 10, "%"),
		Colum("自动休眠", &systemSto.data.settingsBits[sysBits], B00000001),
		Colum("时间阈值", &systemSto.data.SleepTime, 3, 900, 0, 1, 10, "S")	//最多亮屏15分钟
};

//辅助功能
std::vector<Colum> columsAccessibility = {
		Colum("扫描设备", 	columsAccessibility_I2CScaner, LOC_ENTER),
		Colum("电池信息", 	columsAccessibility_Battery, LOC_ENTER),
		Colum("重启", 		columsHome_Reset, LOC_ENTER),
		Colum("恢复出厂",	columsAccessibility_ResetSettings, LOC_ENTER)
};

/******************** 构造二级菜单Page对象 ********************/
Page pageDataCollect(&columsDataCollect);
Page pageDateTime(&columsDateTime);
Page pageDisplaySettings(&columsDisplaySettings);
Page pageScreenOffAndWKUP(&columsScreenOffAndWKUP);
Page pageAccessibility(&columsAccessibility);

/******************** 构造一级菜单Colum对象 ********************/
std::vector<Colum> columsHome = {
		Colum("数据采集", &pageDataCollect),
		Colum("日期时间", &pageDateTime),
		Colum("显示设置", &pageDisplaySettings),
		Colum("休眠唤醒", &pageScreenOffAndWKUP),
		Colum("辅助功能", &pageAccessibility),
		Colum("版本信息", columsHome_ShowVerInfo)
};

/******************** 构造一级菜单Page对象 ********************/
Page pageHome(&columsHome);

/******************** 初始化Page对象的static数据成员 ********************/
Page *Page::ptrPage = &pageHome;	//初始化共用Page指针为起始页
Page *Page::ptrPagePrev;// = &pageHome;	//初始化上一次的Page指针为起始页
std::list<Page *> Page::ptrPageList;
Page *Page::homePage = &pageHome;
bool Page::timeOut = false;
uint16_t indexColumsValue = 0;

/*
 * Page::indexColums初始化
 * 备注：
 * 用于索引当前选中的colum在oled上的y坐标位置
 * 参数 places = 2： 		暂时用不到，是指AutoValue的val的十进制下的位数，范围是0~32，填两位好了
 * 参数 upper = 32： 		即oled 64x48分辨率 一页在y放向（0，16，32）上显示三组colum，最大的y坐标为32
 * 参数 lower =  0： 		即oled 64x48分辨率 一页在y放向（0，16，32）上显示三组colum，最小的y坐标为0
 * 参数 shortSteps = 16: 	即每个Colum y坐标间隔为16像素
 * 参数 slongSteps = 0：	没有用到，给0好了
 * 参数 ValueCycle = flase：不使用AutoValue的循环逻辑，循环菜单在Page::flashPage的switch部分实现
 */
AutoValue Page::indexColums(&indexColumsValue, 2, 32, 0, 16, 0, false);
uint8_t Page::indexColumsUpperFristIn;
/*
 * Page::cntIndexColumsOffsset初始化
 * 备注：
 * 用于flashPage的switch()内迭代IndexCoums时，菜单循环情况的IndexColums的迭代次数，该值为Oled每页显示Colum的个数-1，页可以通过以下方法自动计算
 */
const uint8_t Page::cntIndexColumsOffsset = (Page::indexColums.upper / Page::indexColums.shortSteps); //等于2


uint8_t Page::valIndex = 0;
//uint8_t Page::bbb = 0;			//按钮状态，debug
//#pragma GCC pop_options
