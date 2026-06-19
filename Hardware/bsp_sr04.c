#include "bsp_sr04.h"

static volatile uint32_t t_start = 0;
static volatile float sr04_distance = 0.0f;

void SR04_Init(void) {
    // Start the SR04 timer
    DL_TimerG_startCounter(TIMER_SR04_INST);
    // Enable ECHO pin interrupt
    DL_GPIO_enableInterrupt(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN);
    // 强制开启 GPIOB 的 NVIC 中断响应，防止底层未开启导致无法进入中断
    NVIC_EnableIRQ(GPIOB_INT_IRQn);
}

void SR04_Trigger(void) {
    DL_GPIO_setPins(GPIO_SR04_PORT, GPIO_SR04_TRIG_PIN);
    // 使用足够大的延时确保无论 32/40/80 MHz 都能给出大于 10us 的高电平
    delay_cycles(3200); 
    DL_GPIO_clearPins(GPIO_SR04_PORT, GPIO_SR04_TRIG_PIN);
}

float SR04_Get_Distance(void) {
    return sr04_distance;
}

#define SR04_FILTER_N 5
static volatile float distance_buf[SR04_FILTER_N] = {0};
static volatile uint8_t buf_idx = 0;
static volatile uint8_t buf_full = 0;

void SR04_IRQHandler(void) {
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN);
    if (pending & GPIO_SR04_ECHO_PIN) {
        DL_GPIO_clearInterruptStatus(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN);
        if (DL_GPIO_readPins(GPIO_SR04_PORT, GPIO_SR04_ECHO_PIN)) {
            // Rising edge
            t_start = DL_TimerG_getTimerCount(TIMER_SR04_INST);
        } else {
            // Falling edge
            uint32_t t_end = DL_TimerG_getTimerCount(TIMER_SR04_INST);
            uint32_t load_val = DL_TimerG_getLoadValue(TIMER_SR04_INST);
            uint32_t delta = (t_end >= t_start) ? (t_end - t_start) : (load_val - t_start + t_end + 1);
            
            float tick_us = 30000.0f / (float)(load_val + 1);
            float raw_dist = (float)delta * tick_us * 0.017f;

            // 零延迟突变抑制滤波 (Rate Limiter Filter)
            // 物理极限限制：每 30ms 移动的最大距离（50cm/30ms 约等于 16.6 m/s）
            #define MAX_DELTA_CM_PER_30MS 50.0f
            
            static float last_out = -1.0f;
            static uint8_t spike_count = 0;

            if (last_out < 0.0f) {
                // 首次上电初始化
                last_out = raw_dist;
            } else {
                float diff = (raw_dist > last_out) ? (raw_dist - last_out) : (last_out - raw_dist);
                if (diff <= MAX_DELTA_CM_PER_30MS) {
                    // 正常移动速度范围内，零延迟瞬间跟手！
                    last_out = raw_dist;
                    spike_count = 0;
                } else {
                    // 检测到超自然突变（可能是遇到悬崖跳到291，或者是噪声跳到0）
                    if (spike_count == 0) {
                        // 第一次突变，怀疑是噪声毛刺，不更新输出，保持原值
                        spike_count = 1;
                    } else {
                        // 突变连续存在两帧以上，说明是真实的地形切换（例如转角遇到墙），立刻接受新现实
                        last_out = raw_dist;
                        spike_count = 0;
                    }
                }
            }
            sr04_distance = last_out;
        }
    }
}

// 兼容备用：若由于引脚变更导致该端口中断被映射到 GROUP0
void GROUP0_IRQHandler(void) {
    SR04_IRQHandler();
}
