/*
 * Colum.hpp
 *
 *  Created on: 2021年4月20日
 *      Author: OldGerman (过气德国佬)
 */

#ifndef INC_COLUM_HPP_
#define INC_COLUM_HPP_
#include "stdint.h"
#include "Buttons.hpp"	//提供ButtonState枚举类型
#include "Settings.h"
#ifdef __cplusplus

#define AUTOVALUE_MAP_TO_STRING 0	//如果显示待修改的数组值需要映射到字符串, 1: 映射 0: 不映射，可以节省180byte Flash
#if AUTOVALUE_MAP_TO_STRING
#include <map>
#endif

//改值函数Page::columValAdjust()内，当前Colum对象的函数指针执行位置
enum FunLoc {
	LOC_NONE,   //	执行Page::columValAdjust()函数体内的修改值方法，但不执行当前Colum对象的函数指针
	LOC_ENTER,	//不执行Page::columValAdjust()函数体内的修改值方法，只执行一次当前Colum对象的函数指针
	LOC_CHANGE,	//	执行Page::columValAdjust()函数体内的修改值方法，且一旦该方法改值一次，就执行一次当前Colum对象的函数指针
	LOC_EXTI	//	执行Page::columValAdjust()函数体内的修改值方法，只在退出Page::columValAdjust()函数时，执行一次当前Colum对象的函数指针
};

class Page;	//前置声明

/**
 * 构造时val支持char、int8_t、uint8_t、int16_t、uint16_t等不大于2byte的类型，
 * 使用1byte类型取val时需要强制转换
 */
class AutoValue {
public:
	AutoValue() {
	}
	AutoValue(uint16_t *Val, uint8_t Places, uint16_t Upper, uint16_t Lower,	//注意val是泛型指针
			uint8_t ShortSteps, uint8_t LongSteps = 0, bool FollowButtonState =
					true, bool ValueCycle = false) :
			val(Val), places(Places), upper(Upper), lower(Lower), shortSteps(
					ShortSteps), longSteps(LongSteps), followButtonState(
					FollowButtonState), valueCycle(ValueCycle) {
		if (FollowButtonState)
			valueCycle = true;
	}
	virtual ~AutoValue() {
	}
	uint16_t operator++(int) {
		if ((buttonState == BUTTON_B_SHORT || !followButtonState)
				&& shortSteps != 0)
			stepsPlus(shortSteps);
		//++运算不足一个LongSteps时会自动变为upper
		if (buttonState == BUTTON_B_LONG || shortSteps == 0)
			stepsPlus(longSteps);
		return *(uint16_t*)val;
	}

	uint16_t operator--(int) {
		if ((buttonState == BUTTON_A_SHORT || !followButtonState)
				&& shortSteps != 0)
			stepsLess(shortSteps);
		//--运算不足一个LongSteps时会自动变为lower
		if (buttonState == BUTTON_A_LONG || shortSteps == 0)
			stepsLess(longSteps);
		return *(uint16_t*)val;
	}

	bool valueIsBool() {
		return (upper == 1 && lower == 0) ? 1 : 0;
	}

	void stepsLess(uint8_t steps) {
		if (*(uint16_t*)val - lower > steps)
			*(uint16_t*)val -= steps;
		else {
			*(uint16_t*)val = lower;
			if (valueCycle) {	//循环值
				static uint8_t bound = 0;
				++bound;
				if (bound == 2) {
					*(uint16_t*)val = upper;
					bound = 0;
				}
			}
		}
	}
	void stepsPlus(uint8_t steps) {
		if (upper - *(uint16_t*)val > steps)
			*(uint16_t*)val += steps;
		else {
			*(uint16_t*)val = upper;
			if (valueCycle) {	//循环值
				static uint8_t bound = 0;
				++bound;
				if (bound == 2) {
					*(uint16_t*)val = lower;
					bound = 0;
				}
			}
		}
	}

