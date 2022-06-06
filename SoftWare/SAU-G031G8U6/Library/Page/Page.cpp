/*
 * Page.cpp
 *
 *  Created on: 2021年4月20日
 *      Author: OldGerman
 */

#include "Page.hpp"
//#pragma GCC push_options
//#pragma GCC optimize ("O0")	//Os优化无Bug

#define resetWatchdog(...)

void   enterSettingsMenu()
{
	Page::flashPage();
}

Page::Page(std::vector<Colum> *columVec) { //默认指向的上一个Page是自己
	_numColums = columVec->size();				//获取colums个数
	std::vector<Colum>::const_iterator  columIter;
	for (columIter = columVec->begin(); columIter != columVec->end(); columIter++)
		_listColums.push_back(&(*columIter));	//std::list元素为Colum对象指针	//使用了STL内存分配器，需要系统堆空间
	_itrColums = _listColums.begin(); 			//将选中的栏还原为第一个
}

void Page::flashPage() {
	bool InHomePage = 1;						//标记Page对象第一次执行flashPage函数，只给最上级菜单pageHome区分其他下级菜单用
	ptrPageList.push_back(ptrPage);					//进入菜单首先把homePage对象添加到链表末尾
	indexColumsUpperFristIn = indexColums.upper;

	//用于首次进入时，需要松开按键才能
	ButtonState buttonsFristIn = buttons;
	for(;;) {
		buttons = getButtonState();
		if(buttons != buttonsFristIn)
			break;
		ptrPage->drawColums();
		u8g2.sendBuffer();
	}

	//主逻辑
	for (;;) {
		valIndex = *(indexColums.val);

		buttons = getButtonState();

		//按键长按的延迟步幅
		bool markLoogPressed = true;
		if(buttons == BUTTON_A_LONG || buttons == BUTTON_B_LONG) {
			if(iterateWaitTimeLongPressed())
				markLoogPressed = false;
			else
				markLoogPressed = true;
		}

		switch (buttons) {
		case BUTTON_A_LONG:
			if(markLoogPressed) break;
		case BUTTON_A_SHORT:														//若向后迭代Colum
			if (*(ptrPage->_itrColums) == ptrPage->_listColums.back()) {				//且此时_itrColums迭代器指向_listColums的最后元素
				ptrPage->_itrColums = ptrPage->_listColums.begin();						//将_itrColums迭代器指向_listColums的第一个元素
				for(uint8_t cntICO = 0; cntICO < cntIndexColumsOffsset; cntICO++) {
					indexColums--;														//AutoValue类型的indexColums对象也要从upper回退到lower
				}
			}else{																		//否则正常单向迭代
				ptrPage->_itrColums++;
				indexColums++;
			}
			break;
		case BUTTON_B_LONG:
			if(markLoogPressed) break;
		case BUTTON_B_SHORT:															//若向前迭代Colum
			if (ptrPage->_itrColums == ptrPage->_listColums.begin()) {					//且此时_itrColums迭代器指向_listColums的第一个元素
#if 1	//暂时不能等价交换上下，不知道为啥
				ptrPage->_itrColums = ptrPage->_listColums.end();						//将_itrColums迭代器指向_listColums最后的元素的下一个地址
				ptrPage->_itrColums--;													//然后回退一个，指向最后一个元素
#else
				*(ptrPage->_itrColums) = ptrPage->_listColums.back();
#endif
				for(uint8_t cntICO = 0; cntICO < cntIndexColumsOffsset; cntICO++) {
					indexColums++;														//AutoValue类型的indexColums对象也要从lower增加到upper
				}
			}else{
				ptrPage->_itrColums--;													//否则正常单向迭代
				indexColums--;
			}
			break;
		case BUTTON_OK_LONG:															//长按OK退出本级菜单的flash::Page函数
			break;
		case BUTTON_OK_SHORT:															//短按OK进入下级菜单，根据下级菜单的对象，配置Page类的static成员
			/*支持三级及以上菜单*/
			if ((*ptrPage->_itrColums)->nextPage != nullptr)	//若解引用ptrPage得到的Colum的成员，指向Page类的nextPage指针 指向的地址有效，就是说指向了一个新的Page对象
			{
				if(InHomePage) {				//如果是pageHome（广义上的最上级菜单）
					InHomePage = 0;			//将InHomePage标记更改为0
				}else{								//否则是pageXXX（广义上的下级菜单）
					ptrPageList.push_back(ptrPage);	//不是pageHome，那么就是下级page，将它的地址放到ptrPageList末尾
				}

				ptrPage->_indexColumsVal = *indexColums.val;
				ptrPage = (*ptrPage->_itrColums)->nextPage; //当前页面指针指向当前页面指针指向的Colum的下级菜单，可能为nullptr
				restorePageIndex(true);						//重置indexColums和_iterColums到下级Page对象的情况
			}
			else												//否则是非标识下级Page的Colum类型
			{
				const Colum *ptrColum = ptrPage->getColumsSelected();
				//若Colum没有AutoValue对象，是纯函数指针的Colum，例如版本信息Colum
				if (ptrColum->funPtr != nullptr &&
					ptrColum->ptrAutoValue == nullptr &&
					ptrColum->ptrBits == nullptr)
					ptrColum->funPtr();	//跳去执行函数指针的函数
				else
					//否则，Colum有AutoValue或settingsBitsType对象，执行colums的改值函数
					columValAdjust(*ptrPage->_itrColums);
			}
			break;
		default:
			break;
		}

		const uint8_t lastExitCnt = 1;
		static uint8_t lastExit = lastExitCnt;


		if (stateTimeOut() && (buttons == BUTTON_OK_LONG)) {	//若长按OK键，回到向上级菜单
#if DoubleMenu	/*仅支持二级菜单*/
			ptrPage = homePage;
#else		/*支持三级及以上菜单*/
			ptrPage->_indexColumsVal = *indexColums.val;	//记忆本page的_indexColumsVal
			ptrPage = ptrPageList.back();			//此时ptrPage才是前一个Page*

			if(ptrPage != ptrPageList.front()) 		//if内的语句检查是否为homePage情况，防止删除
				ptrPageList.pop_back();				//删除尾端Page
#endif
			restorePageIndex(true);
		}

		//非改值情况快速查看本Page所有Colum的值（只在str遮盖住val和unit情况下有用）
		if(buttons == BUTTON_BOTH || buttons == BUTTON_BOTH_LONG ) {
			ptrPage->drawColums(true);
		}
		else {
			ptrPage->drawColums(false);
		}

		u8g2.sendBuffer();
		HAL_Delay(MENU_DELAY);

		//到最父级的homePage页才退出本函数块
		if(ptrPage == ptrPageList.front() &&
				stateTimeOut() &&
				(buttons == BUTTON_OK_LONG))
		{
			--lastExit;
			if(!lastExit )
			{
				lastExit = lastExitCnt;
//				在此处(退出homePage时)才保存值
				saveSettings();
//				restoreSettings();
				break;
			}
		}
		resetWatchdog();
	}
}



