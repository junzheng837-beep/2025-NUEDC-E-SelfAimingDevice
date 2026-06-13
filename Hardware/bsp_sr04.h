#ifndef __BSP_SR04_H__
#define __BSP_SR04_H__

#include "ti_msp_dl_config.h"

void SR04_Init(void);
float SR04_GetLength(void);
void SR04_ECHO_IRQHandler(void);

#endif
