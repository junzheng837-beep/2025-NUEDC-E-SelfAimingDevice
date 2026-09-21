/**
 * @file timer.c
 * @brief 定时器中断及核心控制任务调度
 * @details 处理系统定时任务，包括按键扫描、传感器读取、PID控制计算以及电机输出控制等。
 * @author WILLiam
 * @date 2026-06-12
 */

#include "timer.h"
#include "key.h"
#include "usart.h"
#include "Encoder.h"
#include "motor_ctrl.h"
#include "protocol.h"
#include "gw_gray.h"
#include "task.h"
#include "bsp_gyro.h"

// ==================== 全局变量 ====================

uint16_t count_10ms = 0;
uint16_t count_100ms = 0;
uint16_t t = 0;
volatile uint32_t ms_ticks = 0;

uint32_t get_micros(void) {
    uint32_t ms = ms_ticks;
    uint32_t count = DL_TimerG_getTimerCount(TIMER_0_INST);
    
    // Use getRawInterruptStatus to check without clearing the hardware flag!
    if (DL_TimerG_getRawInterruptStatus(TIMER_0_INST, DL_TIMERG_INTERRUPT_ZERO_EVENT)) {
        count = DL_TimerG_getTimerCount(TIMER_0_INST);
        ms++;
    } else if (ms != ms_ticks) {
        ms = ms_ticks;
        count = DL_TimerG_getTimerCount(TIMER_0_INST);
    }
    
    return (ms * 1000) + (count * 1000) / 625;
}


volatile float Target_ChaSu;        // 目标差速
volatile float Motor1_Target_Speed; // 左边电机目标速度
volatile float Motor2_Target_Speed; // 右边电机目标速度

uint8_t huidu_state = 0;
volatile uint32_t control_cnt = 0;

// 全局运行标志位（供主循环轮询使用）
volatile uint8_t flag_5ms_gyro_read = 0;
volatile uint8_t flag_1s_printf = 0;
volatile uint8_t flag_50ms_telemetry = 0;
volatile uint8_t flag_50ms_lcd = 0;
volatile uint16_t telemetry_pause_ms = 0;



float Task1_Time_Sec = 0.0f; // 任务一用时
float Task2_Time_Sec = 0.0f; // 任务二用时
float Task1_Dist = 0.0f;     // 任务一当前路程
float Task2_Dist = 0.0f;     // 任务二当前路程

// ==================== 外部变量声明 ====================
extern float Basic_Speed;          // 电机目标速度
extern uint8_t OLED_View_Select;   // OLED选择界面变量
extern float Target_Distance;
extern float Target_Gyro;
extern float Target_Angle;

extern uint8_t Turn_PID_Flag;
extern uint8_t Angle_PID_Flag;
extern pid_t pid_Turn;
extern pid_t pid_Angle;
extern Gyro_Struct *JY61P_Data;    // 陀螺仪数据结构体

extern volatile bool g_rx_frame_flag; 
extern int16_t Gimbal_Speed_X;     // 云台控制目标速度变量X
extern int16_t Gimbal_Speed_Y;     // 云台控制目标速度变量Y
extern volatile uint8_t ble_rx_idle_cnt;
extern uint8_t Test_Speed_Mode;
extern float Target_Speed_Test;

// ==================== 内部函数声明 ====================
static void Timer_10ms_Control_Task(void);

/**
 * @brief 定时器0中断服务函数（1ms周期）
 * @details 负责系统的1ms基准定时，调度5ms、10ms、100ms及1s周期任务。
 */
void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) 
    {
        case DL_TIMER_IIDX_ZERO: // TIMG 的加载归零中断标志是 IIDX_ZERO
        {
            ms_ticks++;
            ble_rx_idle_cnt++;
            
            if (telemetry_pause_ms > 0) 
            {
                telemetry_pause_ms--;
            }
            control_cnt++;

            // 5ms 周期任务处理
            if (control_cnt >= 5) 
            {
                flag_5ms_gyro_read = 1;
                control_cnt = 0;
            }

            // 10ms 周期任务处理
            if (++count_10ms >= 10) 
            {
                count_10ms = 0;
                Timer_10ms_Control_Task();
            }
             
            // 100ms 周期任务处理
            if (++count_100ms >= 100) 
            {
                count_100ms = 0;
            }
             
            // 1000ms 周期任务处理
            if (++t >= 1000) 
            {
                t = 0;
                flag_1s_printf = 1; // 1秒打印标记，留给主循环执行
                LED3_TOGGLE();
                LED1_TOGGLE();
            }
        }
        break;
        
        default:
            break;
    }
}