void changeSettingsBitByMask(settingsBitsType * ptrBits, uint8_t mask){
	uint8_t b = ptrBits->ctrl;	//先备份一个
	(b & mask)?			//若b对应mask为1的位为1
		(b = b & ~mask)		//不改变b对应mask为0位的值，将mask为1的值改为0
	:					//b对应mask为1的位为0
		(b = b | mask);		//不改变b对应mask为0位的值，将mask为0的值改为1
//参考			    b = (data != 0) ? (b | mask) : (b & ~mask);
    ptrBits->ctrl = b;	//保存b到结构体
}



void Page::columValAdjust(const Colum *ptrColum) {
	AutoValue *ptrAutoValue = ptrColum->ptrAutoValue;
	uint32_t lastChange = HAL_GetTick();
	//入口处执行函数指针的不用执行else部分
	if (ptrColum->str == nullptr)
		return;

	if (ptrColum->funLoc == LOC_ENTER) {
		ptrColum->funPtr();
	} else {

		for (;;) {
			//u8g2.clearBuffer();	//保留上级for循环刷新的buffer，本节只刷新值
			buttons = getButtonState();

			//按键长按的延迟步幅
			bool markLoogPressed = true;
			if(buttons == BUTTON_A_LONG || buttons == BUTTON_B_LONG) {
				if(iterateWaitTimeLongPressed())
					markLoogPressed = false;
				else
					markLoogPressed = true;
			}


			if (buttons)
				lastChange = HAL_GetTick();
			AutoValue::buttonState = buttons;

			switch (buttons) {
			case BUTTON_A_LONG:
				if(markLoogPressed) break;
			case BUTTON_A_SHORT:
				if(ptrColum->ptrBits != nullptr){
					changeSettingsBitByMask(ptrColum->ptrBits, ptrColum->mask);
				}
				else {
					(*ptrAutoValue)--; //AutoValue::operator--()在内部判断buttons长按短按
				}
				if (ptrColum->funLoc == LOC_CHANGE)
					ptrColum->funPtr();
				break;
			case BUTTON_B_LONG:
				if(markLoogPressed) break;
			case BUTTON_B_SHORT:
				if(ptrColum->ptrBits != nullptr){
					changeSettingsBitByMask(ptrColum->ptrBits, ptrColum->mask);
				}
				else {
					(*ptrAutoValue)++;
				}
				if (ptrColum->funLoc == LOC_CHANGE)
					ptrColum->funPtr();
				break;
			default:
				break;
			}

			uint16_t y = *(indexColums.val);
			ptrPage->drawColum(ptrColum, y, 1, true);
			y += 1;
			u8g2.setDrawColor(0);
			u8g2.drawStr(OLED_WIDTH - 46, y + 11, "*");	//绘制更改标记
			u8g2.sendBuffer();

			if ((HAL_GetTick() - lastChange > 2000)
					|| (buttons == BUTTON_OK_SHORT)) {
				if (ptrColum->funLoc == LOC_EXTI)
					ptrColum->funPtr();
				return;
			}

			HAL_Delay(MENU_DELAY);
		}
	}
}


