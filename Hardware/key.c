/***
    *******************************************************************************************************************************************************************
    * @file    key.c
    * @version V2.1
    * @date    2024-7-22
    * @author  御龙   
    * @brief   MSPM0G3507小车PID调试通用模板
   *************************************************************************************************
   * @description
    * * 接口配置可以使用Sysconfig�?
   *
>>>>> 其他说明：未经允许不可擅自转发、售卖本套代码，大家都是我国的有为青年，请保持好自己的初心，在此，向你表达我的感�?
    *************************************************************************************************************************************************************
***/

#include "key.h"
#include "task.h"
#include "timer.h"
#include "gw_gray.h"
// #include "oled.h"
#include "motor_ctrl.h"
#include "smd.h"
#include "process_frame.h"
//结构体声�?
KEY Key[KEY_Number];
/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------外部变量声明--------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
/* �?key.c 顶部引入外部变量 */
extern float Target_Angle;
extern float Basic_Speed;
extern pid_t pid_Turn;          // 引入转向/循迹PID结构�?
extern uint8_t Task_1_State;    // 引入任务1状态机
extern uint8_t Task_2_State;    // 引入任务2状态机
extern uint8_t flag_state;      // 引入任务防误�?定点状态标志位
/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------步进电机应答--------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------*/
/*-----------------------按键读取函数（需要放�?0ms中断内进行扫描）--------------------------*/
/*-------------------------------------------------------------------------------------------*/
void Key_Read(void)
{
    //添加读取按键
    Key[0].Down_State=KEY1; //默认摁下�? 
    Key[1].Down_State=KEY2;
    Key[2].Down_State=KEY3;
    
    for(int i=0;i<KEY_Number;i++){
        switch(Key[i].Judge_State){
            case 0:{if(Key[i].Down_State==0){Key[i].Judge_State=1;Key[i].Down_Time=0;}else{Key[i].Judge_State=0;}}break;
            case 1:{if(Key[i].Down_State==0){Key[i].Judge_State=2;}else{Key[i].Judge_State=0;}}break;
            case 2:{if((Key[i].Down_State==1)&&(Key[i].Down_Time<=70)){if(Key[i].Double_Time_EN==0){Key[i].Double_Time_EN=1;Key[i].Double_Time=0;}else{Key[i].Double_Flag=1;Key[i].Double_Time_EN=0;}Key[i].Judge_State=0;}else if((Key[i].Down_State==1)&&(Key[i].Down_Time>70)){Key[i].Long_Flag=1;Key[i].Judge_State=0;}else{Key[i].Down_Time++;}}break;
        }
        if(Key[i].Double_Time_EN==1){Key[i].Double_Time++;if(Key[i].Double_Time>=35){Key[i].Short_Flag=1;Key[i].Double_Time_EN=0;}}
    }
}

/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------按键处理函数------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
static uint8_t is_motor_running = 0;//PD