/**
 * @brief 10ms 控制任务核心逻辑
 * @details 包括传感器读取、里程积分、PID闭环计算及电机控制输出。
 */
static void Timer_10ms_Control_Task(void)
{
    static uint8_t cnt_50ms = 0;
    
    // 20ms 标志位生成 (极限提速 50Hz)
    if (++cnt_50ms >= 2) 
    {
        cnt_50ms = 0;
        flag_50ms_lcd = 1;
        if (telemetry_pause_ms == 0) 
        {
            flag_50ms_telemetry = 1;
        }
    }

    // 任务时间及里程积分
    if (Task1_Time_flag == 1)
    {
        Task1_Time_Sec += 0.01f;
        Task1_Dist += (Motor1_Speed + Motor2_Speed) / 2.0f; // 累加路程
    }
    else if (Task2_Time_flag == 1) 
    {
        Task2_Time_Sec += 0.01f;
        Task2_Dist += (Motor1_Speed + Motor2_Speed) / 2.0f; // 累加路程
    }

    // PD42S1 的云台控制保持独立运行，不受 Car_Mode 影响
    Car_Set_Speed_Unified(Gimbal_Speed_X, Gimbal_Speed_Y);
    
    // 传感器状态更新
    Key_Read(); 
    Huidu_Read(); 
    
    // ==========================================================
    // 核心控制逻辑：根据工作模式解耦运行策略（纯旋转、循迹）
    // ==========================================================
    
    // 跨模式状态保留变量集中声明
    static float Soft_Basic_Speed = 0.0f; // 软启动基础速度
    static uint8_t last_mode = 0;         // 记录上一次的模式状态
    
    float target_speed_1 = 0;
    float target_speed_2 = 0;
    float target_turn = 0.0f;

    // 根据当前工作模式执行相应的控制算法
    if (Car_Mode == Angle_Mode)  // 进入独立旋转模式
    {
        Soft_Basic_Speed = 0.0f; 

        if (JY61P_Data != NULL) 
        {
            target_turn = PID_Calculate(&pid_Angle, JY61P_Data->z, Target_Angle); 
            float angle_err = Target_Angle - JY61P_Data->z;
            
            // 1. 角度死区判断（±0.5度）
            // 适当放宽死区范围，避免电机在微小误差下发生稳态拉扯和高频抖动
            if (angle_err >= -0.5f && angle_err <= 0.5f) 
            {
                target_turn = 0.0f;
                
                pid_Angle.KiOut = 0; pid_Angle.PID_Out = 0;
                pid_Motor1_Speed.KiOut = 0; pid_Motor1_Speed.PID_Out = 0;
                pid_Motor2_Speed.KiOut = 0; pid_Motor2_Speed.PID_Out = 0;
            }
            else 
            {
                // 2. 摩擦力补偿控制
                // 增加低速基础补偿量，防止电机在极低占空比下发生死区堵转卡顿
                float min_power = 2.0f; 
                if (target_turn > 0.0f && target_turn < min_power) 
                {
                    target_turn = min_power; 
                } 
                else if (target_turn < 0.0f && target_turn > -min_power) 
                {
                    target_turn = -min_power;
                }
            }
        }
        
        // 3. 旋转最高限幅
        if (target_turn > 15.0f)  target_turn = 15.0f;
        if (target_turn < -15.0f) target_turn = -15.0f;

        // 4. 【终极绝招：动态交叉耦合防平移 (Anti-Slip)】
        // 理论上原地旋转时，左轮速度和右轮速度一正一负，相加绝对等于0。
        // 如果相加不等于0，说明车体几何中心发生平移。
        float slip_error = Motor1_Speed + Motor2_Speed; 

        // 将瞬间滑移速度乘以P系数，化作纠正力。
        float sync_comp = slip_error * 0.3f; 

        // 严格限制纠正力的上限，绝对避免电机的抽搐和卡顿
        if (sync_comp > 3.0f) sync_comp = 3.0f;
        if (sync_comp < -3.0f) sync_comp = -3.0f;

        // 5. 下发控制：在完美对称的旋转差速上，叠加抵抗瞬间滑移的拉力
        // 原先 M1是右，M2是左。现在 M1是左，M2是右。
        // 为了保持和以前一样的物理转向方向，必须交换它们的公式。
        target_speed_1 = -target_turn - sync_comp; 
        target_speed_2 = target_turn - sync_comp; 
    }
    else // 其他模式 (如 Run_Mode / 循迹模式)
    {
        last_mode = Car_Mode; // 更新模式记录
        Huidu_Proc(Huidu_Datas);
        
        // 【注：已强行拆除“悬空断电保护”】
        // 原来的保护逻辑只要探头全白 50ms 就会切断所有动力，导致过弯丢线时“跑跑停停”且丧失转向力！
        // 拆除后，探头全白时将交由 gw_gray 内部的动态抗干扰与脱轨救车逻辑处理！

        // 软启动平滑过度
        if (Soft_Basic_Speed < Basic_Speed) Soft_Basic_Speed += 0.5f;
        else if (Soft_Basic_Speed > Basic_Speed) Soft_Basic_Speed -= 0.5f;

            // 由按键控制的定时定量脱机测试 (跑一段自动停 + 自动计分)
            extern uint8_t Tracking_Test_Flag;
            static uint16_t auto_stop_timer = 0;
            static float current_run_score = 0;
            
            if (Tracking_Test_Flag == 1) 
            {
                static float last_score_error = 0;
                if (auto_stop_timer == 0) {
                    current_run_score = 0.0f; // 起步时清空上一轮的误差积分
                    last_score_error = Huidu_Error;
                }
                
                extern float Target_Speed_Test;
                Basic_Speed = Target_Speed_Test;
                MOTOR1_ENABLE_FLAG = 1;
                MOTOR2_ENABLE_FLAG = 1;
                Turn_PID_Flag = 1;
                
                auto_stop_timer++;
                
                // 【核心算法升级：引入“抖动惩罚”】
                // 1. 基础误差（绝对值）
                float abs_err = (Huidu_Error < 0) ? -Huidu_Error : Huidu_Error;
                
                // 2. 计算误差的变化率（即车头晃动的剧烈程度）
                float delta_err = Huidu_Error - last_score_error;
                float abs_delta = (delta_err < 0) ? -delta_err : delta_err;
                
                // 3. 综合得分 = 基础误差 + 抖动惩罚（权重设为5倍，严打锯齿状过弯！）
                // 这样一来，平滑走大圈的 3.5 得分会很低，而“分次大拐头”的 4.0 会被罚出高分。
                current_run_score += (abs_err + abs_delta * 5.0f);
                
                last_score_error = Huidu_Error;
                
                // 以 10ms 周期计算，400 次就是 4.0 秒。
                if (auto_stop_timer >= 400) 
                {
                    extern uint8_t Tuning_State;
                    Tracking_Test_Flag = 0; // 4.0 秒后自动熄火刹车！
                    Tuning_State = 0; // 变成 WAIT 状态
                    
                    // 刹车瞬间，通过蓝牙把“本次得分”发给电脑的自动化脚本
                    extern void BLE_send_String(unsigned char *str);
                    char msg[64];
                    sprintf(msg, "[AUTO_SCORE]:%d\r\n", (int)current_run_score);
                    BLE_send_String((unsigned char*)msg);
                }
            } 
            else 
            {
                auto_stop_timer = 0; // 熄火状态下，计时器归零等待下次双击
                Basic_Speed = 0.0f;
                if (Test_Speed_Mode == 0) {
                    MOTOR1_ENABLE_FLAG = 0;
                    MOTOR2_ENABLE_FLAG = 0;
                }
                Turn_PID_Flag = 0;
                target_turn = 0;
                pid_Turn.KpOut = 0; pid_Turn.KiOut = 0; pid_Turn.KdOut = 0; pid_Turn.PID_Out = 0;
            }

            if (Turn_PID_Flag == 1) 
            {
                target_turn = PID_Calculate(&pid_Turn, Huidu_Error, 0); 
                // 响应调试需求：彻底移除 max_turn 限制，让转向环差速值 100% 毫无保留地下发给速度环
            }

            // M1 左轮，M2 右轮 (与 spin mode 第247行注释一致)
            target_speed_1 = Soft_Basic_Speed - target_turn; // 左轮
            target_speed_2 = Soft_Basic_Speed + target_turn; // 右轮
    }

    // 测试模式下的强制速度设定
    static float Test_Soft_Speed = 0.0f;
    static float locked_yaw = 0.0f;
    static uint8_t gyro_lock_init = 0;

    // 恢复悬空测速支持
    if (Test_Speed_Mode == 1) 
    {
        // 恢复软启动，避免起步打滑
        if (Test_Soft_Speed < Target_Speed_Test) Test_Soft_Speed += 0.6f;
        else if (Test_Soft_Speed > Target_Speed_Test) Test_Soft_Speed -= 0.6f;
        
        if (Target_Speed_Test != 0 && JY61P_Data != NULL) 
        {
            if (gyro_lock_init == 0) 
            {
                locked_yaw = JY61P_Data->total_z; // 锁定起步瞬间的车头航向角
                gyro_lock_init = 1;
                // 清理陀螺仪 PID 历史积分
                pid_Gyro.KiOut = 0;
                pid_Gyro.PID_Out = 0;
                pid_Gyro.Error[0] = 0;
                pid_Gyro.Error[1] = 0;
                pid_Gyro.Error[2] = 0;
            }
            // 计算航向补偿（P 和 D 用于抵抗偏航）
            float turn_comp = PID_Calculate(&pid_Gyro, JY61P_Data->total_z, locked_yaw);
            
            // 限制最大补偿量，防止车身剧烈晃动
            if (turn_comp > 15.0f) turn_comp = 15.0f;
            if (turn_comp < -15.0f) turn_comp = -15.0f;
            
            target_speed_1 = Test_Soft_Speed - turn_comp;
            target_speed_2 = Test_Soft_Speed + turn_comp;
        }
        else 
        {
            gyro_lock_init = 0;
            target_speed_1 = Test_Soft_Speed;
            target_speed_2 = Test_Soft_Speed;
        }
    }
    else 
    {
        Test_Soft_Speed = 0.0f; // 退出测试模式时清零
        gyro_lock_init = 0;
    }
    
    // 统一执行速度环 PID 并输出给直流电机
    MEASURE_MOTORS_SPEED(); 
    float pwm1 = PID_Calculate(&pid_Motor1_Speed, Motor1_Speed, target_speed_1);
    float pwm2 = PID_Calculate(&pid_Motor2_Speed, Motor2_Speed, target_speed_2);

    // ==========================================================
    // 约束：强制动力绝对对称（防平移核心）
    // ==========================================================
    if (Car_Mode == Angle_Mode && Test_Speed_Mode == 0)
    {
        if (target_speed_1 == 0.0f && target_speed_2 == 0.0f)
        {
            // 【绝招 1】：停车死区时，彻底切断底层 PWM！
            // 避免两轮编码器读数差异导致算出差异刹车，引发意外位移
            pwm1 = 0;
            pwm2 = 0;
            
            // 清除速度环残留积分
            pid_Motor1_Speed.KiOut = 0; pid_Motor1_Speed.PID_Out = 0;
            pid_Motor2_Speed.KiOut = 0; pid_Motor2_Speed.PID_Out = 0;
        }
        else
        {
            // 【绝招 2】：电子差速锁算法
            // 提取两者绝对值求平均，强行赋予一正一负，保证底盘受力绝对对称
            float abs_pwm1 = (pwm1 > 0) ? pwm1 : -pwm1;
            float abs_pwm2 = (pwm2 > 0) ? pwm2 : -pwm2;
            float avg_pwm = (abs_pwm1 + abs_pwm2) / 2.0f;

            // 根据目标速度的方向，重新分配绝对相等的 PWM
            pwm1 = (target_speed_1 > 0) ? avg_pwm : -avg_pwm;
            pwm2 = (target_speed_2 > 0) ? avg_pwm : -avg_pwm;
        }
    }

    // 设置底层电机占空比
    SET_MOTORS_SPEED((int)pwm1, (int)pwm2);
}

