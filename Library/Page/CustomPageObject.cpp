/*
 * CustomPageObject.cpp
 *
 *  Created on: May 25, 2022
 *      Author: OldGerman
 */
#include "CustomPage.hpp"
#include <string.h>
//#include "Translation.h"
#include "stdio.h"
//#include "configuration.h"
#include "dtostrf.h"

/******************** 构造二级或三级菜单Colum或Page对象 ********************/
//数据采集--开始日期
std::vector<Colum> columsDataCollectSTDateTime = {
		Colum("年", &eepromSettings.STyy, 2, 99, 0, 1, 10),
		Colum("月", &eepromSettings.STMM, 2, 12, 1, 1, 10), //一旦加减它就溢出
		Colum("日", &eepromSettings.STdd, 2, 31, 1, 1, 10),
		Colum("时", &eepromSettings.SThh, 2, 23, 0, 1, 10),//一旦加减它就溢出
		Colum("分", &eepromSettings.STmm, 2, 59, 0, 1, 10),
		Colum("秒", &eepromSettings.STss, 2, 59, 0, 1, 10) //一旦加减它就溢出
};

Page pageDataCollectSTDateTime(&columsDataCollectSTDateTime);

//数据采集--采集周期
std::vector<Colum> columsDataCollectT = {
		Colum("时", &eepromSettings.Thh, 2, 23, 0, 1, 10),
		Colum("分", &eepromSettings.Tmm, 2, 59, 0, 1, 10),	//一旦加减它就溢出
		Colum("秒", &eepromSettings.Tss, 2, 59, 0, 1, 10)
};

Page pageDataCollectT(&columsDataCollectT);

//数据采集--任务设置
std::vector<Colum> columsDataCollect_ScheduleSetting = {
		Colum("单次样本", &eepromSettings.TSamples, 2, 10, 1, 1, 10),
		Colum("采集周期", &pageDataCollectT),
		Colum("开始日期", &pageDataCollectSTDateTime)	//	级联三级菜单
//		Colum("结束日期", &pageDataCollectxxx)			//	自动计算出
};

Page pageDataCollect_ScheduleSetting(&columsDataCollect_ScheduleSetting);

//数据采集
std::vector<Colum> columsDataCollect = {
		Colum("任务设置", &pageDataCollect_ScheduleSetting),
		Colum("任务进度", columsDataCollected_Schedule),
		Colum("运行任务", &eepromSettings.BinCodeOfEnCollect, 1, 1, 0, 1, 1, " ", coulmBinCodeAdj, LOC_ENTER),	//如何操作位数组元素？用指针获取当前Colum第几个然后识别吧
		Colum("删除任务", colum_FeaturesUnrealized)
};

//日期时间
std::vector<Colum> columsDateTime = {
		Colum("年", &systemSettings.yy, 2, 99, 0, 1, 10),
		Colum("月", &systemSettings.MM, 2, 12, 1, 1, 10),
		Colum("日", &systemSettings.dd, 2, 31, 1, 1, 10),
		Colum("时", &systemSettings.hh, 2, 23, 0, 1, 10),
		Colum("分", &systemSettings.mm, 2, 59, 0, 1, 10),
		Colum("秒", &systemSettings.ss, 2, 59, 0, 1, 10),
		Colum("更改时间", columsDateTime_ChangeDateTime, LOC_ENTER)	//这里也要检查时间有效性
};

//显示设置
/* 这个页的Coulms比正常的3少一个*/
std::vector<Colum> columsDisplaySettings = {
		Colum("屏幕亮度", &systemSettings.ScreenBrightness, 3, 100, 1, 1, 10, "%", columsScreenSettings_Brightness, LOC_CHANGE),
		Colum("开机图标", &systemSettings.PowerOnShowLogo, 1, 1, 0, 1, 1)
};

//休眠唤醒
std::vector<Colum> columsScreenOffAndWKUP = {
		Colum("动作阈值", &systemSettings.Sensitivity, 3, 100, 1, 1, 10, "%"),
		Colum("自动休眠", &systemSettings.SleepEn, 1, 1, 0, 1, 1),
		Colum("时间阈值", &systemSettings.SleepTime, 3, 900, 0, 1, 10, "S")	//最多亮屏15分钟
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