	uint16_t *val;			//待修改值，为了兼容1byte和2byte数据类型而使用模板太麻烦了，改为泛型指针好了。。。
	uint8_t places;			//值的位数
	uint16_t upper;			//值的上限
	uint16_t lower;			//值的下限
	uint8_t shortSteps;		//短按值的步幅
	uint8_t longSteps;		//长按值的步幅
	bool followButtonState;	//是否跟随buttons状态，默认跟随，例外是Page::indexColums不跟随，会导致非法访问
	static ButtonState buttonState;	//用于从外部获取按钮状态
	uint16_t percentageVal;
	bool valueCycle;		//值是否越界循环
};

class Colum {
public:
	Colum(const char *Str = nullptr) :
			str(Str) {}

	Colum(const char *Str, uint16_t *Val, uint8_t Places, uint16_t Upper,
			uint16_t Lower, uint8_t ShortSteps, uint8_t LongSteps = 0,
			const char *Uint = nullptr, void (*FunPtr)(void) = nullptr,
			FunLoc FunLoc = LOC_NONE
#if AUTOVALUE_MAP_TO_STRING
			,std::map<uint16_t, const char*> *PtrColumVal2Str = nullptr
#endif
			):str(Str), unit(Uint), funPtr(FunPtr), funLoc(FunLoc)
#if AUTOVALUE_MAP_TO_STRING
			,ptrColumVal2Str(PtrColumVal2Str)
#endif
	{ ptrAutoValue = new AutoValue(Val, Places, Upper, Lower, ShortSteps, LongSteps); }

	Colum(const char *Str, AutoValue *PtrAutoValue, const char *Uint = nullptr,
			void (*FunPtr)(void) = nullptr, FunLoc FunLoc = LOC_NONE
#if AUTOVALUE_MAP_TO_STRING
			,std::map<uint16_t, const char*> *PtrColumVal2Str = nullptr
#endif
			) : str(Str), ptrAutoValue(PtrAutoValue), unit(Uint), funPtr(FunPtr), funLoc(
					FunLoc)
#if AUTOVALUE_MAP_TO_STRING
	, ptrColumVal2Str(PtrColumVal2Str)
#endif
	{}

	Colum(const char *Str, void (*FunPtr)(void), FunLoc FunLoc = LOC_NONE) :
			str(Str), funPtr(FunPtr), funLoc(FunLoc) {}

	Colum(const char *Str, settingsBitsType* Bits, uint8_t Mask,
			void (*FunPtr)(void) = nullptr, FunLoc FunLoc = LOC_NONE)  :
		str(Str), ptrBits(Bits), mask(Mask), funPtr(FunPtr), funLoc(FunLoc){}

	//仅这个函数nextPage ≠ nullptr，该Colum对象进入下一个Page对象，三级及以上菜单使用
	Colum(const char *Str, Page *NextPage) :
			str(Str), nextPage(NextPage) {}
	virtual ~Colum() {}

	const char *str; //warning：str will be initialized after：确保成员在初始化列表中的出现顺序与在类中出现的顺序相同：
	AutoValue *ptrAutoValue = nullptr;
	const char *unit = nullptr;
	Page *nextPage = nullptr;
	Page *prevPage = nullptr;
	settingsBitsType * ptrBits = nullptr;
	uint8_t mask;
	void (*funPtr)(void) = nullptr;	//改值页执行函数
	FunLoc funLoc = LOC_NONE;			//注意操控bit的Colum构造函数没有初始化funLoc，导致ColumAdjust函数内运行到	if (ptrColum->funLoc == LOC_ENTER) {ptrColum->funPtr();里面使程序
#if AUTOVALUE_MAP_TO_STRING
	std::map<uint16_t, const char*> *ptrColumVal2Str = nullptr;//若有数值映射需要，则指向传入的map数组解析值到字符串
#endif

};
#endif
#endif /* INC_COLUM_HPP_ */
//引用在定义时必须初始化
//算术运算符中的转换规则：
//double ←── float 高
//↑
//long
//↑
//unsigned
//↑
//int ←── char,short 低
