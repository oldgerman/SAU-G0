/*
 * Buttons.h
 *
 *  Created on: 29 May 2020
 *      Author: Ralim
 *      Modify: OldGerman
 *
 *      原本是"KEY_A"  "KEY_B" 双键状态检测
 *      添加了"KEY_OK"，进行中键短按长按检测
 *      因此ButtonState枚举增加到7种状态识别
 *      延时函数是osDelay，只适用于RTOS系统，osDelay默认参数为SW_DELAY的值为50ms
 *
 *		在任意循环中添加以下代码，对应状态下添加想执行代码即可
 		buttons = getButtonState();
		switch (buttons) {
		case BUTTON_NONE:
			break;
		case BUTTON_A_SHORT:
			break;
		case BUTTON_A_LONG:
			break;
		case BUTTON_B_SHORT:
			break;
		case BUTTON_B_LONG:
			break;
		case BUTTON_BOTH:
			break;
		case BUTTON_BOTH_LONG:
			break;
		case BUTTON_OK_SHORT:
			break;
		case BUTTON_OK_LONG:
			break;
		default:
			break;
		}

 *      STM32CubeMX需要对对应的按键引脚的 User Label，左键、中键、右键分别为 KEY_A 、KEY_OK、和 KEY_B
 *      默认的getButtonXXX()检测的按键需要配置为上拉：按下低电平为返回1，松开高电平返回0，例如
 *      uint8_t getButtonB() { return HAL_GPIO_ReadPin(KEY_B_GPIO_Port, KEY_B_Pin) == GPIO_PIN_RESET ? 1 : 0; }
 *      若的某一按键比如 KEY_B 配置为下拉，将函数返回值取反即可，例如
 *      uint8_t getButtonB() { return !(HAL_GPIO_ReadPin(KEY_B_GPIO_Port, KEY_B_Pin) == GPIO_PIN_RESET ? 1 : 0); }
 *
 *		添加的buttonExchange和buttonIsBeep标志位
 *		可用于与蜂鸣器联动，例如某一个RTOS任务，插入以下代码
 *		void doXXXTask() {
	 		static bool BeepDouble = false;		//长按情况响和不响的交替变化标志
			static uint8_t cntBeepDouble = 0;	//长按情况响和不响的交替计数变量
			//......

			for(;;)
			{
				//......

				//蜂鸣器跟随按键动作
				if(buttonExchange)
				{
					if(buttonExchange ==  BUTTON_EX_SHORT_TO_LONG) {
						BeepDouble = true;
						cntBeepDouble = 4;
					}

					buttonExchange = BUTTON_EX_NONE;
					buttonIsBeep = false;
					valVolume = map(systemSettings.BuzzerVolume, 0, 100, 0, 128);
					if(valVolume)
					{
						htim3.Instance->CCR2 = valVolume;
						HAL_TIM_PWM_Start(&htim3, BUZZER_CHANNEL);	//buzzer
					}
				}
				else
				{
					if(!BeepDouble)	{//触发了BeepDouble则锁定getButtonState()里更新的buttonExchange状态为第209行的BUTTON_EX_NONE
						//将CCR1置为0或255时，蜂鸣器仍有底噪异响，必须关闭PWM通道才安静
						//htim3.Instance->CCR1 = 0;
						HAL_TIM_PWM_Stop(&htim3, BUZZER_CHANNEL);	//buzzer
						buttonIsBeep = true;	//下次进入时才能置为true，每次响必定经过两倍的周期时间，即200ms（100ms太短不好听）
					}
		#if 1		//对于一直长按的情况，进行比较麻烦的响一下停一下的处理，防止蜂鸣器一直长鸣
					else{
						//本地在BUTTON_EX_SHORT_TO_LONG后要响蜂鸣器两次，才会将BeepDouble置于false，此期间buttonIsBeep一直为false以锁定
						if(!(cntBeepDouble % 2)) {
							HAL_TIM_PWM_Stop(&htim3, BUZZER_CHANNEL);	//buzzer
							buttonIsBeep = true;
						}
						else {
							if(cntBeepDouble == 1)
							{
								BeepDouble = false;
							}
							else {
								HAL_TIM_PWM_Start(&htim3, BUZZER_CHANNEL);	//buzzer
								buttonIsBeep = false;
							}
						}
						cntBeepDouble -= 1;	//响-100ms 4-停 3-响 2-停 1-不响并结束BeepDouble模式
					}
		#endif
				}

				osDelay(TICKS_100MS); // Slow down update rate	//每100ms调用一次
			}
		}
 */
#ifndef INC_BUTTONS_H_
#define INC_BUTTONS_H_

#include "main.h"

#ifdef __cplusplus

// Returns what buttons are pressed (if any)


// Helpers
void waitForButtonPressOrTimeout(uint32_t timeout);
void waitForButtonPress();

extern "C" {
#endif
//B改A，F改B
//7种状态测试OK
typedef enum{
  BUTTON_NONE      = 0,  /* No buttons pressed / < filter time*/
  //3的倍数，非5的倍数
  BUTTON_B_SHORT   = 3,  /* User has pressed the front button*/
  BUTTON_A_SHORT   = 6,  /* User has pressed the back  button*/
  BUTTON_BOTH      = 9, /* User has pressed both buttons*/
  BUTTON_OK_SHORT  = 12, /* OK键短按 */
  //5的倍数，非3的倍数
  BUTTON_B_LONG    = 5,  /* User is  holding the front button*/
  BUTTON_A_LONG    = 10,  /* User is  holding the back button*/
  BUTTON_BOTH_LONG = 20, /* User is holding both buttons*/
  BUTTON_OK_LONG   = 25,/* OK键长按 */
  //取素数
  BUTTON_IDLE	   = 73,/* 用于不同Page间切换时提供断点*/
  BUTTON_FROZEN    = 79 /*用于切换时的冻结时间*/

  /*
   * Note:
   * Pressed means press + release, we trigger on a full \__/ pulse
   * holding means it has gone low, and been low for longer than filter time
   */
}ButtonState;
extern ButtonState buttons;		//全局按键状态

//一颗按键的3种变化状态
typedef enum{
  BUTTON_EX_NONE      		= 0,
  BUTTON_EX_DONE      		= 1,
  BUTTON_EX_SHORT_TO_LONG 	= 2
}ButtonExchange;
extern ButtonExchange buttonExchange;

extern bool buttonChanged;
extern bool buttonIsBeep;
extern uint32_t lastButtonTime;
uint8_t getButtonA();
uint8_t getButtonB();
uint8_t getButtonOK();
extern bool waitingToChooseOneFromTwo();
ButtonState readButtonState();
ButtonState getButtonState();
#ifdef __cplusplus
}
#endif
#endif /* INC_BUTTONS_H_ */
