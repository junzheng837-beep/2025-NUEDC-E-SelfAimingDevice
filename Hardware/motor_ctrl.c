#include "motor_ctrl.h"
#include <math.h> // 必须包含这个头文件，因为用到了 fabs() 求绝对值
#include "main.h"
#include "smd.h"

uint8_t MOTOR1_ENABLE_FLAG = 0; // 电机使能标志位
uint8_t MOTOR2_ENABLE_FLAG = 0;

uint8_t Turn_PID_Flag = 0;
uint8_t Distance_PID_Flag = 0;
uint8_t Gyro_PID_Flag = 0;
uint8_t Angle_PID_Flag = 0;
uint8_t Stand_PID_Flag = 0;

// 声明在 app_protocol.c 中定义的模式变量
extern uint8_t K230_Ctrl_Mode;

/*---------------------------------------------------------------------------------------------------*/
/*---------------------------------------PID计算部分-------------------------------------------------*/
/*---------------------------------------------------------------------------------------------------*/

// 速度环参数
pid_t pid_Motor1_Speed = {
    .Kp = 35.000f,  // 针对重车体强化内功：拉大 Kp，让电机输出更爆裂的瞬时扭矩抵抗惯性
    .Ki = 5.000f,   // 加大 Ki，让大质量车身能在极短时间内消除稳态速度误差
    .Kd = 0.040f,
    .Target = 0,
    .Measure = 0,
    {0, 0, 0},
    .KpOut = 0,
    .KiOut = 0,
    .KdOut = 0,
    .PID_Out = 0,
    .PID_Limit_MAX = 9999,
    .Ki_Limit_MAX = 9000,
};

pid_t pid_Motor2_Speed = {
    .Kp = 35.000f,  // 左右轮保持一致，内功大成，无视配重带来的响应迟滞
    .Ki = 5.000f,
    .Kd = 0.040f,
    .Target = 0,
    .Measure = 0,
    {0, 0, 0},
    .KpOut = 0,
    .KiOut = 0,
    .KdOut = 0,
    .PID_Out = 0,
    .PID_Limit_MAX = 9999,
    .Ki_Limit_MAX = 9000,
};

// 转向环参数 (大负载、长前探专用起步参数)
pid_t pid_Turn = {
    .Kp = 3.0f,  // 35.0极速重载版：3.0完美比例，彻底消除直道轻微抖动
    .Ki = 0.0f,  // 零积分：彻底抛弃积分修正，防止任何潜在的长周期画龙
    .Kd = 100.0f, // 35.0极速重载版：100.0恰到好处的阻尼，抗离心力且不放大噪声
    .Target = 0,
    .Measure = 0,
    {0, 0, 0},
    .KpOut = 0,
    .KiOut = 0,
    .KdOut = 0,
    .PID_Out = 0,
    .PID_Limit_MAX = 9999,
    .Ki_Limit_MAX = 500,
};

// 距离环参数
pid_t pid_Distance = {
    .Kp = 0.8f,
    .Ki = 0.0f,
    .Kd = 0.0f,
    .Target = 0,
    .Measure = 0,
    {0, 0, 0},
    .KpOut = 0,
    .KiOut = 0,
    .KdOut = 0,
    .PID_Out = 0,
    .PID_Limit_MAX = 100,
    .Ki_Limit_MAX = 100,
};

// 角速度环参数
pid_t pid_Gyro = {
    .Kp = 0.3f,  // 极度柔和的比例控制：Kp调小，让修正动作如丝般顺滑，消除硬拽感
    .Ki = 0.0f,  // 坚决剔除积分：绝不让历史误差累积，杜绝一切长周期画龙（低频震荡）
    .Kd = 0.0f,  // 彻底剔除微分：因为离散差分会极度放大陀螺仪的高频噪声，导致车身剧烈抖动
    .Target = 0,
    .Measure = 0,
    {0, 0, 0},
    .KpOut = 0,
    .KiOut = 0,
    .KdOut = 0,
    .PID_Out = 0,
    .PID_Limit_MAX = 20,
    .Ki_Limit_MAX = 0,
};

// 角度环参数
pid_t pid_Angle = {
    .Kp = 0.12f,
    .Ki = 0.0f,
    .Kd = 1.0f,
    .Target = 0,
    .Measure = 0,
    {0, 0, 0},
    .KpOut = 0,
    .KiOut = 0,
    .KdOut = 0,
    .PID_Out = 0,
    .PID_Limit_MAX = 3000,
    .Ki_Limit_MAX = 3000,
};

/**
  * @brief  K230云台专用的统一速度下发接口
  * @param  speed_L: X轴/Yaw偏航 速度
  * @param  speed_R: Y轴/Pitch俯仰 速度
  */