void Page::drawColums(bool forceDislayColumVal) {
	//绘制被选中的colum
	drawColum(*_itrColums, *(indexColums.val), 1, forceDislayColumVal);
	//绘制被选中colum上面的colum
	for (uint8_t i = indexColums.lower; i < *(indexColums.val);) {
		i += indexColums.shortSteps;
		drawColum(
				*_itrColums
						- (*(indexColums.val) - indexColums.lower)
								/ indexColums.shortSteps
						+ (i / indexColums.shortSteps) - 1,
				i - indexColums.shortSteps, 0, forceDislayColumVal);
	}
	//绘制被选中colum下面的colum
	for (uint8_t i = 0; i < indexColums.upper - *(indexColums.val);) {
		i += indexColums.shortSteps;
		drawColum(*_itrColums + (i / indexColums.shortSteps),
				*(indexColums.val) + i, 0, forceDislayColumVal);
	}

	//针对栏数小于oled一页显示的最大数，在不显示栏的地方绘制黑色矩形
	if(indexColumsUpperFristIn > indexColums.upper){
		u8g2.setDrawColor(0);	//设置黑色
		for(uint8_t i = indexColums.upper; i < indexColumsUpperFristIn;) {
		    i += 16;
		    u8g2.drawBox(0, i , 64, 16);
		}
		u8g2.setDrawColor(1);	//还原白色
	}
}

