/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------头文件声明--------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
#include "main.h"
#include "ti_msp_dl_config.h"
#include "key.h"
#include "delay.h"
// #include "oled.h"
#include "bmp.h"
#include "usart.h"
#include "Encoder.h"
#include "motor_ctrl.h"
#include "protocol.h"
#include "gw_gray.h"
#include "task.h"
#include "timer.h"
#include "app_lcd.h"
#include "hw_lcd.h"
#include "app_protocol.h"
#include "bsp_gyro.h"
#include "bsp_hc05.h"

/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------自定义变量--------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
uint8_t Car_Mode = 0; // 假设 0 为 Run_Mode
float Basic_Speed = 10;
uint8_t OLED_View_Select = 1;
float Target_Distance = 0;
float Target_Gyro = 0;
float Target_Angle = 0;
Gyro_Struct *JY61P_Data;

extern volatile uint8_t flag_5ms_gyro_read;
extern uint8_t flag_1s_printf;

void LCD_Init(void);
void PD42S1_Init(void);
void JY61P_Dis(void);
void JY61P_Proc(void);

/*-------------------------------------------------------------------------------------------*/
/*---------------------------------------主函数----------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
// TODO : 测试
int main(void)
{
    SYSCFG_DL_init(); // TI底层外设初始化
    NVIC_EnableIRQ_Init();// 初始化各种系统中断

    jy61pInit();//陀螺仪初始化
    LCD_Init();//屏幕初始化（实际使用TFT屏幕）
    Bluetooth_Init(); // 蓝牙初始化
    PD42S1_Init();// 电机相关的初始化 (如果你的smd.c里有初始化函数的话)
    // protocol_init(); // 初始化野火协议的环形缓冲区
    while (1)
    {
        
        if (flag_5ms_gyro_read)
        {
            flag_5ms_gyro_read = 0;
            JY61P_Data = get_angle();
        }

        KEY_PROC();
        LCD_Show_Proc();
        // 🌟 极简之美：主循环只管调用大总管，具体的苦活累活全都丢给 task.c 里的调度器
        Task_Scheduler();
        
        // 蓝牙数据接收处理
        Receive_Bluetooth_Data();
        
        extern volatile uint8_t flag_50ms_telemetry;
        if (flag_50ms_telemetry)
        {
            flag_50ms_telemetry = 0;
            extern float Motor1_Speed;
            extern float Motor2_Speed;
            extern float Debug_Yaw_Diff;
            extern uint8_t Test_Speed_Mode;
            extern float Target_Speed_Test;
            extern volatile uint16_t telemetry_pause_ms;
            extern volatile uint8_t telemetry_enabled;
            
            if (telemetry_pause_ms == 0 && telemetry_enabled == 1) {
                float yaw_to_send = Debug_Yaw_Diff;
                if (Test_Speed_Mode == 1) {
                    yaw_to_send = Target_Speed_Test;
                }
                
                Mobile_sendData_UART1(Motor1_Speed, Motor2_Speed, yaw_to_send);
            }
        }
    }
}

void JY61P_Dis(void)
{
    // 每 5ms 读取一次陀螺仪
    if (flag_5ms_gyro_read)
    {
        flag_5ms_gyro_read = 0;   // 清除标记
        JY61P_Data = get_angle(); // 不在中断里卡死
    }

    // 每 1s 打印一次数据
    if (flag_1s_printf)
    {
        flag_1s_printf = 0; // 清除标记
        // if(JY61P_Data != NULL) {
        //     lc_printf("JY61P RollX  = [ %d ]\r\n", (int)JY61P_Data->x);
        //     lc_printf("JY61P PitchY = [ %d ]\r\n", (int)JY61P_Data->y);
        //     lc_printf("JY61P YawZ   = [ %d ]\r\n", (int)JY61P_Data->z);
        // }
        // 打印两个轮子的实时速度
        uart1_printf("Speed1: %.2f | Speed2: %.2f \r\n", Motor1_Speed, Motor2_Speed);
    }
}

void LCD_Init(void)
{
    lcd_init();
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK); // 清为黑屏
    LCD_BLK_Set();                       // 打开背光 (如果硬件上需要拉高)
}

void PD42S1_Init(void)
{
    /* ==================================================================== */
    /* ================== 电机内部参数固化配置 ============================= */
    /* ==================================================================== */
    delay_ms(200); // 确保驱动器已经通电准备好

    // 【配置电机 1】
    g_smd_target_uart = 1; // 路由切到串口1
    clear_uart_rx_buffer(1);
    smd_origin_aoto_zero(1, 0); // 发送：关闭上电自动回零 (参数0)
    handle_ack(1, 50);

    clear_uart_rx_buffer(1);
    smd_param_save(1); // 发送：将设置保存到驱动器内部Flash
    handle_ack(1, 50);

    // 【配置电机 2】
    g_smd_target_uart = 2; // 路由切到串口2
    clear_uart_rx_buffer(2);
    smd_origin_aoto_zero(1, 0); // 发送：关闭上电自动回零 (参数0)
    handle_ack(2, 50);

    clear_uart_rx_buffer(2);
    smd_param_save(1); // 发送：将设置保存到驱动器内部Flash
    handle_ack(2, 50);
    /* ==================================================================== */
}
