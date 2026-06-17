#include "task.h"
#include <math.h>
#include "motor_ctrl.h"
#include "Encoder.h"
#include "gw_gray.h"
#include "usart.h"
#include "key.h"
#include "delay.h"
#include "timer.h"
#include "bsp_gyro.h"
#include "smd.h"
#include "process_frame.h"

// =================================================================
// 在 task.c 顶部的变量定义区，接管 Task_Mode 变量
// =================================================================
uint8_t Task_Mode = 0; // 0:待机, 1:跑任务一, 2:跑任务二

uint8_t Task1_Time_flag = 0;
uint8_t Task2_Time_flag = 0;
uint8_t Test_Speed_Mode = 0;
float Target_Speed_Test = 45.0f;
// 新增引入编码器计算的里程变量
extern float Measure_Distance;
extern float Motor1_Lucheng;
extern float Motor2_Lucheng;
// =================================================================
// task.c 大总管调度器函数
// =================================================================
void Task_Scheduler(void)
{
    switch (Task_Mode)
    {
        case 0:
            if (Test_Speed_Mode == 1) {
                // 测试模式下，保持电机使能并设定速度
                MOTOR1_ENABLE_FLAG = 1; 
                MOTOR2_ENABLE_FLAG = 1;
                Basic_Speed = Target_Speed_Test;
            } else {
                // 待机模式：统一关闭电机使能，确保静止
                MOTOR1_ENABLE_FLAG = 0; 
                MOTOR2_ENABLE_FLAG = 0;
                Basic_Speed = 0;
            }
            break;
            
        case 1:
            Task_1(); // 调用任务一状态机
            break;
            
        case 2:
            Task_2(); // 调用任务二状态机
            break;
            
        default:
            break;
    }
}
// 声明在 usart.c 中定义的全局变量
extern volatile bool g_rx_frame_flag;
extern volatile uint8_t g_rx_len;
extern uint8_t g_rx_cmd[128];
// 新增引入电机2的变量
extern volatile bool g_rx2_frame_flag;
extern volatile uint8_t g_rx2_len;
extern uint8_t g_rx2_cmd[128];

//======================PD电机控制任务======================
/**
 * @brief    处理应答
 * @param    over_time : 超时时间（单位ms）
 * @retval   0：处理成功，1：等待应答超时
 */
uint8_t handle_ack(uint8_t motor_id, uint32_t over_time)
{
    SERIAL_FRAME g_serial_frame;
    
    if (motor_id == 1) 
    {
        while(g_rx_frame_flag == false) {
            delay_ms(1);
            if (--over_time == 0) {
                g_rx_len = 0; g_rx_frame_flag = false; return 1;
            }
        }
        serial_frame_process((uint8_t *)g_rx_cmd, g_rx_len, &g_serial_frame);
        g_rx_frame_flag = false; g_rx_len = 0;
    } 
    else if (motor_id == 2) 
    {
        while(g_rx2_frame_flag == false) {
            delay_ms(1);
            if (--over_time == 0) {
                g_rx2_len = 0; g_rx2_frame_flag = false; return 1;
            }
        }
        serial_frame_process((uint8_t *)g_rx2_cmd, g_rx2_len, &g_serial_frame);
        g_rx2_frame_flag = false; g_rx2_len = 0;
    }
    return 0;
}

// 清除对应电机的接收缓存
void clear_uart_rx_buffer(uint8_t motor_id)
{
    if(motor_id == 1) {
        g_rx_frame_flag = false; 
        g_rx_len = 0;
    } else if(motor_id == 2) {
        g_rx2_frame_flag = false; 
        g_rx2_len = 0;
    }
}

//===================================================================
extern Gyro_Struct *JY61P_Data;
float Debug_Yaw_Diff = 0.0f; // 两个任务共用这个变量推送到LCD显示差值即可

/* ================================================================== */
/* ============================ 任务一 ============================== */
/* ================================================================== */

uint8_t Task_1_State = 0;
uint8_t Lap_Count = 0;
float Start_Yaw = 0.0f; 
static uint8_t Super_Lock = 0;

