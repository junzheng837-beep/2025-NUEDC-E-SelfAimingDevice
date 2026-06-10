#include "timer.h"
#include "key.h"
#include "usart.h"
#include "Encoder.h"
#include "motor_ctrl.h"
#include "protocol.h"
#include "gw_gray.h"
#include "task.h"
#include "bsp_gyro.h"

uint16_t count_10ms=0;
uint16_t count_100ms=0;
uint16_t t=0;

extern float Basic_Speed;									//电机目标速度
extern uint8_t OLED_View_Select;							//OLED选择界面变量
extern float Target_Distance;
extern float Target_Gyro;
extern float Target_Angle;
// 外部变量声明
extern uint8_t Turn_PID_Flag;
extern uint8_t Angle_PID_Flag;
extern pid_t pid_Turn;
extern pid_t pid_Angle;
extern Gyro_Struct *JY61P_Data; // 陀螺仪数据结构体
// ==============================================
volatile float  Target_ChaSu;		//目标差速
volatile float Motor1_Target_Speed;	//左边电机目标速度
volatile float Motor2_Target_Speed;	//右边电机目标速度

uint8_t huidu_state = 0;

extern volatile bool g_rx_frame_flag; 
volatile uint32_t control_cnt=0;

// 全局运行标志位（供主循环轮询使用）
volatile uint8_t flag_5ms_gyro_read = 0;
volatile uint8_t flag_1s_printf = 0;
volatile uint8_t flag_50ms_telemetry = 0;
volatile uint8_t flag_50ms_lcd = 0;
volatile uint16_t telemetry_pause_ms = 0;

// 云台控制目标速度变量
extern int16_t Gimbal_Speed_X;
extern int16_t Gimbal_Speed_Y;

