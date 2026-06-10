#include "timer.h"
#include "usart.h"
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
// ============ 新增以下 extern 变量 ============
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

// ====== 新增：全局运行标志位，供 main 函数轮询 ======
volatile uint8_t flag_5ms_gyro_read = 0;
volatile uint8_t flag_1s_printf = 0;
volatile uint8_t flag_50ms_telemetry = 0;
volatile uint8_t flag_50ms_lcd = 0;
volatile uint16_t telemetry_pause_ms = 0;

// 记得在 timer.c 上方用 extern 引入这两个变量
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

            // 在 middle/timer.c 中找到 10ms 处理部分 (大约第 93 行)
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
                 // 🌟 终极控制逻辑重构：严格隔离纯旋转、循迹与悬空保护
                 // ==========================================================
                 // 【修复】：将所有跨模式需要记忆的静态变量统一提升到这里声明！
                 static float Soft_Basic_Speed = 0.0f; // 软启动基础速度
                 
                 float target_speed_1 = 0;
                 float target_speed_2 = 0;
                 float target_turn = 0.0f;
                // 【绝招】：声明位置锚点变量
                 static float Forward_Displacement = 0.0f; // 累计的前后平移误差
                 static uint8_t last_mode = 0;             // 记录上一次的模式状态
                 // ==========================================================
                 // 🌟 模式隔离逻辑：根据 main.h 中定义的 Car_Mode 分流
                 // ==========================================================
                 if (Car_Mode == Angle_Mode)  // 进入独立旋转模式
                 {
                     Soft_Basic_Speed = 0.0f; 

                     if(JY61P_Data != NULL) 
                     {
                         target_turn = PID_Calculate(&pid_Angle, JY61P_Data->z, Target_Angle); 
                         float angle_err = Target_Angle - JY61P_Data->z;
                         
                         // 1. 到达死区 (±0.5度)，彻底停机断电
                         // 稍微放宽一点死区，防止低Kp时在零点几度反复拉扯
                         if (angle_err >= -0.5f && angle_err <= 0.5f) 
                         {
                             target_turn = 0.0f;
                             
                             pid_Angle.KiOut = 0; pid_Angle.PID_Out = 0;
                             pid_Motor1_Speed.KiOut = 0; pid_Motor1_Speed.PID_Out = 0;
                             pid_Motor2_Speed.KiOut = 0; pid_Motor2_Speed.PID_Out = 0;
                         }
                         else 
                         {
                             // 2. 摩擦力补偿 (低速托底)
                             // 这里将 min_power 从 3.5 降到了 2.0，避免末端起步太冲导致卡顿
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

                     // =========================================================
                     // 4. 【终极绝招：动态交叉耦合防平移 (Anti-Slip)】
                     // =========================================================
                     // 理论上原地旋转时，左轮速度(Motor1)和右轮速度(Motor2)必然是一正一负，相加绝对等于 0。
                     // 如果相加不等于 0，说明车体几何中心在这一瞬间发生了“往前溜”或“往后退”。
                     float slip_error = Motor1_Speed + Motor2_Speed; 

                     // 将瞬间滑移速度乘以 P系数，化作纠正力。
                     // (如果依然有平移，可将 0.3f 加大到 0.5f；如果抖动，减小到 0.1f)
                     float sync_comp = slip_error * 0.3f; 

                     // 严格限制纠正力的上限，绝对避免电机的抽搐和卡顿！
                     if(sync_comp > 3.0f) sync_comp = 3.0f;
                     if(sync_comp < -3.0f) sync_comp = -3.0f;

                     // 5. 下发控制：在完美对称的旋转差速上，叠加抵抗瞬间滑移的拉力！
                     target_speed_1 =  target_turn - sync_comp; 
                     target_speed_2 = -target_turn - sync_comp; 
                 }
                 else // 其他模式 (如 Run_Mode / 循迹模式)
                 {
                     last_mode = Car_Mode; // 更新模式记录
                     Huidu_Proc(Huidu_Datas);
                     
                     // 🌟 新增：防抖丢线计数器
                     static uint8_t lost_line_cnt = 0; 
                     
                     // 循迹模式下的悬空保护逻辑 (加入防抖滤除颠簸)
                     if(Huidu_Datas == 0xFFF) 
                     {
                         lost_line_cnt++;
                         // 连续5个周期(50ms)全是全白，才认为是真悬空或真丢线冲出赛道
                         if(lost_line_cnt >= 5) 
                         {
                             Soft_Basic_Speed = 0.0f; 
                             target_speed_1 = 0;
                             target_speed_2 = 0;
                             target_turn = 0; // 清空转向
                         }
                         // 注意：如果只是短暂的1-4个周期丢线，保留上一周期的 Soft_Basic_Speed 和 target_turn 盲冲！
                     }
                     else 
                     {
                         lost_line_cnt = 0; // 只要看到线，立马清零防抖计数

                         // ==================== 🌟 植入动态速度规划引擎 🌟 ====================
                         // 这里不再死板地追赶 Basic_Speed，而是根据偏离程度瞬间换挡
                         float High_Speed = 50.0f;  // 🚀 直道狂飙极速 (调稳后可继续往 60、70 加)
                         float Mid_Speed  = 35.0f;  // 🚙 缓弯微调速度 
                         float Low_Speed  = 20.0f;  // 🛑 急弯重刹速度 (必须低，给 PID 留出足够的差速抓地力)

                         // 获取当前灰度误差的绝对值
                         float abs_err = Huidu_Error > 0 ? Huidu_Error : -Huidu_Error;

                         if (abs_err == 0.0f) 
                         {
                             // 1. 完美居中：大直道，暴力提速！
                             // 提速步长给 3.0，让车在大直道瞬间把速度拉满
                             if(Soft_Basic_Speed < High_Speed) Soft_Basic_Speed += 3.0f; 
                         }
                         else if (abs_err <= 1.5f) // 对应 gw_gray.c 中的 0x04, 0x02 状态
                         {
                             // 2. 略微偏离：进缓弯或修正中，退回中速
                             // 刹车步长给 4.0，保证进弯前能迅速把速度降下来
                             if(Soft_Basic_Speed > Mid_Speed) Soft_Basic_Speed -= 4.0f; 
                             else if (Soft_Basic_Speed < Mid_Speed) Soft_Basic_Speed += 2.0f;
                         }
                         else 
                         {
                             // 3. 压到最边缘 (大误差)：死弯/急弯，一脚重刹！
                             // 瞬间把基础速度拍到底，把电机的所有力矩配额全交给差速 PID 强行甩尾过弯
                             Soft_Basic_Speed = Low_Speed; 
                         }
                         // ======================================================================

                         if(Turn_PID_Flag == 1) 
                         {
                            target_turn = PID_Calculate(&pid_Turn, Huidu_Error, 0); 
    
                            // 🌟 彻底解除封印：既然速度要提上去，救车的瞬间差速必须能吃满电机的极限！
                            float max_turn = 120.0f; // 原来是 60 或 90，现在直接放到 100~150！
                            if(target_turn > max_turn)  target_turn = max_turn;
                            if(target_turn < -max_turn) target_turn = -max_turn;
                         }

                         extern uint8_t Task_Mode; 
                         float feedforward_turn = 0.0f;
                         
                         if(Task_Mode == 1) 
                        {
                            // 【修改点】：后驱车的前馈系数需要减小，甚至设为 0 测试
                            // 如果车头向外甩出赛道，尝试减小或注释掉前馈
                            feedforward_turn = Soft_Basic_Speed * 0.03f; // 原 0.10f 降为 0.03f
                        }
                        // 目标速度融合：为了防止转弯时车速过快冲出去，可以在大转角时主动降低基础速度 (降速过弯)
                        float current_speed = Soft_Basic_Speed;
                        if (fabs(target_turn) > 20.0f) // 当转向力度很大时
                        {
                            current_speed = Soft_Basic_Speed * 0.7f; // 基础速度打7折，防止甩尾飞出
                        }
                        target_speed_1 = current_speed - target_turn - feedforward_turn; 
                        target_speed_2 = current_speed + target_turn + feedforward_turn;
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
				// ==========================================================
                 // 🌟 终极约束：强制动力绝对对称（防平移核心）
                 // ==========================================================
                 if (Car_Mode == Angle_Mode && Test_Speed_Mode == 0)
                 {
                     if (target_speed_1 == 0.0f && target_speed_2 == 0.0f)
                     {
                         // 【绝招 1】：停车死区时，彻底切断底层 PWM！
                         // 避免两轮编码器读数微弱差异导致算出“长短脚”差异刹车，从而引发最后那一下位移
                         pwm1 = 0;
                         pwm2 = 0;
                         
                         // 清除速度环残留积分
                         pid_Motor1_Speed.KiOut = 0; pid_Motor1_Speed.PID_Out = 0;
                         pid_Motor2_Speed.KiOut = 0; pid_Motor2_Speed.PID_Out = 0;
                     }
                     else
                     {
                         // 【绝招 2】：电子差速锁算法
                         // 物理电机存在响应差异，独立计算出的 pwm1 和 pwm2 大小往往不同
                         // 只要输出大小不等，底盘就会产生平移推力。
                         // 解决办法：提取两者绝对值求平均，强行赋予一正一负，保证底盘受力绝对对称！
                         float abs_pwm1 = (pwm1 > 0) ? pwm1 : -pwm1;
                         float abs_pwm2 = (pwm2 > 0) ? pwm2 : -pwm2;
                         float avg_pwm = (abs_pwm1 + abs_pwm2) / 2.0f;

                         // 根据目标速度的方向，重新分配绝对相等的 PWM
                         pwm1 = (target_speed_1 > 0) ? avg_pwm : -avg_pwm;
                         pwm2 = (target_speed_2 > 0) ? avg_pwm : -avg_pwm;
                     }
                 }
				SET_MOTORS_SPEED((int)pwm1, (int)pwm2);
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

volatile uint16_t cnt=0;
uint32_t warn_cnt=0;
uint32_t angle_cnt=0;

/*---------------------------------------------------------------------------------------*/
/*------------------------------定时器G0的3ms中断服务函数------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
void TIMER_1_INST_IRQHandler(void)//定时器中断服务函数
{
	switch (DL_TimerG_getPendingInterrupt(TIMER_1_INST)) 
	{
		case DL_TIMER_IIDX_ZERO:
		
		// angle_cnt++;
		// if(angle_cnt == 1000)
		// {
		// 	angle_cnt = 0;
		// 	if(Target_Angle_flag == 1)
		// 	{
		// 		Target_Angle ++;
		// 		Target_Angle_flag = 0;
		// 	}
		// 	else if(Target_Angle_flag == 2)
		// 	{
		// 		Target_Angle --;
		// 		Target_Angle_flag = 0;
		// 	}
		// }
		// if(flag_warn == 1)
		// {
		// 	warn_cnt ++;
		// 	if(warn_cnt >= 1000)//1.0s
		// 	{
		// 		warn_cnt = 0;
		// 		flag_warn = 0;
		// 	}
		// }
		break;
		default:break;
	}
}
/*---------------------------------------------------------------------------------------*/
/*------------------------------定时器G7的10us中断服务函数------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
uint32_t tim2_cnt = 0;
uint32_t flagtim_cnt = 0;
void TIMER_2_INST_IRQHandler(void)//定时器中断服务函数//不要执行太快，会卡在中断的
{
	switch (DL_TimerG_getPendingInterrupt(TIMER_2_INST)) 
	{
		case DL_TIMER_IIDX_LOAD:
		tim2_cnt++;

		
		// if(flag_state == 1)
		// {
		// 	flagtim_cnt++;
		// 	if(flagtim_cnt >= 230000)//2300ms
		// 	{
		// 		huidu_lasterror = 0;
		// 		Huidu_Error = 0;
		// 		flag_cnt ++;
		// 		flagtim_cnt = 0;
		// 		flag_state = 0;
		// 	}
		// }

		// if(tim2_cnt >= 800)//8ms
		// {
		// 	tim2_cnt = 0;
		// 	if(Angle_PID_Flag != 1)//未开角度环时读灰度
		// 	{
		// 		Huidu_Read();
		// 	}
		// 	switch(Task_Select)
		// 	{
		// 		case 1:
		// 		{
		// 			Task_1();
		// 		}
		// 		break;
		// 	}
		// }
		
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