void Page::drawColum(const Colum *ptrColum, int8_t y, uint8_t selected, bool valAdjusting) {
	//绘制反显矩形
	if(selected != 2) {
		u8g2.setDrawColor(selected);
		u8g2.drawBox(0, y, 64, 16);
		u8g2.setDrawColor(!selected);
	}
	else
	{
		//不要绘制黑色矩形，动画过渡会闪屏
		//u8g2.setDrawColor(!selected);
		//u8g2.drawBox(0, y, 64, 16);

		u8g2.setDrawColor(selected); //color 2模式
	}

	//绘制栏名称字符,宋体UTF-8
#define DRAW_COLUM_Y_OFFSET 13U
	y += DRAW_COLUM_Y_OFFSET;	//偏移字符串y坐标

	if (ptrColum->str != nullptr) {
		u8g2.setFont(u8g2_simsun_9_fontUniSensorChinese);	//12x12 pixels
		u8g2.setFontRefHeightText();
		u8g2.drawUTF8(1, y, ptrColum->str);	//打印中文字符，编译器需要支持UTF-8编码，显示的字符串需要存为UTF-8编码
	}

	//若Colum对象存在待修改的值
	if (ptrColum->ptrAutoValue != nullptr ||
			ptrColum->ptrBits != nullptr) {

		//Colum的str对象长度是否与val和unit部分有显示重叠,中文字符宽度为13像素，有重叠则仅在改值情况下绘制遮罩矩形，否则不绘制val和unit
		if((!valAdjusting && ((strlen(ptrColum->str) / 3)*13 < (OLED_WIDTH - 49))) || valAdjusting){
			//绘制遮罩矩形
			u8g2.setDrawColor(selected);
			u8g2.drawBox(OLED_WIDTH - 49, y - DRAW_COLUM_Y_OFFSET, 64, 16);
			u8g2.setDrawColor(!selected);

			//绘制栏详情字符
			if ((!(*ptrColum->ptrAutoValue).valueIsBool()  && ptrColum->ptrBits == nullptr)
#if AUTOVALUE_MAP_TO_STRING
					|| (ptrColum->ptrColumVal2Str != nullptr) 	//针对只有0和1两个值map string的情况
#endif
					)
			{
#if AUTOVALUE_MAP_TO_STRING
				//如果数字值存在到中文字符映射
				if(ptrColum->ptrColumVal2Str != nullptr)
				{
					std::map<uint16_t, const char*>::iterator itr =
							ptrColum->ptrColumVal2Str->find(*(ptrColum->ptrAutoValue)->val);
					u8g2.drawUTF8( OLED_WIDTH -  strlen(itr->second) / 3 /*"中" = 3 "中文" = 6 "中文字" = 9;=*/
							* 12/*12=字体宽度*/ -3 /*边缘偏移*/, y, itr->second);
				}
				//否则绘制数字值
				else
#endif
				{
					// 修改字体为非中文字体
					u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
					u8g2.setFontRefHeightText();

					bool is2Byte = false;
					if(ptrColum->ptrAutoValue->places > 2) {	//根据ptrAutoValue的Places，即位数，判断是多少byte的类型，对1byte类型若强制转2byte，传入drawNumber内部会程序会崩溃
						is2Byte = true;
					}
#if 0
					drawNumber((OLED_WIDTH - 15) - (ptrColum->ptrAutoValue->places) * 6, y,
							is2Byte?(*(ptrColum->ptrAutoValue)->val):(*(ptrColum->ptrAutoValue)->val),
						(ptrColum->ptrAutoValue)->places);

					if (ptrColum->unit != nullptr) {
						u8g2.drawStr((OLED_WIDTH - 10), y, ptrColum->unit);	//	绘制单位
					}
#else
					uint8_t numXOffset = 15;
					if (ptrColum->unit != nullptr) {
						u8g2.drawStr((OLED_WIDTH - 10), y, ptrColum->unit);	//	绘制单位
					}
					else
						numXOffset = 10;	//若没有单位，最多可绘制四位数number
					drawNumber((OLED_WIDTH - numXOffset) - (ptrColum->ptrAutoValue->places) * 6, y,
							is2Byte?(*(ptrColum->ptrAutoValue)->val):(*(ptrColum->ptrAutoValue)->val),
						(ptrColum->ptrAutoValue)->places);
#endif
				}
			}


			else	//为“虚拟bool类型”，特殊处理
			{
				u8g2.setFont(u8g2_font_unifont_tr);	//10x7 pixels
				u8g2.setFontRefHeightText();
				bool val;
				if(ptrColum->ptrBits != nullptr){
					val = ptrColum->ptrBits->ctrl & ptrColum->mask;	//第一种为1bit(位域)形式
				}
				else{
					val = *(ptrColum->ptrAutoValue)->val;			//第二种为AutoValue形式
				}
				val ?
						u8g2.drawStr(OLED_WIDTH - 17, y, "ON") :
						u8g2.drawStr(OLED_WIDTH - 25, y, "OFF");
			}
		}
	}
}


