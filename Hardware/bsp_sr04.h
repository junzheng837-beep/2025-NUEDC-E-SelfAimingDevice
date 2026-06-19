#ifndef __BSP_SR04_H
#define __BSP_SR04_H

#include "ti_msp_dl_config.h"

void SR04_Init(void);
void SR04_Trigger(void);
float SR04_Get_Distance(void);
void SR04_IRQHandler(void);

#endif
