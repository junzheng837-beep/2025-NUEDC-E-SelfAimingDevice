#include "app_protocol.h"
#include "stdint.h"

// 这两个全局变量，将实时保存 K230 算出的目标速度
int16_t Gimbal_Speed_X = 0; 
int16_t Gimbal_Speed_Y = 0;
// 🌟 新增：云台控制模式标志 (0=位置归位模式/上电默认， 1=视觉速度模式)
uint8_t K230_Ctrl_Mode = 0;
extern uint8_t g_smd_target_uart; // 路由标志
/**
  * @brief  K230 视觉数据帧解析状态机 (极简高效，放在中断里完全不占 CPU)
  * @param  byte: 串口每接收到一个字节，就传入这个函数一次(支持速度与位置指令)
  */
void K230_Parse_Data(uint8_t byte)
{
    static uint8_t state = 0;
    static uint8_t cmd_type = 0; // 0: 速度模式(BB), 1: 绝对位置模式(DD)
    static uint8_t buf[8];       // 缓存扩大到8字节
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
            if(byte == 0xCC) { // 匹配帧尾
                if (cmd_type == 0) {
                    K230_Ctrl_Mode = 1; // 🌟 收到 BB，立刻切为视觉速度模式
                    Gimbal_Speed_X = (int16_t)(buf[0] | (buf[1] << 8));
                    Gimbal_Speed_Y = (int16_t)(buf[2] | (buf[3] << 8));
                } else if (cmd_type == 1) {
                    K230_Ctrl_Mode = 0; // 🌟 收到 DD，进入位置模式
                    
                    int32_t pos_x = (int32_t)(buf[0] | (buf[1]<<8) | (buf[2]<<16) | (buf[3]<<24));
                    int32_t pos_y = (int32_t)(buf[4] | (buf[5]<<8) | (buf[6]<<16) | (buf[7]<<24));
                    
                    uint8_t dir_x = (pos_x >= 0) ? 0 : 1; 
                    uint8_t dir_y = (pos_y >= 0) ? 0 : 1;
                    uint32_t pulse_x = (pos_x >= 0) ? pos_x : -pos_x;
                    uint32_t pulse_y = (pos_y >= 0) ? pos_y : -pos_y;
                    
                    g_smd_target_uart = 1; 
                    smd_pos_mode(1, dir_x, 5, 200, pulse_x); // 200是归位速度RPM，可按需改
                    
                    g_smd_target_uart = 2; 
                    smd_pos_mode(1, dir_y, 5, 200, pulse_y); 
                }
            }
            state = 0; 
            break;
        default: state = 0; break;
    }
}
/*-------------------------------------------------------------------------------------------*/
/*-------------------------------野火上位机助手数据处理--------------------------------------*/
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
			
			//上传数据选择
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
		receiving_process();//对接收数据进行处理
	}
	else//处于Run_Mode下的处理	（不需要上传数据）
	{
		//处于Run_Mode下的程序逻辑运行
		
		// switch(Task_Select)
		// {
		// 	case 1:
		// 	{
		// 		Task_1();
		// 	}
		// 	break;
//			case 2:
//			{
//				Task_2();
//			}
//			break;
		// }
	}
}