void drawNumber(uint8_t x, uint8_t y, uint16_t number,
		uint8_t places) {
	char buffer[7] = { 0 };
	//sprintf(buffer, "%06d" , number);
	sprintf(buffer, "%6d", number);
	uint8_t cntFirstNum = 0;
	uint8_t i = 0;
	while (i < 7) {
		if (buffer[i] != ' ') {
			cntFirstNum = i;
			break;
		}
		++i;
	}
	uint8_t cntNum = 6 - cntFirstNum;
	u8g2.drawStr(x, y, buffer + cntFirstNum - (places - cntNum));
	//u8g2.drawUTF8(x, y, buffer + cntFirstNum - (places - cntNum));
}
bool Page::stateTimeOut() {
	uint32_t previousState = HAL_GetTick();
	static uint32_t previousStateChange = HAL_GetTick();
	if ((previousState - previousStateChange) > 500) {	///	这个500决定向父级菜单递归的阻塞感
		previousStateChange = previousState;
		return true;
	}
	return false;
}

bool Page::iterateWaitTimeLongPressed() {
	uint32_t previousState = HAL_GetTick();
	static uint32_t previousStateChange = HAL_GetTick();
	if ((previousState - previousStateChange) > 250) {	//250决定按键长按的延迟步幅
		previousStateChange = previousState;
		return true;
	}
	return false;
}
// 恢复索引变量值
void Page::restorePageIndex(bool restore) {
	if (restore) {	//如果恢复栏index变量
		//定向选中栏到最后一次退出上级菜单的情况（根据私有成员_indexColumsVal记录的上一级菜单下，选中栏的最后位置（由static AutoValue indexColums的val值决定的））,
		*(indexColums.val) = ptrPage->_indexColumsVal;

		/* 限制选中栏不超过page的最后一个colum显示的位置
		 * 		栏高固定为16pixels
		 * 		例如
		 * 			若为oled 128x32，那么一页最多显示2个栏，此情况在当本页栏为1个时会出现
		 * 			若为oled  64x48，那么一页最多显示3个栏，此情况在当本页栏为2个时会出现
		 * 			若为oled 128x64，那么一页最多显示4个栏，此情况在当本页栏为3个时会出现
		 */
		if(ptrPage->_numColums <= cntIndexColumsOffsset)
			indexColums.upper = (ptrPage->_numColums - 1) * 16;
		else
			indexColums.upper = indexColumsUpperFristIn;

	} else {		//否则强制定向index到第一个栏
		ptrPage->_itrColums = ptrPage->_listColums.begin();
		(*(indexColums.val) = 0);
	}
}
//#pragma GCC pop_options
// 临时健忘笔记
/**
 * C++对象数组构造函数
 * 要使用需要多个参数的构造函数，则初始化项必须釆用函数调用的形式。
 * 例如，来看下面的定义语句，它为 3 个 Circle 对象的每一个调用 3 个参数的构造函数：
 * Circle circle[3] = {Circle(4.0, 2, 1),Circle(2.0, 1, 3),Circle (2.5, 5, -1) };
 * 没有必要为数组中的每个对象调用相同的构造函数。例如，以下语句也是合法的：
 * Circle circle [3] = { 4.0,Circle (2.0, 1, 3),2.5 };
 */
/**
 * array对象和数组存储在相同的内存区域（栈）中，vector对象存储在自由存储区（堆）
 */

/*
	利用 strlen 和 sizeof 求取字符串长度注意事项
	strlen 是函数，sizeof 是运算操作符，二者得到的结果类型为 size_t，即 unsigned int 类型。
	大部分编译程序在编译的时候就把 sizeof 计算过了，而 strlen 的结果要在运行的时候才能计算出来。
	对于以下语句：
	char *str1 = "asdfgh";
	char str2[] = "asdfgh";
	char str3[8] = {'a', 's', 'd'};
	char str4[] = "as\0df";
	执行结果是：
	sizeof(str1) = 4;  strlen(str1) = 6;
	sizeof(str2) = 7;  strlen(str2) = 6;
	sizeof(str3) = 8;  strlen(str3) = 3;
	sizeof(str4) = 6;  strlen(str4) = 2;
	注意中文字体一个是4
*/
