
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

void colum_FuncNull();
bool colums_StrChooseOneFromTwo(bool featuresRealized = false, const char *str = nullptr, uint8_t yOffSet = 0);
void colum_FeaturesUnrealized();
void columsScreenSettings_Brightness();
void columsAccessibility_ResetSettings();
void columsAccessibility_I2CScaner();
void columsHome_Reset();
void coulmBinCodeAdj();
void columsAccessibility_Battery();
void columsDateTime_ChangeDateTime();
void columsDataCollected_Schedule();
void columsHome_ShowVerInfo();
void columsDataCollect_ScheduleSetting_NumDataOneDay();
void columsDataCollect_ScheduleSetting_NumDataWillCollect();
uint32_t getEEPROMFreeSize();
void columsDataCollect_ScheduleSetting_EDDateTime();
void columsDataCollect_ScheduleSetting_STDateTime();
void columsDrawDateTime(DateTime *dt);
#endif /* __CUSTOM_PAGE */
