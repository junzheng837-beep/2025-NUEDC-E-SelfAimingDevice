#ifndef __TASK_H__
#define __TASK_H__

#include "main.h"

void Task_1(void);
void Task_2(void);
// 声明外部可用的模式变量和调度器函数
extern uint8_t Task_Mode;
// 在 task.c 文件中添加以下全局变量
extern float Task1_Time_Sec; // 任务一用时
extern float Task2_Time_Sec; // 任务二用时
extern uint8_t Task1_Time_flag;
extern uint8_t Task2_Time_flag;
extern uint8_t Task1_run_flag;
extern uint8_t Task2_run_flag;
void Task_Scheduler(void);

extern uint8_t handle_ack(uint8_t motor_id, uint32_t over_time);
extern void clear_uart_rx_buffer(uint8_t motor_id);

extern uint8_t Task_1_State;  // 任务一状态机
extern uint8_t Task_2_State; 
extern float Start_Yaw;

extern uint8_t Task_Select;
extern uint8_t Task_1_State;
extern uint8_t Task_2_State;
extern uint8_t flag_state;
extern uint8_t flag_cnt;
extern uint8_t car_num;
extern uint8_t flag_warn;
extern uint8_t Target_Angle_flag;
#endif