float Task1_Time_Sec = 0.0f; // 任务一用时
float Task2_Time_Sec = 0.0f; // 任务二用时
float Task1_Dist = 0.0f; // 任务一当前路程
float Task2_Dist = 0.0f; // 任务二当前路程
/*-------------------------------------------------------------------------------------------*/
/*------------------------------定时器0的1ms中断服务函数------------------------------------*/
/*-------------------------------------------------------------------------------------------*/

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) 
    {
         case DL_TIMER_IIDX_ZERO: // TIMG 的加载归零中断标志是 IIDX_ZERO
         {
            extern volatile uint8_t ble_rx_idle_cnt;
            ble_rx_idle_cnt++;
            
            if(telemetry_pause_ms > 0) telemetry_pause_ms--;
            control_cnt++;

            if(control_cnt>=5)//5ms执行一次
            {
				flag_5ms_gyro_read = 1;
                control_cnt = 0;
            }

            // 10ms 周期任务处理
			if(++count_10ms >= 10) 
			{
				count_10ms = 0;
                
                static uint8_t cnt_50ms = 0;
                if(++cnt_50ms >= 5) {
                    cnt_50ms = 0;
                    flag_50ms_lcd = 1;
                    if(telemetry_pause_ms == 0) {
                        flag_50ms_telemetry = 1;
                    }
                }

                
                if(Task1_Time_flag == 1)
                {
                    Task1_Time_Sec += 0.01f;
                    Task1_Dist += (Motor1_Speed + Motor2_Speed) / 2.0f; // 累加路程
                }
                else if (Task2_Time_flag == 1) {
                    Task2_Time_Sec += 0.01f;
                    Task2_Dist += (Motor1_Speed + Motor2_Speed) / 2.0f; // 累加路程
                }

				// PD42S1 的云台控制保持独立运行，不受 Car_Mode 影响
				Car_Set_Speed_Unified(Gimbal_Speed_X, Gimbal_Speed_Y);
				
				Key_Read(); 
				Huidu_Read(); 
				
				// ==========================================================
                 // 核心控制逻辑：根据工作模式解耦运行策略（纯旋转、循迹及悬空保护）
                 // ==========================================================
                 // 跨模式状态保留变量集中声明
                 static float Soft_Basic_Speed = 0.0f; // 软启动基础速度
                 
                 float target_speed_1 = 0;
                 float target_speed_2 = 0;
                 float target_turn = 0.0f;
                // 状态及位移辅助变量声明
                 static float Forward_Displacement = 0.0f; // 累计的前后平移误差
                 static uint8_t last_mode = 0;             // 记录上一次的模式状态
                 // ==========================================================
                 // 根据当前工作模式执行相应的控制算法
                 // ==========================================================
                 if (Car_Mode == Angle_Mode)  // 进入独立旋转模式
                 {
                     Soft_Basic_Speed = 0.0f; 

                     if(JY61P_Data != NULL) 
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
                             if (target_turn > 0.0f && target_turn < min_power) {
                                 target_turn = min_power; 
                             } else if (target_turn < 0.0f && target_turn > -min_power) {
                                 target_turn = -min_power;
                             }
                         }
                     }
                     
                     // 3. 旋转最高限幅
                     if(target_turn > 15.0f)  target_turn = 15.0f;
                     if(target_turn < -15.0f) target_turn = -15.0f;


                     target_speed_1 =  target_turn; 
                     target_speed_2 = -target_turn; 
                 }
                 else // 其他模式 (如 Run_Mode / 循迹模式)
                 {
                     last_mode = Car_Mode; // 更新模式记录
                     Huidu_Proc(Huidu_Datas);
                     
                     // 丢线检测防抖计数器
                     static uint8_t lost_line_cnt = 0; 
                     
                     // 循迹模式下的悬空保护逻辑 (加入防抖滤除颠簸)
                     if(Huidu_Datas == 0xFFF) 
                     {
                         lost_line_cnt++;
                         // 连续多次（如 5 次）检测到全白状态，判定为彻底丢线或悬空
                         if(lost_line_cnt >= 5) 
                         {
                             Soft_Basic_Speed = 0.0f; 
                             target_speed_1 = 0;
                             target_speed_2 = 0;
                             target_turn = 0; // 清空转向
                         }
                         // 若仅为短暂丢线波动，则沿用上一周期的状态参量以执行惯性盲冲过度
                     }
                     else 
                     {
                         lost_line_cnt = 0; // 传感器捕捉到赛道信号，立即复位防抖计数

                         if(Turn_PID_Flag == 1) 
                         {
                            target_turn = PID_Calculate(&pid_Turn, Huidu_Error, 0); 
    
                            // 放大转向差速补偿的限幅
                            float max_turn = 120.0f; // 确保在大偏差纠偏瞬间具备充足的差速调整裕量，从而提高救车成功率
                            if(target_turn > max_turn)  target_turn = max_turn;
                            if(target_turn < -max_turn) target_turn = -max_turn;
                         }

                        target_speed_1 = Soft_Basic_Speed - target_turn; 
                        target_speed_2 = Soft_Basic_Speed + target_turn;
                     }
                 }
                extern uint8_t Test_Speed_Mode;
                extern float Target_Speed_Test;
                if (Test_Speed_Mode == 1) {
                    target_speed_1 = Target_Speed_Test;
                    target_speed_2 = Target_Speed_Test;
                }
                
                // 统一执行速度环 PID 并输出给直流电机
				MEASURE_MOTORS_SPEED(); 
				float pwm1 = PID_Calculate(&pid_Motor1_Speed, Motor1_Speed, target_speed_1);
				float pwm2 = PID_Calculate(&pid_Motor2_Speed, Motor2_Speed, target_speed_2);
				
			}
             
             if(++count_100ms>=100)//定时100ms
             {
				
                 count_100ms=0;
             }
             
            if(++t>=1000)//定时1s
            {
                t=0;
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

/*---------------------------------------------------------------------------------------*/
/*------------------------------定时器G0的3ms中断服务函数------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
void TIMER_1_INST_IRQHandler(void)//定时器中断服务函数
{
	switch (DL_TimerG_getPendingInterrupt(TIMER_1_INST)) 
	{
		case DL_TIMER_IIDX_ZERO:
		
		break;
		default:break;
	}
}
/*---------------------------------------------------------------------------------------*/
/*------------------------------定时器G7的10us中断服务函数------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
void TIMER_2_INST_IRQHandler(void) // 定时器 2 中断服务函数（注意触发频率不可过高，防止中断过载）
{
	switch (DL_TimerG_getPendingInterrupt(TIMER_2_INST)) 
	{
		case DL_TIMER_IIDX_LOAD:
	
		break;
		default:break;
	}
}

/*-------------------------------------------------------------------------------------------*/
/*-----------------------------------所有中断初始化------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
void NVIC_EnableIRQ_Init(void)
{
	//清除串口0，1中断标志
    NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);
	NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
	NVIC_ClearPendingIRQ(UART_MOTOR_INST_INT_IRQN);
	NVIC_ClearPendingIRQ(UART_MOTOR_2_INST_INT_IRQN);
    //使能串口0，1中断
    NVIC_EnableIRQ(UART_0_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_MOTOR_INST_INT_IRQN);
	NVIC_EnableIRQ(UART_MOTOR_2_INST_INT_IRQN);
	//清除定时器中断标志
    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);
	NVIC_ClearPendingIRQ(TIMER_1_INST_INT_IRQN);
	NVIC_ClearPendingIRQ(TIMER_2_INST_INT_IRQN);
    //使能定时器中断
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
	NVIC_EnableIRQ(TIMER_1_INST_INT_IRQN);
	NVIC_EnableIRQ(TIMER_2_INST_INT_IRQN);
	//定时器A开始计数
	DL_TimerG_startCounter(TIMER_0_INST);
	//编码器中断使能
	NVIC_EnableIRQ(Encoder_INT_IRQN);
	//陀螺仪中断使能（没有用到DMP库）
	//NVIC_EnableIRQ(MPU6050_INT_IRQN);
	
}
