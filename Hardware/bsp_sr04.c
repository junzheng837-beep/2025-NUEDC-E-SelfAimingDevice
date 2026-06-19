#include "bsp_sr04.h"

static volatile uint32_t t_start = 0;
static volatile float sr04_distance = 0.0f;

void SR04_Init(void) {
    // Start the SR04 timer
    DL_TimerG_startCounter(TIMER_SR04_INST);
    // Enable ECHO pin interrupt
    DL_GPIO_enableInterrupt(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN);
}

void SR04_Trigger(void) {
    DL_GPIO_setPins(GPIO_SR04_PORT, GPIO_SR04_TRIG_PIN);
    delay_cycles(320); // ~10us at 32MHz
    DL_GPIO_clearPins(GPIO_SR04_PORT, GPIO_SR04_TRIG_PIN);
}

float SR04_Get_Distance(void) {
    return sr04_distance;
}

void SR04_IRQHandler(void) {
    uint32_t pending = DL_GPIO_getPendingInterrupt(GPIO_SR04_PORT);
    if (pending & GPIO_SR04_ECHO_PIN) {
        DL_GPIO_clearInterruptStatus(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN);
        if (DL_GPIO_readPins(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN)) {
            // Rising edge
            t_start = DL_TimerG_getTimerCount(TIMER_SR04_INST);
        } else {
            // Falling edge
            uint32_t t_end = DL_TimerG_getTimerCount(TIMER_SR04_INST);
            uint32_t delta = (t_end >= t_start) ? (t_end - t_start) : (65535 - t_start + t_end);
            sr04_distance = delta * 0.017f;
        }
    }
}