void Car_Set_Speed_Unified(float speed_L, float speed_R)
{
    // 核心拦截：如果当前 K230 是上电位置模式 (0)，直接 return
    // 坚决不允许后台定时器再发任何 smd_speed_mode 指令
    if (K230_Ctrl_Mode == 0) return; 

    /* ================= 处理 X 轴电机 (UART1) ================= */
    g_smd_target_uart = 1; 
    uint8_t dir_L = (speed_L >= 0) ? 0 : 1; 
    smd_speed_mode(1, dir_L, 5, (uint32_t)fabs(speed_L)); 

    /* ================= 处理 Y 轴电机 (UART2) ================= */
    g_smd_target_uart = 2; 
    uint8_t dir_R = (speed_R >= 0) ? 0 : 1; 
    smd_speed_mode(1, dir_R, 5, (uint32_t)fabs(speed_R)); 
}

// 通用限幅函数 (float)
void PID_Limit(float *a, float ABS_MAX)
{
    if (*a > ABS_MAX)
        *a = ABS_MAX;
    if (*a < -ABS_MAX)
        *a = -ABS_MAX;
}

/*************************************************************************************************
*   函 数 名:   PID_Calculate
*   函数功能:   PID位置式计算公式
*   参    数:   PID结构体指针，当前测量值，目标值
*************************************************************************************************/
float PID_Calculate(pid_t *pid, float Measure, float Target)
{
    pid->Target = Target;
    pid->Measure = Measure;
    pid->Error[0] = Target - Measure;

    pid->KpOut = pid->Kp * pid->Error[0];
    pid->KiOut += pid->Ki * pid->Error[0];
    PID_Limit(&(pid->KiOut), pid->Ki_Limit_MAX);
    pid->KdOut = pid->Kd * (pid->Error[0] - pid->Error[1]);

    pid->PID_Out = pid->KpOut + pid->KiOut + pid->KdOut;
    PID_Limit(&(pid->PID_Out), pid->PID_Limit_MAX);

    pid->Error[2] = pid->Error[1];
    pid->Error[1] = pid->Error[0];

    return pid->PID_Out;
}

void Set_PID_Param(pid_t *pid, float P, float I, float D)
{
    pid->Kp = P;
    pid->Ki = I;
    pid->Kd = D;
    pid->KpOut = 0;
    pid->KiOut = 0;
    pid->KdOut = 0;
    pid->PID_Out = 0;
}

// 通用限幅函数 (int)
void PWM_Limit(int *a, int ABS_MAX)
{
    if (*a > ABS_MAX)
        *a = ABS_MAX;
    if (*a < -ABS_MAX)
        *a = -ABS_MAX;
}

// 设置电机1的PWM占空比
void Set_Motor1_PWM(int Target_PWM)
{
    PWM_Limit(&Target_PWM, 9999);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, Target_PWM, GPIO_PWM_0_C1_IDX); // 映射到真实的左轮硬件
}

// 设置电机2的PWM占空比
void Set_Motor2_PWM(int Target_PWM)
{
    PWM_Limit(&Target_PWM, 9999);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, Target_PWM, GPIO_PWM_0_C0_IDX); // 映射到真实的右轮硬件
}

// 设置电机1的速度
void Set_Motor1_Speed(int Target_Speed)
{
    if (MOTOR1_ENABLE_FLAG == 1)
    {
        if (Target_Speed >= 0) // 正转
        {
            Set_Motor1_PWM(Target_Speed);
            Motor1_Forward();
        }
        else // 反转
        {
            Set_Motor1_PWM(-Target_Speed);
            Motor1_Backward();
        }
    }
    else 
    {
        Motor1_Stop();
        pid_Motor1_Speed.KpOut = 0;
        pid_Motor1_Speed.KiOut = 0;
        pid_Motor1_Speed.KdOut = 0;
        pid_Motor1_Speed.PID_Out = 0;
    }
}

// 设置电机2的速度
void Set_Motor2_Speed(int Target_Speed)
{
    if (MOTOR2_ENABLE_FLAG == 1)
    {
        if (Target_Speed >= 0) // 正转
        {
            Set_Motor2_PWM(Target_Speed);
            Motor2_Forward();
        }
        else // 反转
        {
            Set_Motor2_PWM(-Target_Speed);
            Motor2_Backward();
        }
    }
    else 
    {
        Motor2_Stop();
        pid_Motor2_Speed.KpOut = 0;
        pid_Motor2_Speed.KiOut = 0;
        pid_Motor2_Speed.KdOut = 0;
        pid_Motor2_Speed.PID_Out = 0;
    }
}

// 设置所有电机速度
void SET_MOTORS_SPEED(int Target_Motor1_Speed, int Target_Motor2_Speed)
{
    Set_Motor1_Speed(Target_Motor1_Speed);
    Set_Motor2_Speed(Target_Motor2_Speed);
}
