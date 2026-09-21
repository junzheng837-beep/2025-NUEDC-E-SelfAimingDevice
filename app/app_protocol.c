#include "app_protocol.h"
#include "stdint.h"
#include "motor_ctrl.h"
#include "protocol.h"
#include "task.h"
#include "smd.h"
#include "usart.h"
#include "Encoder.h"
#include "gw_gray.h"

// K230 视觉模块下发的目标速度缓存
int16_t Gimbal_Speed_X = 0;
int16_t Gimbal_Speed_Y = 0;
// 云台控制模式标志 (0: 位置模式/归位, 1: 视觉速度模式)
uint8_t K230_Ctrl_Mode = 0;
extern uint8_t g_smd_target_uart; // 数据发送路由标志

// K230 超时保护：200ms 无数据则停机
extern volatile uint32_t ms_ticks;
volatile uint32_t K230_Rx_Timestamp = 0;
#define K230_TIMEOUT_MS 200
/**
  * @brief  K230 视觉数据帧解析状态机
  * @param  byte: 接收到的单字节数据
  */
void K230_Parse_Data(uint8_t byte)
{
    static uint8_t state = 0;
    static uint8_t cmd_type = 0; // 0: 速度模式(BB), 1: 绝对位置模式(DD)
    static uint8_t buf[8];       // 接收缓存
    static uint8_t data_idx = 0;

    switch(state) {
        case 0: if(byte == 0xAA) state = 1; else state = 0; break; 
        case 1: 
            if(byte == 0xBB) { cmd_type = 0; state = 2; data_idx = 0; } 
            else if(byte == 0xDD) { cmd_type = 1; state = 2; data_idx = 0; }
            else state = 0; 
            break; 
        case 2: 
            buf[data_idx++] = byte; 
            if((cmd_type == 0 && data_idx == 4) || (cmd_type == 1 && data_idx == 8)) state = 3;
            break;
        case 3:
            if(byte == 0xCC) { // 校验帧尾
                K230_Rx_Timestamp = ms_ticks; // 更新接收时间戳
                if (cmd_type == 0) {
                    K230_Ctrl_Mode = 1; // 收到 BB 帧，切换为视觉速度模式
                    Gimbal_Speed_X = (int16_t)(buf[0] | (buf[1] << 8));
                    Gimbal_Speed_Y = (int16_t)(buf[2] | (buf[3] << 8));
                } else if (cmd_type == 1) {
                    K230_Ctrl_Mode = 0; // 收到 DD 帧，切换为绝对位置模式
                    
                    int32_t pos_x = (int32_t)(buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24));
                    int32_t pos_y = (int32_t)(buf[4] | (buf[5]<<8) | (buf[6]<<16) | (buf[7]<<24));
                    
                    static int32_t last_pos_x = -999999;
                    static int32_t last_pos_y = -999999;
                    
                    if (pos_x != last_pos_x) {
                        uint8_t dir_x = (pos_x >= 0) ? 0 : 1; 
                        uint32_t pulse_x = (pos_x >= 0) ? pos_x : -pos_x;
                        g_smd_target_uart = 1; 
                        smd_pos_mode(1, dir_x, 5, 200, pulse_x); // 设置归位速度 (RPM)
                        last_pos_x = pos_x;
                    }
                    
                    if (pos_y != last_pos_y) {
                        uint8_t dir_y = (pos_y >= 0) ? 0 : 1;
                        uint32_t pulse_y = (pos_y >= 0) ? pos_y : -pos_y;
                        g_smd_target_uart = 2; 
                        smd_pos_mode(1, dir_y, 5, 200, pulse_y); 
                        last_pos_y = pos_y;
                    }
                }
            }
            state = 0; 
            break;
        default: state = 0; break;
    }
}
/*-------------------------------------------------------------------------------------------*/
/*--------------------------------步进电机实时速度控制----------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
static int16_t last_sent_x = 0;
static int16_t last_sent_y = 0;
static uint32_t last_send_time = 0;
#define SEND_INTERVAL_MS  20   // 最短发送间隔 20ms（50Hz）
#define SPEED_DEADBAND    2    // 速度变化死区，小于该值不更新

void K230_Speed_Control(void)
{
    // 超时保护：超过 200ms 未收到 K230 数据则停机
    if ((ms_ticks - K230_Rx_Timestamp) > K230_TIMEOUT_MS) {
        if (K230_Ctrl_Mode != 0) {
            K230_Ctrl_Mode = 0;
            g_smd_target_uart = 1;
            smd_speed_mode(1, 0, 0, 0);
            g_smd_target_uart = 2;
            smd_speed_mode(1, 0, 0, 0);
            last_sent_x = 0;
            last_sent_y = 0;
        }
        return;
    }

    if (K230_Ctrl_Mode != 1) return;

    // 发送频率限制：20ms 内不重复发送
    if ((ms_ticks - last_send_time) < SEND_INTERVAL_MS) return;
    last_send_time = ms_ticks;

    // 死区过滤：速度变化太小就不发，避免微小抖动引发持续震荡
    int16_t dx = Gimbal_Speed_X - last_sent_x;
    int16_t dy = Gimbal_Speed_Y - last_sent_y;
    if (dx < 0) dx = -dx;
    if (dy < 0) dy = -dy;

    if (dx < SPEED_DEADBAND && dy < SPEED_DEADBAND) return;

    last_sent_x = Gimbal_Speed_X;
    last_sent_y = Gimbal_Speed_Y;

    // X 轴（电机1）
    uint8_t dir_x = (Gimbal_Speed_X >= 0) ? 0 : 1;
    float speed_x = (Gimbal_Speed_X >= 0) ? (float)Gimbal_Speed_X : (float)(-Gimbal_Speed_X);
    g_smd_target_uart = 1;
    smd_speed_mode(1, dir_x, 20, speed_x);  // acc=20 RPM/S 平滑过渡

    // Y 轴（电机2）
    uint8_t dir_y = (Gimbal_Speed_Y >= 0) ? 0 : 1;
    float speed_y = (Gimbal_Speed_Y >= 0) ? (float)Gimbal_Speed_Y : (float)(-Gimbal_Speed_Y);
    g_smd_target_uart = 2;
    smd_speed_mode(1, dir_y, 20, speed_y);  // acc=20 RPM/S 平滑过渡
}
/*-------------------------------------------------------------------------------------------*/
/*--------------------------------上位机协议数据处理-----------------------------------------*/
/*-------------------------------------------------------------------------------------------*/

