#ifndef __MAIN_H__
#define __MAIN_H__

#include "ti_msp_dl_config.h"

/*-------------------------------------------------------------------------------------------*/
/*-------------------------����ͷ�ļ��ͳ��õ��������Ͷ̹ؼ���--------------------------------*/
/*-------------------------------------------------------------------------------------------*/
#include "stdbool.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "delay.h"
// 步进电机相关头文件 (确保这些文件你已经移植并包含进来了)
#include "process_frame.h"
#include "smd.h"

typedef int32_t s32;
typedef int16_t s16;
typedef int8_t s8;
typedef const int32_t sc32;
typedef const int16_t sc16;
typedef const int8_t sc8;
typedef __IO int32_t vs32;
typedef __IO int16_t vs16;
typedef __IO int8_t vs8;
typedef __I int32_t vsc32;
typedef __I int16_t vsc16;
typedef __I int8_t vsc8;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
typedef const uint32_t uc32;
typedef const uint16_t uc16;
typedef const uint8_t uc8;
typedef __IO uint32_t vu32;
typedef __IO uint16_t vu16;
typedef __IO uint8_t vu8;
typedef __I uint32_t vuc32;
typedef __I uint16_t vuc16;
typedef __I uint8_t vuc8;
typedef float fp32;
typedef double fp64;

/*-------------------------------------------------------------------------------------------*/
/*---------------------------------���������ͱ����ⲿ����------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
void NVIC_EnableIRQ_Init(void);
void Protocol_Datas_Proc(void);
void OLED_Show_Proc(void);
// FIXME : 测试

extern float Basic_Speed;
extern uint8_t OLED_View_Select;
extern float Target_Distance;
extern float Target_Gyro;
extern float Target_Angle;

extern uint8_t Car_Mode;
// С��ģʽ�궨��
#define Run_Mode 0
#define Speed_Mode 1
#define Turn_Mode 2
#define Distance_Mode 3
#define Gyro_Mode 4
#define Angle_Mode 5

#endif
