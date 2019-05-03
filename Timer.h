#ifndef _TIMER_H_
#define _TIMER_H_

#include <avr/interrupt.h>

typedef unsigned long Time;

void Timer0_Init();
Time Timer0_GetTime();
void Timer0_StartTimer(Time* StartTime);
uint8_t Timer0_TimeIsOut(Time* StartTime,Time Delay);

#endif