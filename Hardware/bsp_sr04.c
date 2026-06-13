#include "bsp_sr04.h"

volatile uint32_t msHcCount = 0;
float distance = 0;
volatile uint8_t SR04_Flag = 0; 

void SR04_Init(void)
{
    NVIC_ClearPendingIRQ(TIMER_SR04_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_SR04_INST_INT_IRQN);
}

void Open_Timer(void)
{
    DL_TimerG_setTimerCount(TIMER_SR04_INST, 0); 
    msHcCount = 0;
    DL_TimerG_startCounter(TIMER_SR04_INST); 
}

uint32_t Get_TIMER_Count(void)
{
    uint32_t time = msHcCount * 1000;
    time += DL_TimerG_getTimerCount(TIMER_SR04_INST);
    DL_TimerG_setTimerCount(TIMER_SR04_INST, 0);
    return time;
}

void Close_Timer(void)
{
    DL_TimerG_stopCounter(TIMER_SR04_INST);
}

// 1ms 中断
void TIMER_SR04_INST_IRQHandler(void)
{
    switch(DL_TimerG_getPendingInterrupt(TIMER_SR04_INST))
    {
        case DL_TIMER_IIDX_LOAD:
            msHcCount++;
            break;
        default:
            break;
    }
}

// 这个函数在 Encoder.c 的 GROUP1_IRQHandler 中被调用
void SR04_ECHO_IRQHandler(void)
{
    // ECHO 引脚状态
    if(DL_GPIO_readPins(SR04_PORT, SR04_ECHO_PIN)) // 上升沿
    {
        SR04_Flag = 0;
        distance = 0.0f;
        Open_Timer();
    }
    else // 下降沿
    {
        Close_Timer();
        SR04_Flag = 1;
        distance = (float)Get_TIMER_Count() / 58.0f; 
    }
}

float SR04_GetLength(void)
{
    uint32_t TimeOut = 400000; // 约40ms 超时
    
    msHcCount = 0;
    SR04_Flag = 0;

    DL_GPIO_enableInterrupt(SR04_PORT, SR04_ECHO_PIN);
    
    // 短暂延时
    for(volatile int j=0; j<3000; j++);

    DL_GPIO_clearPins(SR04_PORT, SR04_TRIG_PIN);
    for(volatile int j=0; j<100; j++);
    DL_GPIO_setPins(SR04_PORT, SR04_TRIG_PIN);
    for(volatile int j=0; j<400; j++);
    DL_GPIO_clearPins(SR04_PORT, SR04_TRIG_PIN);

    while (SR04_Flag == 0 && TimeOut)
    {
        TimeOut--;
    }

    DL_GPIO_disableInterrupt(SR04_PORT, SR04_ECHO_PIN);

    if (TimeOut == 0) return 999.0f; // Timeout indicates out of range

    return distance;
}