void Task_1(void)
{
    // ========================================================
    // 1. 全天候计算层
    // ========================================================
    if (JY61P_Data != NULL) 
    {
        float current_z = JY61P_Data->total_z;
        float diff = current_z - Start_Yaw;
        
        if (diff < 0.0f) diff = -diff; 
        
        Debug_Yaw_Diff = diff; 
        
        if (Task_1_State == 2) 
        {
            uint8_t real_laps = 0;
            

            if (diff >= 710.0f) {
                real_laps = 2; 
            } else if (diff >= 355.0f) {
                real_laps = 1; 
            } else {
                real_laps = 0;
            }
            
            if (real_laps > Lap_Count)
            {
                Lap_Count = real_laps; 
                // 100ms 的长鸣
                BEEP_ON();
                LED3_ON(); 
                delay_ms(100); 
                BEEP_OFF();
                LED3_OFF();
                if(Lap_Count >= 2) 
                {
                    Task_1_State = 3; 
                }
            }
            // ================ 新增：终点前预判减速逻辑 ================
            // 终点阈值 710 度，提前约 60 度开始强制减速逻辑
            if (diff >= 680.0f && diff < 710.0f) 
            {
                Basic_Speed = 10; // 降低基础速度以确保平稳越线
                
                // 关键逻辑：低速下需降低 PID 参数以防止车体剧烈震荡
                // 平滑减小 PID P与D 参数，保证滑行稳定
                pid_Turn.Kp = 2.5f;   // 恢复为平稳控制的 Kp 值
                pid_Turn.Kd = 100.0f; // 恢复为平稳控制的 Kd 值
            }
            // ==========================================================
        }
    }

    // ========================================================
    // 2. 状态机控制层
    // ========================================================
    switch(Task_1_State)
    {
        case 0: // 初始待机
            MOTOR1_ENABLE_FLAG = 0; 
            MOTOR2_ENABLE_FLAG = 0;
            Basic_Speed = 0;
            Turn_PID_Flag = 0; 
            Super_Lock = 0; // 允许重复触发
            break;

        case 1: // 启动初始化
            if (Super_Lock == 0) {
                if (JY61P_Data != NULL) Start_Yaw = JY61P_Data->total_z;
                else Start_Yaw = 0.0f;
                
                Lap_Count = 0; 
                Super_Lock = 1; 
                
                // ===== 新增：每次任务一开始时，清空之前累计的路程 =====
                Measure_Distance = 0;
                Motor1_Lucheng = 0;
                Motor2_Lucheng = 0;
                // =======================================================
            }
            
            MOTOR1_ENABLE_FLAG = 1; 
            MOTOR2_ENABLE_FLAG = 1; 
            
           // 提高 P 和 D 参数以增强循迹响应
            pid_Turn.Kp = 6.0;   // 增大 P 参数，克服前轮摩擦力僵硬现象
            pid_Turn.Ki = 0.0;  
            pid_Turn.Kd = 25.0;  // 原始工作值
            
            // 启动前清空 PID 历史误差累积数据，防止初始误判
            pid_Turn.KpOut = 0;
            pid_Turn.KiOut = 0;
            pid_Turn.KdOut = 0;
            pid_Turn.PID_Out = 0;
            pid_Turn.Error[0] = 0;
            pid_Turn.Error[1] = 0;
            pid_Turn.Error[2] = 0;

            Turn_PID_Flag = 1;      // 清理干净后，再开启计算
            Distance_PID_Flag = 0;
            Gyro_PID_Flag = 0;
            Angle_PID_Flag = 0;
            
            Basic_Speed = 25;    // 设置初始测试速度并重置计时器
            Task1_Time_Sec = 0.0f; // 重置计时
            Task_1_State = 2; // 进入行驶
            break;

        case 2: // 行驶状态（维持）
            Task1_Time_flag = 1;
            break;

        case 3: // 彻底完成，停车
            Basic_Speed = 0;
            Turn_PID_Flag = 0; 
            MOTOR1_ENABLE_FLAG = 0; 
            MOTOR2_ENABLE_FLAG = 0; 
                        
            Task1_Time_flag = 0;

            Task_1_State = 4; 
            break;
            
        case 4: 
            break; // 停车死锁状态
            
        default: break;
    }
}


