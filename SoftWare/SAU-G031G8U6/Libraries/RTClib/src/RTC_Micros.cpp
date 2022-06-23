#include "RTClib.h"



/*
 * Returns the number of microseconds since the Arduino board began running the current program.
 * This number will overflow (go back to zero), after approximately 70 minutes. On the boards from
 * the Arduino Portenta family this function has a resolution of one microsecond on all cores.
 * On 16 MHz Arduino boards (e.g. Duemilanove and Nano), this function has a resolution of four
 * microseconds (i.e. the value returned is always a multiple of four). On 8 MHz Arduino boards
 * (e.g. the LilyPad), this function has a resolution of eight microseconds.
 */
/**************************************************************************/
/*!
    @brief  Set the current date/time of the RTC_Micros clock.
    @param dt DateTime object with the desired date and time
*/
/**************************************************************************/
void RTC_Micros::adjust(const DateTime &dt) {
  lastMicros = micros();
  lastUnix = dt.unixtime();
}

/**************************************************************************/
/*!
    @brief  Adjust the RTC_Micros clock to compensate for system clock drift
    @param ppm Adjustment to make. A positive adjustment makes the clock faster.
*/
/**************************************************************************/
void RTC_Micros::adjustDrift(int ppm) { microsPerSecond = 1000000 - ppm; }

/**************************************************************************/
/*!
    @brief  Get the current date/time from the RTC_Micros clock.
    @return DateTime object containing the current date/time
*/
/**************************************************************************/
DateTime RTC_Micros::now() {
  uint32_t elapsedSeconds = (micros() - lastMicros) / microsPerSecond;
  lastMicros += elapsedSeconds * microsPerSecond;
  lastUnix += elapsedSeconds;
  return lastUnix;
}