void KEY_PROC(void)
{
    static uint8_t first_init = 0;
    if (first_init == 0) 
    {
        // 强制将按键电平的初始内存值设�?1（松手状态）
        Key[0].Down_State = 1; 
        Key[1].Down_State = 1;
        Key[2].Down_State = 1;
        first_init = 1;
        return; // 直接退出第一次错乱的循环，等待定时器读取真实的引脚状�?
    }
    /* ============================================== */
    /* =================== 长按�?=================== */
    /* ============================================== */
    // Key[0].Down_State == 0 表示 KEY1 正在被死死按�?
    if (Key[0].Down_State == 0) 
    {
        // 确保只有在“刚按下”的那一瞬间发送一次启动指�?
        if (is_motor_running == 0) 
        {
            
            is_motor_running = 1;
            
        }
    }
    else if (Key[1].Down_State == 0) 
    {
        // 确保只有在“刚按下”的那一瞬间发送一次启动指�?
        if (is_motor_running == 0) 
        {
            
            is_motor_running = 1;
            
        }
    }
    // Key[0].Down_State != 0 表示 KEY1 已被松开
    else 
    {
        // 确保只有在“刚松开”的那一瞬间发送一次停止指�?
        if (is_motor_running == 1) 
        {
            
        }
    }

    /* ================= 引入脱机调参系统变量 ================= */
    extern uint8_t Tuning_Mode;
    extern uint8_t Tuning_Cursor;
    extern uint8_t OLED_View_Select;
    extern pid_t pid_Turn;
    /* ======================================================= */


    /* ----- 短摁处理 ----- */
    if(Key[0].Short_Flag==1) // KEY1短按
    {
        if(OLED_View_Select == 2 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) { pid_Turn.Kp -= 0.01f; if(pid_Turn.Kp < 0) pid_Turn.Kp = 0; }
            else if(Tuning_Cursor == 1) { pid_Turn.Ki -= 0.01f; if(pid_Turn.Ki < 0) pid_Turn.Ki = 0; }
            else if(Tuning_Cursor == 2) { pid_Turn.Kd -= 0.01f; if(pid_Turn.Kd < 0) pid_Turn.Kd = 0; }
        } else if(OLED_View_Select == 1 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) { pid_Motor1_Speed.Kp -= 0.01f; if(pid_Motor1_Speed.Kp < 0) pid_Motor1_Speed.Kp = 0; }
            else if(Tuning_Cursor == 1) { pid_Motor1_Speed.Ki -= 0.01f; if(pid_Motor1_Speed.Ki < 0) pid_Motor1_Speed.Ki = 0; }
            else if(Tuning_Cursor == 2) { pid_Motor1_Speed.Kd -= 0.01f; if(pid_Motor1_Speed.Kd < 0) pid_Motor1_Speed.Kd = 0; }
        } else {
            Task_Mode = 1;
            Task_1_State = 1;
            count_10ms = 0;
            // LED3_TOGGLE();
        }
        Key[0].Short_Flag=0;

    }
    else if(Key[1].Short_Flag==1) // KEY2短按
    {
        if(OLED_View_Select == 2 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) pid_Turn.Kp += 0.01f;
            else if(Tuning_Cursor == 1) pid_Turn.Ki += 0.01f;
            else if(Tuning_Cursor == 2) pid_Turn.Kd += 0.01f;
        } else if(OLED_View_Select == 1 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) pid_Motor1_Speed.Kp += 0.01f;
            else if(Tuning_Cursor == 1) pid_Motor1_Speed.Ki += 0.01f;
            else if(Tuning_Cursor == 2) pid_Motor1_Speed.Kd += 0.01f;
        } else {
            Task_Mode = 2;
            Task_2_State = 1;
            count_10ms = 0;
            // LED3_TOGGLE();
        }
        Key[1].Short_Flag=0;
        

    }
    else if(Key[2].Short_Flag==1) // KEY3短按：切�?�?切光�?
    {
        if((OLED_View_Select == 2 || OLED_View_Select == 1) && Tuning_Mode == 1) {
            Tuning_Cursor++;
            if(Tuning_Cursor > 2) Tuning_Cursor = 0;
        } else {
            OLED_View_Select++;
            if(OLED_View_Select > 4) OLED_View_Select = 1; // 仅在�?页数据看板之间来回切�?
        }
        Key[2].Short_Flag=0;
    }
    
    /* ----- 双击处理 ----- */
    if(Key[0].Double_Flag==1)
    {
        Key[0].Double_Flag=0;
		// LED3_TOGGLE();
    }
    else if(Key[1].Double_Flag==1) // 增加圈数
    {
        Key[1].Double_Flag=0;
		// LED3_TOGGLE();
        // car_num ++;
        // car_num %= 6;//最多设�?�?
    }
    else if(Key[2].Double_Flag==1)
    {
        Key[2].Double_Flag=0;
		// LED3_TOGGLE();
    }
    
    /* ----- 长摁处理 ----- */
    if(Key[0].Long_Flag==1)
    {
        if(OLED_View_Select == 2 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) { pid_Turn.Kp -= 0.1f; if(pid_Turn.Kp < 0) pid_Turn.Kp = 0; }
            else if(Tuning_Cursor == 1) { pid_Turn.Ki -= 0.1f; if(pid_Turn.Ki < 0) pid_Turn.Ki = 0; }
            else if(Tuning_Cursor == 2) { pid_Turn.Kd -= 0.1f; if(pid_Turn.Kd < 0) pid_Turn.Kd = 0; }
        } else if(OLED_View_Select == 1 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) { pid_Motor1_Speed.Kp -= 0.1f; if(pid_Motor1_Speed.Kp < 0) pid_Motor1_Speed.Kp = 0; }
            else if(Tuning_Cursor == 1) { pid_Motor1_Speed.Ki -= 0.1f; if(pid_Motor1_Speed.Ki < 0) pid_Motor1_Speed.Ki = 0; }
            else if(Tuning_Cursor == 2) { pid_Motor1_Speed.Kd -= 0.1f; if(pid_Motor1_Speed.Kd < 0) pid_Motor1_Speed.Kd = 0; }
        } else {
            extern uint8_t Test_Speed_Mode;
            Test_Speed_Mode = !Test_Speed_Mode; // 长按切换纯速度测试模式
        }
        Key[0].Long_Flag=0;
    }
    else if(Key[1].Long_Flag==1)
    {
        if(OLED_View_Select == 2 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) pid_Turn.Kp += 0.1f;
            else if(Tuning_Cursor == 1) pid_Turn.Ki += 0.1f;
            else if(Tuning_Cursor == 2) pid_Turn.Kd += 0.1f;
        } else if(OLED_View_Select == 1 && Tuning_Mode == 1) {
            if(Tuning_Cursor == 0) pid_Motor1_Speed.Kp += 0.1f;
            else if(Tuning_Cursor == 1) pid_Motor1_Speed.Ki += 0.1f;
            else if(Tuning_Cursor == 2) pid_Motor1_Speed.Kd += 0.1f;
        }
        Key[1].Long_Flag=0;
        
    }
    else if(Key[2].Long_Flag==1)
    {
        if(OLED_View_Select == 2 || OLED_View_Select == 1) {
            Tuning_Mode = !Tuning_Mode; // 开�?关闭调参模式
        }
        Key[2].Long_Flag=0;
        
    }
}
