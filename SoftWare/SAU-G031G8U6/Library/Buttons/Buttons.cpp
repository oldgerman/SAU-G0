/*
 * Buttons.c
 *
 *  Created on: 29 May 2020
 *      Author: Ralim
 *      Modify: OldGerman
 */
#include <Buttons.hpp>
#include "BSP.h"


#define SW_DELAY 0								//ms 按键中delay的时间

#if 0
#define DBG_BUTTONS usb_printf
#else
#define DBG_BUTTONS(...)
#endif

uint32_t lastButtonTime = 0;
ButtonExchange buttonExchange = BUTTON_EX_NONE;	//用于标记button状态被更改
bool buttonIsBeep = true;						//用于标记button状态被更改

ButtonState buttons = BUTTON_NONE;				//全局按键状态


/**
 * 死循环等待按键选择
 */
bool waitingSelect(SelectState sel)
{
	int8_t ok = -1;
	ButtonState buttonsOld = buttons;
	while(getButtonState() == buttonsOld)
		;//		resetWatchdog();
	for(;;)
	{
//		resetWatchdog();
		ButtonState buttons = getButtonState();
		switch(buttons)
		{
			case BUTTON_OK_SHORT:
				if(sel == SEL_1 || sel == SEL_3) ok = 1;
				break;
			case BUTTON_A_SHORT:
				if(sel == SEL_2 || sel == SEL_3) ok = 1;
				break;
			case BUTTON_B_SHORT:
				if(sel == SEL_2) ok = 0;
				if(sel == SEL_3) ok = 1;
				break;
			default:
				break;
		}
		if(ok != -1)
			break;
		HAL_Delay(SW_DELAY);
		static int8_t cntBuzzerTime = 0;
		cntBuzzerTime++;
		if(cntBuzzerTime == 3)


		cntBuzzerTime = cntBuzzerTime % 4;
	}
	while(getButtonState() == buttonsOld)
		;//		resetWatchdog();

	return ok;
}


ButtonState getButtonState()
{
	static ButtonState buttonValPrev = BUTTON_NONE;
	ButtonState buttonVal;
	buttonVal = readButtonState();

	if((buttonVal != BUTTON_NONE))
	{
		if(buttonIsBeep == true)
		{
			//DBG_BUTTONS("buttonIsBeep = ture!\r\n");
			//本次状态是长按，并且上次状态是短按
			if(buttonVal % 5 == 0 && buttonValPrev % 5 != 0)
				buttonExchange = BUTTON_EX_SHORT_TO_LONG;
			else
				buttonExchange = BUTTON_EX_DONE;

			DBG_BUTTONS("Button = %d, buttonExchange = %d\r\n", buttonVal, buttonExchange);
		}
	}

	if(buttonVal != buttonValPrev)
		usb_printf("ButtonStatte = %d\r\n", buttonVal);

	buttonValPrev = buttonVal;
	return buttonVal;
};

