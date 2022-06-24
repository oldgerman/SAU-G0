
/*
 * CustomPage.hpp
 *
 *  Created on: May 25, 2022
 *      Author: OldGerman (过气德国佬)
 */
#ifndef __CUSTOM_PAGE
#define __CUSTOM_PAGE
#include "Page.hpp"
#include "RTClib.h"
#include "BMP.h"
#include "icons.h"
#include "CommonMacro.h"
extern const char *strChooseOK;
extern const char *strChooseCXL;
extern const char *strfeaturesNotRealized;

void colum_FuncNull();

//str只能传中文字符, str2通过str2Chinese指定是否为中文字符
bool colums_StrSelect(bool featuresRealized, SelectState sel, const char *str = nullptr,
		uint8_t OK_CXL_OffSetNumColum = 0, const char *str2 = nullptr, bool str2Chinese = true);
void colum_FeaturesUnrealized();
void columsScreenSettings_Brightness();
void columsScreenOffAndWKUP_Sensitivity();
void columsAccessibility_ResetSettings();
void columsAccessibility_I2CScaner();
void columsHome_Reset();
void coulmBinCodeAdj();
void columsAccessibility_Battery();
void columsDateTime_ChangeDateTime();
void columsDataCollected_Schedule();
void drawLogoAndVersion();
void columsHome_ShowVerInfo();
void columsDataCollect_ScheduleSetting_NumDataOneDay();
void columsDataCollect_ScheduleSetting_NumDataWillCollect();
uint32_t getEEPROM_FreeSize();
DateTime getEEPROMData_DateTime(uintDateTime* ptr);
void columsDataCollect_ScheduleSetting_StartDateTime();
void columsDataCollect_ScheduleSetting_EndDateTime();
void columsDataCollect_ScheduleSetting_NextDateTime();
void columsDrawDateTime(DateTime *dt, const char* str = nullptr);//str只能传中文字符
void columsDataCollect_Switch();
DateTime getScheduleSetting_NextDateTime();
void columsDataCollect_ScheduleDelete();
void columsDataCollect_CollectedDelete();
void columsDataCollect_Export();
void columsAccessibility_RunTime();
void synchronisedUintDateTime(uintDateTime * udt);
void synchronisedTimeStartCollect();
void synchronisedTimeSys();

typedef struct num_X4 {
	uint8_t num3;
	uint8_t num2;
	uint8_t num1;
	uint8_t num0;
}numX4Type;
uint32_t numNthPower(uint8_t base, uint8_t exponent);
numX4Type numSplit(uint16_t num);
#endif /* __CUSTOM_PAGE */
