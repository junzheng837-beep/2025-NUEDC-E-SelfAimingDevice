#ifndef __TIMER_H__
#define __TIMER_H__

#include "main.h"

extern uint16_t count_10ms;

void NVIC_EnableIRQ_Init(void);
void TIMER_0_INST_IRQHandler(void);
uint32_t get_micros(void);

#endif
