#include "task.h"
#include <math.h>
#include "motor_ctrl.h"
#include "Encoder.h"
#include "gw_gray.h"
#include "usart.h"
#include "key.h"
#include "delay.h"
#include "timer.h"
// #include "mpu6050.h"
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
float Target_Speed_Test = 30.0f;
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
                // 待机模式：统一关闭电机，防止乱跑
                MOTOR1_ENABLE_FLAG = 0; 
                MOTOR2_ENABLE_FLAG = 0;
                Basic_Speed = 0;
            }
            break;
            
        case 1:
            Task_1(); // 只有 Task_Mode == 1 时，任务一才有资格运行
            break;
            
        case 2:
            Task_2(); // 只有 Task_Mode == 2 时，任务二才有资格运行
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
            // 👇================ 新增：终点前预判减速逻辑 ================👇
            // 终点是 710 度，我们提前约 60度 (大概终点前 15~20 厘米) 开始强制拉刹车
            if (diff >= 680.0f && diff < 710.0f) 
            {
                Basic_Speed = 10; // 强行把基础速度降到 15 (爬行速度过线)
                
                // 【极其关键】：65高速下用的狂暴 PID 放到 15 的低速下，会让车子剧烈摇摆画龙！
                // 所以减速的同时，必须把 PID 的 P 和 D 压下来，让它平稳滑行过终点。
                pid_Turn.Kp = 2.5f;   // 降回平稳的 P (可微调 2.0~3.0)
                pid_Turn.Kd = 100.0f; // 降回平稳的 D (可微调 80~150)
            }
            // 👆==========================================================👆
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
            
           // 🌟🌟🌟 把 P 和 D 直接翻倍甚至数倍！
            pid_Turn.Kp = 6.0;   // 【重拳 P】：从 1.7 提高到 5.0~8.0 左右。强行克服前轮僵硬！
            pid_Turn.Ki = 0.0;  
            pid_Turn.Kd = 400.0; // 【爆破 D】：遇到黑线瞬间，给电机几百的差速值，把车头生生砸回去！
            
            // 彻底清空你在摆放时积攒的垃圾数据！
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
            
            Basic_Speed = 25;    // 先用中低速测试，一旦它能咬住线了，再提速。              Task1_Time_Sec = 0.0f; // 重置计时
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
            // 👇================ 新增：终点前预判减速逻辑 ================👇
            // 终点是 710 度，我们提前约 60度 (大概终点前 15~20 厘米) 开始强制拉刹车
            if (diff >= 325.0f && diff < 355.0f) 
            {
                Basic_Speed = 10; // 强行把基础速度降到 15 (爬行速度过线)
                
                // 【极其关键】：65高速下用的狂暴 PID 放到 15 的低速下，会让车子剧烈摇摆画龙！
                // 所以减速的同时，必须把 PID 的 P 和 D 压下来，让它平稳滑行过终点。
                pid_Turn.Kp = 2.5f;   // 降回平稳的 P (可微调 2.0~3.0)
                pid_Turn.Kd = 100.0f; // 降回平稳的 D (可微调 80~150)
            }
            // 👆==========================================================👆
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
            
            pid_Turn.Kp = 6.0;   // 【重拳 P】：从 1.7 提高到 5.0~8.0 左右。强行克服前轮僵硬！
            pid_Turn.Ki = 0.0;  
            pid_Turn.Kd = 400.0; // 【爆破 D】：遇到黑线瞬间，给电机几百的差速值，把车头生生砸回去！
            
            // 彻底清空你在摆放时积攒的垃圾数据！
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
            
            Basic_Speed = 25;    // 【修改点】：降速！后驱推着跑极易失控，先用 30 甚至 20 的低速把环调通，再慢慢加速。
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