ButtonState readButtonState() {
  /*
   * Read in the buttons and then determine if a state change needs to occur
   */

  /*
   * If the previous state was  00 Then we want to latch the new state if
   * different & update time
   * If the previous state was !00 Then we want to search if we trigger long
   * press (buttons still down), or if release we trigger press
   * (downtime>filter)
   */
  static uint8_t  previousABState       = 0;
  static uint32_t previousABStateChange = 0;
  const uint16_t  timeout             = 400;	//若按下超过400秒即为长按
  uint8_t         currentABState;


  static uint8_t  previousOKState       = 0;
  static uint32_t previousOKStateChange = 0;
  uint8_t         currentOKState;

  //currentABState = (getButtonA()) << 0;
  //currentABState |= (getButtonB()) << 1;


  currentABState = (getButtonB()) << 0;
  currentABState |= (getButtonA()) << 1;
  currentOKState = getButtonOK();

  if (currentABState || currentOKState)
    lastButtonTime = HAL_GetTick();


/*
 * BUTTON OK键状态判断
 * 可以判断2种状态：短按，长按
 * 与AB键6种状态不冲突
 **/
  ButtonState retOKVal = BUTTON_NONE;
  if (currentOKState == previousOKState)
  {
	if((currentOKState == 1) && ((HAL_GetTick() - previousOKStateChange) > timeout))
	{
		retOKVal =  BUTTON_OK_LONG; // Both being held case
		return retOKVal;
	}
	//return BUTTON_NONE;//此处不返回，需要跳转到下面的AB返回BUTTON_NONE，因为A、B、OK键共用BUTTON_NONE状态
  }
  else	// currentOKState != previousOKState
  {
    if ((HAL_GetTick() - previousOKStateChange) < timeout)
          retOKVal = BUTTON_OK_SHORT;

    previousOKState       = currentOKState;
    previousOKStateChange = HAL_GetTick();
    return retOKVal;
  }


/*BUTTON AB键6钟状态判断*/
  if (currentABState == previousABState)
  {
    if (currentABState == 0)
      return BUTTON_NONE;
    if ((HAL_GetTick() - previousABStateChange) > timeout) {
      // User has been holding the button down
      // We want to send a button is held message
      if (currentABState == 0x01)// && buttons != BUTTON_IDLE)
        return BUTTON_B_LONG;
      else if (currentABState == 0x02)// && buttons != BUTTON_IDLE)
        return BUTTON_A_LONG;
      else
        return BUTTON_BOTH_LONG; // Both being held case
    } else
      return BUTTON_NONE;
  }
  else
  {
    // A change in button state has occurred
    ButtonState retVal = BUTTON_NONE;
    if (currentABState) {
      // User has pressed a button down (nothing done on down)
      if (currentABState != previousABState) {
        // There has been a change in the button states
        // If there is a rising edge on one of the buttons from double press we
        // want to mask that out As users are having issues with not release
        // both at once
        if (previousABState == 0x03)
          currentABState = 0x03;
      }
    } else {
      // User has released buttons
      // If they previously had the buttons down we want to check if they were <
      // long hold and trigger a press
      if ((HAL_GetTick() - previousABStateChange) < timeout) {
        // The user didn't hold the button for long
        // So we send button press

        if (previousABState == 0x01)
          retVal = BUTTON_B_SHORT;
        else if (previousABState == 0x02)
          retVal = BUTTON_A_SHORT;
        else
          retVal = BUTTON_BOTH; // Both being held case
      }
    }
    previousABState       = currentABState;
    previousABStateChange = HAL_GetTick();
    return retVal;
  }

  return BUTTON_NONE;
}


void waitForButtonPress() {
  // we are just lazy and sleep until user confirms button press
  // This also eats the button press event!
  ButtonState buttons = getButtonState();
  while (buttons) {
    buttons = getButtonState();
    HAL_Delay(SW_DELAY);
  }
  while (!buttons) {
    buttons = getButtonState();
    HAL_Delay(SW_DELAY);
  }
}

void waitForButtonPressOrTimeout(uint32_t timeout) {
  timeout += HAL_GetTick();
  // calculate the exit point

  ButtonState buttons = getButtonState();
  while (buttons) {
    buttons = getButtonState();
    HAL_Delay(SW_DELAY);
    if (HAL_GetTick() > timeout)
      return;
  }
  while (!buttons) {
    buttons = getButtonState();
    HAL_Delay(SW_DELAY);
    if (HAL_GetTick() > timeout)
      return;
  }
}

uint8_t getButtonA() { return HAL_GPIO_ReadPin(KEY_A_GPIO_Port, KEY_A_Pin) == GPIO_PIN_RESET ? 1 : 0; }
uint8_t getButtonB() { return HAL_GPIO_ReadPin(KEY_B_GPIO_Port, KEY_B_Pin) == GPIO_PIN_RESET ? 1 : 0; }
uint8_t getButtonOK() { return HAL_GPIO_ReadPin(KEY_OK_GPIO_Port, KEY_OK_Pin) == GPIO_PIN_RESET ? 1 : 0; }

//若的某一按键比如 KEY_B （对应原理图S3）是x外部下拉的，返回值取反即可，例如
//uint8_t getButtonB() { return !(HAL_GPIO_ReadPin(KEY_B_GPIO_Port, KEY_B_Pin) == GPIO_PIN_RESET ? 1 : 0); }