/* ================================================================== */
/* ============================ 任务二 ============================== */
/* ================================================================== */

// 为任务二准备独立的状态、圈数、起跑角度锁等变量
uint8_t Task_2_State = 0;
uint8_t Lap_Count_2 = 0;
float Start_Yaw_2 = 0.0f; 
static uint8_t Super_Lock_2 = 0;

void Task_2(void)
{
    // ========================================================
    // 1. 全天候计算层 (完全复刻任务一，但使用任务二的专属变量)
    // ========================================================
    if (JY61P_Data != NULL) 
    {
        float current_z = JY61P_Data->total_z;
        float diff = current_z - Start_Yaw_2;
        
        if (diff < 0.0f) diff = -diff; 
        
        Debug_Yaw_Diff = diff; // 同步给 LCD 显示
        
        if (Task_2_State == 2) 
        {
            uint8_t real_laps = 0;


            if (diff >= 355.0f) {
                real_laps = 1; 
            } else {
                real_laps = 0;
            }
            
            if (real_laps > Lap_Count_2)
            {
                Lap_Count_2 = real_laps; 
                if(Lap_Count_2 >= 1) 
                {
                    Task_2_State = 3; 
                }
            }
            // ================ 新增：终点前预判减速逻辑 ================
            // 终点阈值 710 度，提前约 60 度开始强制减速逻辑
            if (diff >= 325.0f && diff < 355.0f) 
            {
                Basic_Speed = 10; // 降低基础速度以确保平稳越线
                
                // 关键逻辑：低速下需降低 PID 参数以防止车体剧烈震荡
                // 平滑减小 PID P与D 参数，保证滑行稳定
                pid_Turn.Kp = 2.5f;   // 恢复为平稳控制的 Kp 值
                pid_Turn.Kd = 100.0f; // 恢复为平稳控制的 Kd 值
            }
            // ==========================================================
        }
    }

    // ========================================================
    // 2. 状态机控制层
    // ========================================================
    switch(Task_2_State)
    {
        case 0: // 初始待机
            MOTOR1_ENABLE_FLAG = 0; 
            MOTOR2_ENABLE_FLAG = 0;
            Basic_Speed = 0;
            Turn_PID_Flag = 0; 
            Super_Lock_2 = 0; 
            break;

        case 1: // 启动初始化
            if (Super_Lock_2 == 0) {
                if (JY61P_Data != NULL) Start_Yaw_2 = JY61P_Data->total_z;
                else Start_Yaw_2 = 0.0f;
                
                Lap_Count_2 = 0; 
                Super_Lock_2 = 1; 
            }
            
            MOTOR1_ENABLE_FLAG = 1; 
            MOTOR2_ENABLE_FLAG = 1; 
            
            pid_Turn.Kp = 6.0;   // 增大 P 参数，克服前轮摩擦力僵硬现象
            pid_Turn.Ki = 0.0;  
            pid_Turn.Kd = 25.0;  // 原始工作值
            
            // 启动前清空 PID 历史误差累积数据，防止初始误判
            pid_Turn.KpOut = 0;
            pid_Turn.KiOut = 0;
            pid_Turn.KdOut = 0;
            pid_Turn.PID_Out = 0;
            pid_Turn.Error[0] = 0;
            pid_Turn.Error[1] = 0;
            pid_Turn.Error[2] = 0;

            Turn_PID_Flag = 1;      
            Distance_PID_Flag = 0;
            Gyro_PID_Flag = 0;
            Angle_PID_Flag = 0;
            
            Basic_Speed = 25;    // 设置低速以保证初次循迹稳定性，后续可逐步提速
            Task2_Time_Sec = 0.0f; // 重置计时
            Task_2_State = 2; // 进入行驶
            break;

        case 2: // 行驶状态（维持）
            Task2_Time_flag = 1;
            break;

        case 3: // 彻底完成，停车
            Basic_Speed = 0;
            Turn_PID_Flag = 0; 
            MOTOR1_ENABLE_FLAG = 0; 
            MOTOR2_ENABLE_FLAG = 0; 

            Task2_Time_flag = 0;
            
            Task_2_State = 4; 
            break;
            
        case 4: 
            break; // 停车死锁状态
            
        default: break;
    }
}