void Protocol_Datas_Proc(void)
{
	if(Car_Mode != Run_Mode)
	{		
		int PROTOCOL_TEMP=0;//野火上位机上传临时数据
		if(Car_Mode==Speed_Mode)//速度环模式
		{
			Turn_PID_Flag = 0;		//关闭循迹
			Angle_PID_Flag = 0;		//关闭角度环
			Gyro_PID_Flag = 0;		//关闭角速度环
			Distance_PID_Flag = 0;	//关闭距离环
			
			// 上传数据通道选择
			if(Set_Motor_Param_Select==1) PROTOCOL_TEMP = Motor1_Speed;
			else if(Set_Motor_Param_Select==2) PROTOCOL_TEMP = Motor2_Speed;
		}
		else if(Car_Mode == Turn_Mode)//转向环模式
		{
			Turn_PID_Flag = 1;		
			Distance_PID_Flag = 0;
			Gyro_PID_Flag = 0;		
			Angle_PID_Flag = 0;	
			
			PROTOCOL_TEMP = Huidu_Error;
		}
		else if(Car_Mode == Distance_Mode)//距离环模式
		{
			Turn_PID_Flag = 0;	
			Distance_PID_Flag = 1;		
			Gyro_PID_Flag = 0;		
			Angle_PID_Flag = 0;		
			
			PROTOCOL_TEMP = Measure_Distance;
		}   
		else if(Car_Mode == Gyro_Mode)//角速度环模式
		{
			Turn_PID_Flag = 0;		
			Distance_PID_Flag = 0;	
			Gyro_PID_Flag = 1;
			Angle_PID_Flag = 1;	
			
			// PROTOCOL_TEMP = Gyro_Z_Measeure;
		}
		else if(Car_Mode == Angle_Mode)//角度环模式
		{
			Turn_PID_Flag = 0;		
			Distance_PID_Flag = 0;	
			Gyro_PID_Flag = 0;
			Angle_PID_Flag = 1;
			
			// PROTOCOL_TEMP = JY61P_Data->total_yaw;
		}
		set_computer_value(SEND_FACT_CMD, CURVES_CH1, &PROTOCOL_TEMP, 4);//向上位机发送数据	
		receiving_process();// 执行接收数据解析
	}
	else// 运行模式 (暂停数据上传以降低系统负载)
	{

	}
}