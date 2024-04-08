/*
 * Page.hpp
 *
 *  Created on: 2021年4月20日
 *      Author: OldGerman (过气德国佬)
 *      基于C++容器list和vector的多级菜单类，支持任意级联，方便修改菜单结构，不使用数组查表法
 *      菜单的跟随按键状态进行页面跳转、子菜单切换、修改值、支持修改1bit位域
 *
 *      2021/04/20:
 *			用于实现ODGIRON的多级菜单，暂时只支持每页显示两个栏
 *			数据结构简述：
 *				所有Page对象中共享的ptrPageList链表，元素是 Page指针，会跟据当前多级菜单的位置增加或销毁元素
 *				每个page对象私有的_listColums链表，元素是 Colum指针，一旦创建后不会销毁
 *				大量共享static成员变量和函数，以最大限度缩减类的大小
 *			系统堆空间：
 *				请根据Colum多少自行调整，因为所有的Colum对象实例都放在vector容器，
 *				一个全局Colum对象消耗SRAM 36byte，消耗堆空间，在vector里只创建Colum不销毁，建议系统堆0x400以上
 *				例如：
 *
 *					全局Colum：
 *						消耗RAM .bss段36Byte       Colum test1("开始任务", &eepromSettings.BinCodeOfEnCollect, 1, 1, 0, 1, 1, " ", coulmBinCodeAdj, LOC_ENTER);
 *						消耗RAM .bss段36Byte       Colum test2("版本信息", columsHome_ShowVerInfo);
 *
 *					全局vector<Colum>:
 *						消耗RAM .bss段12Byte
 *							std::vector<Colum> columsTest3 = {
 *								Colum(...),	//随便什么方法构造匿名Colum对象
 *								Colum(...),
 *								...			//任意个Colum元素
 *							};
 *						实际需要根据columsTest3容器内有多少个匿名Colum合理分配堆空间
 *
 *			菜单行为：跟随Buttons库buttons的状态
 *			依赖的类：AutoValue、Colum
 *			依赖的库：Buttons
 *
 *      2022/05/27:
 *      	适配每页大于两个栏的情况
 *      	记忆最后一次切换页时选中栏的位置
 *          适配 显示、循环特性 的特殊情况：每页至多显示n个栏时，当前页显示的栏数为m，m<n，且n-m不一定等于1
 *      2022/05/28:
 *      	支持union settingsBitsType内嵌位域结构体的指定位值修改，0和1 显示为ON/OFF
 *      	Colum类新增支持位域修改的 重载构造函数可以像如下方式在std::vector<Colum> XXX内构造:
 *      		Colum("自动休眠", &systemSto.data.settingsBits[0], B10000000)
 *      	该方式节省了new AutoValue的开销和eeprom，代价是Colum新增2个1byte成员：ptrBits和mask
 *      2022/?/?
 *      	添加_VIFEXTech的PageManger库，以状态机进行异步页面调度，
 *      	消除所有子页面的for(;;)，从此作为MillisTaskManager的一个任务时，
 *      	不会因为for(;;)而阻塞其他任务的调度
 */

#ifndef INC_PAGE_HPP_
#define INC_PAGE_HPP_
#include "Colum.hpp"
#include <list>
#include <vector>
#include "oled_init.h"
#include "Settings.h"
#include <Buttons.hpp>
#include "Arduino.h" //提供bit操作宏
#include "stdio.h"	//提供sprintf
//#include "printf.h"

#ifndef DBG_PAGE
#define DBG_PAGE 0	//< Change 0 to 1 to open debug macro and check program debug information
#if DBG_PAGE
#define DBG_PAGE_PRINT usb_printf
#else
#define DBG_PAGE_PRINT(...)
#endif
#endif


#define mymax(a,b) ((a) > (b) ? (a) : (b))
#define mymin(a,b) ((a) < (b) ? (a) : (b))

#define MENU_DELAY 0	//ms 菜单刷新时间,RTOS下才需要例如50ms


#ifdef __cplusplus
#include <algorithm>
#include <cstring>
void cartoonFreshColums(bool Dir, uint8_t Steps = 4);
void drawLogoAndVersion(char firmwareMark);
void drawNumber(uint8_t x, uint8_t y, uint16_t number,uint8_t places);

class Page {
public:
	Page() {};
	Page(std::vector<Colum> *columVec);
	virtual ~Page() {}
	static void flashPage();
	static void upDatePage();	//非阻塞，待实现
	static void columValAdjust(const Colum *ptrColum);
	static void drawColum(const Colum *ptrColum, int8_t y, uint8_t selected, bool valAdjusting = false); //绘制一个colum
																				//selected 可能为 0，1，0代表不选择，1代表选中
																				//valAdjusting由columValAdjust传入true，用于显示一些字符盖过值的情况
	static void restorePageIndex(bool restore);
	static bool stateTimeOut();
	static bool iterateWaitTimeLongPressed();
	static const Colum* getColumsSelected() { return *ptrPage->_itrColums; }
	static int num_of_itrColum() { return *ptrPage->_itrColums - *ptrPage->_listColums.begin(); }  //求当前_itrColums是当前_listColums的第几个元素，0表示第一个, 测试OK
	static void drawIcon() {;} 					//绘制icons，未使用
	static void cartoonFreshColums(bool Dir, uint8_t Steps = 4);
	static void cartoonFreshFonts(bool Dir = 0, uint8_t Steps = 4);
	static AutoValue indexColums;				//用于索引当前选中的colum在oled上的y坐标位置
	static uint8_t indexColumsUpperFristIn;		//存储首次flash homePage时AutoValue indexColums的Upper，用于在restorePageIndex()中恢复upper
	static const uint8_t cntIndexColumsOffsset;	//用于flashPage的switch()内迭代IndexCoums时，菜单循环情况的IndexColums的迭代次数，该值为Oled每页显示Colum的个数-1
	static Page *ptrPage;
	static Page *ptrPagePrev;
	static Page *homePage;					//最上级菜单（只有一个）
	static std::list<Page *> ptrPageList;	//用于级联各个菜单级的Page对象
	static bool timeOut;					//Page对象共用返回上级Page对象的停顿感延时的标记
	static uint8_t valIndex;				//用于临时储存当前Page的indexColums的val值

	std::list<const Colum*> _listColums;	//唯一的非static public成员变量，用于在构造Page时，生成构造函数传入的std::vector<Colum> *columVec参数的链表
	void drawColums(bool forceDislayColumVal = false);						//唯一的非静态成员函数，批量绘制能当前Page对象能显示在oled屏幕可见范围内的colums
private:
	uint8_t _indexColumsVal; 				//用于Page私有保存static AutoValue indexColums的val值，作用:到上级或下级菜单执行Page::resetPageIndex()时，记忆最后一次选中的colum位置，而更改AutoValue indexColums的val
	std::list<const Colum*>::iterator _itrColums;
	Page *_nextPage;
	Page *_prevPage;
	uint8_t _numColums;	//记录Colums个数，用于动态修改indexColums的upper以防止越界绘制colums
						//比如page的_listColums只有2个成员，而实际oled一页可以绘制3个，那么在flash::Page的switch里迭代index变量
						//(的最大值受到indexColums的upper限制)，会绘制第三个不存在的colum导致越界访问
};

extern Page pageHomePage;
void   enterSettingsMenu();
#endif
#endif	/* INC_PAGE_HPP_ */
