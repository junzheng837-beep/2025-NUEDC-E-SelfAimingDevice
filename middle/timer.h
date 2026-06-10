#ifndef __TIMER_H__
#define __TIMER_H__

#include "main.h"
#include "key.h"
#include "usart.h"
#include "Encoder.h"
#include "motor_ctrl.h"
#include "protocol.h"
#include "gw_gray.h"
// #include "mpu6050.h"
// #include "bsp_gyro.h"

// extern Gyro_Struct *JY61P_Data;
extern uint32_t warn_cnt;

extern uint16_t count_10ms;
extern uint16_t count_100ms;
extern uint16_t t;

void NVIC_EnableIRQ_Init(void);

#endif
