/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------头文件声明--------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
#include "Encoder.h"
#include "app_lcd.h"
#include "app_protocol.h"
#include "bmp.h"
#include "bsp_gyro.h"
#include "bsp_hc05.h"
#include "bsp_sr04.h"
#include "delay.h"
#include "gw_gray.h"
#include "hw_lcd.h"
#include "key.h"
#include "main.h"
#include "motor_ctrl.h"
#include "protocol.h"
#include "task.h"
#include "ti_msp_dl_config.h"
#include "timer.h"
#include "usart.h"

/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------自定义变量--------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
uint8_t Car_Mode = 0; // 假设 0 为 Run_Mode
float Basic_Speed = 10;
uint8_t OLED_View_Select = 2;
float Target_Distance = 0;
float Target_Gyro = 0;
float Target_Angle = 0;
Gyro_Struct *JY61P_Data;

extern volatile uint8_t flag_5ms_gyro_read;
extern uint8_t flag_1s_printf;

void LCD_Init(void);
void PD42S1_Init(void);

/*-------------------------------------------------------------------------------------------*/
/*---------------------------------------主函数----------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/

int main(void) {
  SYSCFG_DL_init();      // TI底层外设初始化
  NVIC_EnableIRQ_Init(); // 初始化各种系统中断

  jy61pInit();      // 陀螺仪初始化
  LCD_Init();       // 屏幕初始化（实际使用TFT屏幕）
  Bluetooth_Init(); // 蓝牙初始化
  PD42S1_Init();    // 电机相关的初始化 (如果你的smd.c里有初始化函数的话)
  SR04_Init();      // 超声波初始化

  while (1) {

    if (flag_5ms_gyro_read) {
      flag_5ms_gyro_read = 0;
      JY61P_Data = get_angle();

      static uint8_t sr04_div = 0;
      if (++sr04_div >= 4) { // 20ms trigger (极限提速 50Hz)
        sr04_div = 0;
        SR04_Trigger();
      }
    }

    if (flag_1s_printf) {
      flag_1s_printf = 0;
      extern volatile float Motor1_Speed;
      extern volatile float Motor2_Speed;
      extern volatile long long Motor1_Total_Pulse;
      extern volatile long long Motor2_Total_Pulse;
      char msg[128];
      sprintf(msg, "SPD M1:%.1f M2:%.1f | PULSE M1:%lld M2:%lld\r\n",
              Motor1_Speed, Motor2_Speed, Motor1_Total_Pulse,
              Motor2_Total_Pulse);
      BLE_send_String((unsigned char *)msg);
    }

    KEY_PROC();
    LCD_Show_Proc();
    // 主循环仅调用任务调度器，具体的任务分发由 task.c 处理
    Task_Scheduler();

    // 蓝牙数据接收处理 (依然保留蓝牙接收以防需要发指令，并配合上方的
    // BLE_send_String 实时上报状态)
    Receive_Bluetooth_Data();
  }
}

void LCD_Init(void) {
  lcd_init();
  LCD_Fill(0, 0, LCD_W, LCD_H, BLACK); // 清为黑屏
  LCD_BLK_Set();                       // 打开背光 (如果硬件上需要拉高)
}

void PD42S1_Init(void) {
  /* ==================================================================== */
  /* ================== 电机内部参数固化配置 ============================= */
  /* ==================================================================== */
  // 【关键修改：加长开机延时】
  // V1.8固件包含USB和OLED初始化，开机速度极慢！
  // 如果只有200ms，单片机发出的“切换模式”指令会在电机还没开机完成时丢失！
  delay_ms(2000); // 必须等待2秒，确保电机屏幕点亮且完全启动完毕

  // 【配置电机 1】
  g_smd_target_uart = 1; // 路由切到串口1
  clear_uart_rx_buffer(1);
  // 新固件适配：1为“通信位置模式”（不再是旧版的3）
  smd_set_mode(1, 1);
  handle_ack(1, 50);

  clear_uart_rx_buffer(1);
  // 强制保存到内部Flash
  smd_param_save(1);
  handle_ack(1, 50);

  // 【重载电机 PID 微调起步方案】
  // 既然必须保留 Ki，我们就用“极小积分 +
  // 极大阻尼”的策略来压制积分饱和带来的摇摆。 Kp 降到 400（软化弹簧），Ki 设为
  // 2（微弱积分保证最终精度），Kd 设为 600（注入大量阻尼吸收惯量）
  // 恢复重载PID：大幅提高Kp以增加固定牢固度，同时同步提高Kd防止超调摇摆
  clear_uart_rx_buffer(1);
  smd_set_pos_pid(1, 1200, 2,
                  1000); // 👈 (地址, Kp比例, Ki积分,
                         // Kd阻尼)。Kp从400加到1200让它变硬，Kd加到800压制摇摆
  handle_ack(1, 50);

  // 新增：解除电流封印，提供强大的物理电磁握力
  clear_uart_rx_buffer(1);
  smd_set_pos_torque(
      1, 1200); // 👈 最大运行/保持电流设为 1200mA，彻底解决软绵绵定不住的问题
  handle_ack(1, 50);

  // 新增：加速度恢复到 80（避开低速共振区），让运动顺滑，靠上面的 PID
  // 来处理终点刹车
  clear_uart_rx_buffer(1);
  smd_pos_mode(
      1, 0, 5, 80,
      12500); // 👈 (地址, 方向, 加速度acc, 最大速度speed, 脉冲)。【备忘：1整圈
              // = 51200 脉冲，当前12500约为1/4圈】
  handle_ack(1, 50);

  // 【配置电机 2】
  g_smd_target_uart = 2; // 路由切到串口2
  clear_uart_rx_buffer(2);
  smd_set_mode(1, 1); // 电机2同样强制切换到通信位置模式
  handle_ack(2, 50);

  clear_uart_rx_buffer(2);
  smd_param_save(1); // 保存到Flash
  handle_ack(2, 50);

  clear_uart_rx_buffer(2);
  smd_set_pos_pid(1, 1200, 2, 1000); // 电机2同样增加牢固度
  handle_ack(2, 50);

  clear_uart_rx_buffer(2);
  smd_set_pos_torque(1, 1200); // 电机2增加物理电磁握力
  handle_ack(2, 50);

  // 新增：电机2同样恢复加速度避开共振
  clear_uart_rx_buffer(2);
  smd_pos_mode(1, 0, 5, 80, 36300); // 【备忘：1整圈 = 51200 脉冲】
  handle_ack(2, 50);
  /* ==================================================================== */
}