/**
 * @brief 定时器1中断服务函数（3ms周期）
 */
void TIMER_1_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_1_INST)) 
    {
        case DL_TIMER_IIDX_ZERO:
            break;
        default:
            break;
    }
}

/**
 * @brief 定时器2中断服务函数（10us周期）
 * @note 触发频率不可过高，防止中断过载
 */
void TIMER_2_INST_IRQHandler(void) 
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_2_INST)) 
    {
        case DL_TIMER_IIDX_LOAD:
            break;
        default:
            break;
    }
}

/**
 * @brief 系统中断配置初始化
 * @details 清除并使能所需的所有外设（串口、定时器、编码器等）中断。
 */
void NVIC_EnableIRQ_Init(void)
{
    // 清除串口中断标志
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_MOTOR_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(UART_MOTOR_2_INST_INT_IRQN);
    
    // 使能串口中断
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_MOTOR_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_MOTOR_2_INST_INT_IRQN);
    
    // 清除定时器中断标志
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(TIMER_1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(TIMER_2_INST_INT_IRQN);
    
    // 使能定时器中断
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
    NVIC_EnableIRQ(TIMER_2_INST_INT_IRQN);
    
    // 启动定时器A计数
    DL_TimerG_startCounter(TIMER_0_INST);
    
    // 编码器/超声波 (PORTA) 中断使能
    NVIC_EnableIRQ(GPIOA_INT_IRQn);
    
    // 陀螺仪中断使能（如果需要）
    // NVIC_EnableIRQ(MPU6050_INT_IRQN);
}