#include "app_lcd.h"
#include "hw_lcd.h"
#include "bsp_gyro.h"
#include "motor_ctrl.h"
#include "task.h"
#include "Encoder.h"
#include "timer.h"
#include "bsp_sr04.h"

// ================= 外部变量声明 =================
extern float Basic_Speed;                                   // 目标基础速度
extern uint8_t OLED_View_Select;                            // OLED选择界面变量

// 速度相关 (这里沿用你代码中的变量名)
extern float Motor1_Speed;                                  // 左轮速度 
extern float Motor2_Speed;                                  // 右轮速度

// 新增：任务状态与传感器变量
extern uint16_t Huidu_Datas;                                 // 灰度原始数据 (12位)

extern float Debug_Yaw_Diff;  // 引入刚才在 task.c 定义的差值变量
extern float Global_Ultrasonic_Distance;

// ================= 脱机在线调参系统状态 =================
uint8_t Tuning_Mode = 0;    // 0: 关闭调参, 1: 开启调参
uint8_t Tuning_Cursor = 0;  // 0: Kp, 1: Ki, 2: Kd
uint8_t Tuning_Loop = 0;    // 0: M1, 1: M2, ...
extern float Task1_Time_Sec;
extern float Task2_Time_Sec;
// 引入在 Encoder.c 中计算的总路程
extern float Measure_Distance;
// ======================================================================
// 1. 底层字符串绘制函数库
// ======================================================================
void TFT_ShowBinNum(uint16_t x, uint16_t y, uint8_t num, uint16_t fc, uint16_t bc, uint8_t sizey) {
    char bin_str[9];
    for(int i=0; i<8; i++) { bin_str[7-i] = (num & (1<<i)) ? '1' : '0'; }
    bin_str[8] = '\0';
    LCD_ShowString(x, y, (uint8_t*)bin_str, fc, bc, sizey, 0); 
}

// 12 位二进制数据显示函数
void TFT_ShowBinNum12(uint16_t x, uint16_t y, uint16_t num, uint16_t fc, uint16_t bc, uint8_t sizey) {
    char bin_str[13];
    // 只取低 12 位循环
    for(int i=0; i<12; i++) { 
        bin_str[11-i] = (num & (1<<i)) ? '1' : '0'; 
    }
    bin_str[12] = '\0'; // 字符串结尾
    LCD_ShowString(x, y, (uint8_t*)bin_str, fc, bc, sizey, 0); 
}

void TFT_ShowInt(uint16_t x, uint16_t y, int32_t val, uint16_t fc, uint16_t bc, uint8_t sizey) {
    char buf[16]; int idx = 0;
    if (val < 0) { buf[idx++] = '-'; val = -val; } else { buf[idx++] = ' '; } 
    char int_buf[10]; int i_idx = 0;
    if (val == 0) { int_buf[i_idx++] = '0'; } else {
        while (val > 0) { int_buf[i_idx++] = (val % 10) + '0'; val /= 10; }
    }
    while (i_idx > 0) { buf[idx++] = int_buf[--i_idx]; }
    buf[idx++] = ' '; buf[idx++] = ' '; buf[idx] = '\0';
    LCD_ShowString(x, y, (uint8_t*)buf, fc, bc, sizey, 0);
}

void TFT_ShowFloat(uint16_t x, uint16_t y, float val, uint16_t fc, uint16_t bc, uint8_t sizey) {
    char buf[20]; int idx = 0;
    if (val < 0) { buf[idx++] = '-'; val = -val; } else { buf[idx++] = ' '; } 
    int32_t int_part = (int32_t)val;
    int32_t frac_part = (int32_t)((val - int_part) * 1000); 
    char int_buf[10]; int i_idx = 0;
    if (int_part == 0) { int_buf[i_idx++] = '0'; } else {
        while (int_part > 0) { int_buf[i_idx++] = (int_part % 10) + '0'; int_part /= 10; }
    }
    while (i_idx > 0) { buf[idx++] = int_buf[--i_idx]; }
    buf[idx++] = '.';
    buf[idx++] = (frac_part / 100) + '0';
    buf[idx++] = ((frac_part / 10) % 10) + '0';
    buf[idx++] = (frac_part % 10) + '0';
    buf[idx++] = ' '; buf[idx++] = ' '; buf[idx] = '\0';
    LCD_ShowString(x, y, (uint8_t*)buf, fc, bc, sizey, 0);
}

// 浮点数居中显示函数
void TFT_ShowFloatCenter(uint16_t rect_x, uint16_t rect_w, uint16_t rect_y, uint16_t rect_h, float val, uint16_t fc, uint16_t bc) {
    char buf[20]; int idx = 0;
    if (val < 0) { buf[idx++] = '-'; val = -val; } else { buf[idx++] = ' '; } 
    int32_t int_part = (int32_t)val;
    int32_t frac_part = (int32_t)((val - int_part) * 1000); 
    char int_buf[10]; int i_idx = 0;
    if (int_part == 0) { int_buf[i_idx++] = '0'; } else {
        while (int_part > 0) { int_buf[i_idx++] = (int_part % 10) + '0'; int_part /= 10; }
    }
    while (i_idx > 0) { buf[idx++] = int_buf[--i_idx]; }
    buf[idx++] = '.';
    buf[idx++] = (frac_part / 100) + '0';
    buf[idx++] = ((frac_part / 10) % 10) + '0';
    buf[idx++] = (frac_part % 10) + '0';
    buf[idx] = '\0';
    
    int str_len = 0; while(buf[str_len] != '\0') str_len++;
    int txt_w = str_len * 8;
    int start_x = rect_x + (rect_w - txt_w) / 2;
    int start_y = rect_y + (rect_h - 16) / 2;
    
    LCD_ShowString(start_x, start_y, (uint8_t*)buf, fc, bc, 16, 0);
}

// ======================================================================
// 2. UI 组件绘制模块
// ======================================================================

static void UI_DrawTitleCenter(int y, uint16_t bg_color, const char* str) {
    int pixel_width = 0;
    const unsigned char* s = (const unsigned char*)str;
    while(*s != '\0') {
        if(*s >= 128) { pixel_width += 16; s += 3; }
        else          { pixel_width += 8;  s += 1; }
    }
    int str_center_x = pixel_width / 2;
    int center_x = LCD_W / 2; 
    
    LCD_ArcRect(center_x - str_center_x - 10, y, center_x + str_center_x + 10, y + 16, bg_color);
    LCD_ShowChinese(center_x - str_center_x, y, (unsigned char*)str, WHITE, bg_color, 16, 1);
}

// 主界面仪表盘背景绘制
static void Draw_Dashboard_Background(void) {
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    UI_DrawTitleCenter(10, BLUE, "系统综合看板");

    // 左侧圆角方块：速度与任务信息
    LCD_ArcRect(10, 40, 155, 160, GRAYBLUE);
    LCD_ShowChinese(15, 45,  (uint8_t *)"基础速:", GREEN, GRAYBLUE, 16, 1);
    LCD_ShowChinese(15, 75,  (uint8_t *)"任务一:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowChinese(15, 105, (uint8_t *)"任务二:", WHITE, GRAYBLUE, 16, 1);
    LCD_ShowChinese(15, 135, (uint8_t *)"初始角:", WHITE, GRAYBLUE, 16, 1);

    // 右侧圆角方块：姿态与轮速信息
    LCD_ArcRect(165, 40, 310, 160, GRAYBLUE);
    LCD_ShowChinese(167, 45,  (uint8_t *)"偏航角:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowChinese(167, 75,  (uint8_t *)"灰度:", CYAN,  GRAYBLUE, 16, 1); 
    LCD_ShowChinese(167, 105, (uint8_t *)"左轮速:", WHITE, GRAYBLUE, 16, 1);
    LCD_ShowChinese(167, 135, (uint8_t *)"右轮速:", WHITE, GRAYBLUE, 16, 1);

    //  新增：表格分割线 (Grid)
    LCD_DrawLine(10, 68, 155, 68, DARKBLUE);
    LCD_DrawLine(10, 98, 155, 98, DARKBLUE);
    LCD_DrawLine(10, 128, 155, 128, DARKBLUE);
    LCD_DrawLine(165, 68, 310, 68, DARKBLUE);
    LCD_DrawLine(165, 98, 310, 98, DARKBLUE);
    LCD_DrawLine(165, 128, 310, 128, DARKBLUE);
}

static void Draw_Dashboard_Page2_Background(void) {
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    UI_DrawTitleCenter(10, BLUE, "循迹PID看板");

    // 左侧信息区域绘制
    LCD_ArcRect(10, 40, 160, 160, GRAYBLUE); // 调整边框宽度以适配数据长度
    LCD_ShowChinese(15, 45,  (uint8_t *)"循迹Kp:", GREEN, GRAYBLUE, 16, 1);
    LCD_ShowChinese(15, 75,  (uint8_t *)"循迹Ki:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowChinese(15, 105, (uint8_t *)"循迹Kd:", WHITE, GRAYBLUE, 16, 1);
    LCD_ShowChinese(15, 135, (uint8_t *)"基础速:", WHITE, GRAYBLUE, 16, 1);

    // 右侧信息区域绘制
    LCD_ArcRect(165, 40, 310, 160, GRAYBLUE);
    LCD_ShowChinese(167, 45,  (uint8_t *)"偏航角:", YELLOW, GRAYBLUE, 16, 1); 
    LCD_ShowChinese(167, 75,  (uint8_t *)"灰度:", CYAN,  GRAYBLUE, 16, 1); 
    LCD_ShowChinese(167, 105, (uint8_t *)"左轮速:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowChinese(167, 135, (uint8_t *)"右轮速:", WHITE, GRAYBLUE, 16, 1); 

    //  新增：表格分割线 (Grid)
    LCD_DrawLine(10, 68, 155, 68, DARKBLUE);
    LCD_DrawLine(10, 98, 155, 98, DARKBLUE);
    LCD_DrawLine(10, 128, 155, 128, DARKBLUE);
    LCD_DrawLine(165, 68, 310, 68, DARKBLUE);
    LCD_DrawLine(165, 98, 310, 98, DARKBLUE);
    LCD_DrawLine(165, 128, 310, 128, DARKBLUE);
}

static void Draw_Dashboard_Page3_Background(void) {
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    UI_DrawTitleCenter(10, BLUE, "测试PID看板");

    // 左侧信息区域绘制
    LCD_ArcRect(10, 40, 160, 160, GRAYBLUE); // 调整边框宽度以适配数据长度
    LCD_ShowString(15, 45,  (uint8_t *)"M1 Kp:", GREEN, GRAYBLUE, 16, 1);
    LCD_ShowString(15, 75,  (uint8_t *)"M1 Ki:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowString(15, 105, (uint8_t *)"M1 Kd:", WHITE, GRAYBLUE, 16, 1);
    LCD_ShowChinese(15, 135, (uint8_t *)"左轮速:", YELLOW, GRAYBLUE, 16, 1);

    // 右侧信息区域绘制
    LCD_ArcRect(165, 40, 310, 160, GRAYBLUE);
    LCD_ShowString(167, 45,  (uint8_t *)"M2 Kp:", GREEN, GRAYBLUE, 16, 1); 
    LCD_ShowString(167, 75,  (uint8_t *)"M2 Ki:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowString(167, 105, (uint8_t *)"M2 Kd:", WHITE, GRAYBLUE, 16, 1); 
    LCD_ShowChinese(167, 135, (uint8_t *)"右轮速:", YELLOW, GRAYBLUE, 16, 1); 

    //  新增：表格分割线 (Grid)
    LCD_DrawLine(10, 68, 155, 68, DARKBLUE);
    LCD_DrawLine(10, 98, 155, 98, DARKBLUE);
    LCD_DrawLine(10, 128, 155, 128, DARKBLUE);
    
    LCD_DrawLine(165, 68, 310, 68, DARKBLUE);
    LCD_DrawLine(165, 98, 310, 98, DARKBLUE);
    LCD_DrawLine(165, 128, 310, 128, DARKBLUE);
}

static void Draw_Dashboard_Page4_Background(void) {
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    UI_DrawTitleCenter(10, BLUE, "WAVEFORM");

    // 外边框
    LCD_ArcRect(10, 40, 310, 230, GRAYBLUE);
    
    // 标签
    LCD_ShowChinese(15, 45, (uint8_t *)"左轮速", GREEN, BLACK, 16, 1);
    LCD_ShowChinese(115, 45, (uint8_t *)"右轮速", RED, BLACK, 16, 1);
    LCD_ShowChinese(215, 45, (uint8_t *)"基础速", YELLOW, BLACK, 16, 1);
    
    // 底层虚线基准线
    for(int i=20; i<=300; i+=4) {
        LCD_DrawPoint(i, 140, DARKBLUE); // 中线
    }
}

static void Draw_App_UI_PID_Background(const char* title) {
    LCD_Fill(0, 0, LCD_W, LCD_H, BLACK);
    UI_DrawTitleCenter(10, BLUE, title);

    int center_x = 160;
    int str_center_x = 12;

    LCD_ShowChar(center_x - str_center_x - 84, 75, 'P', WHITE, BLACK, 16, 1);
    LCD_ShowChar(center_x - str_center_x,      75, 'I', WHITE, BLACK, 16, 1);
    LCD_ShowChar(center_x - str_center_x + 84, 75, 'D', WHITE, BLACK, 16, 1);

    LCD_ArcRect(34,  95, 104, 119, BLUE);
    LCD_ArcRect(118, 95, 188, 119, BLUE);
    LCD_ArcRect(202, 95, 272, 119, BLUE);
    
    LCD_ShowString(20, 140, (const unsigned char*)"Status: RUNNING", GRAY, BLACK, 16, 1);
}

// ======================================================================
// 3. UI 界面刷新主循环
// ======================================================================

void Update_App_UI_PID_Values(float kp, float ki, float kd) {
    TFT_ShowFloatCenter(34,  70, 95, 24, kp, YELLOW, BLUE);
    TFT_ShowFloatCenter(118, 70, 95, 24, ki, YELLOW, BLUE);
    TFT_ShowFloatCenter(202, 70, 95, 24, kd, YELLOW, BLUE);
}

//  新增：速度迷你进度条
static void Draw_Speed_Bar(int x, int y, float speed) {
    int max_speed = 50; 
    int bar_width = 80;
    int fill_w = (int)((speed / max_speed) * bar_width);
    if(fill_w > bar_width) fill_w = bar_width;
    if(fill_w < -bar_width) fill_w = -bar_width;
    if(fill_w < 0) fill_w = -fill_w; // 取绝对值作为宽度
    
    uint16_t color = (speed >= 0) ? GREEN : RED; // 正转绿，倒车红
    
    LCD_Fill(x, y, x + bar_width, y + 4, BLACK); // 背景槽加粗到 5px 高
    if(fill_w > 0) {
        LCD_Fill(x, y, x + fill_w, y + 4, color);
    }
}

extern volatile uint8_t flag_50ms_lcd;

void LCD_Show_Proc(void)
{
    if (!flag_50ms_lcd) return;
    flag_50ms_lcd = 0;

    static uint8_t last_view = 255;
    static uint8_t last_tuning_mode = 255;
    static uint8_t last_tuning_loop = 255;
    static uint8_t last_tuning_cursor = 255;

    // 1. 静态背景层绘制 (仅在界面切换时刷新)
    if(last_view != OLED_View_Select) {
        // 界面切换时，重置边框绘制记录，强制下一次绘制新边框
        last_tuning_mode = 255; 
        
        switch(OLED_View_Select) {
            case 1: Draw_Dashboard_Page3_Background(); break;
            case 2: Draw_Dashboard_Page2_Background(); break;
            case 3: Draw_Dashboard_Background(); break;
            case 4: Draw_Dashboard_Page4_Background(); break;
            case 5: Draw_App_UI_PID_Background("MOTOR 1 PID"); break;
            case 6: Draw_App_UI_PID_Background("MOTOR 2 PID"); break;
            case 7: Draw_App_UI_PID_Background("TURN PID"); break;
            case 8: Draw_App_UI_PID_Background("DISTANCE PID"); break;
            case 9: Draw_App_UI_PID_Background("GYRO PID"); break;
            case 10: Draw_App_UI_PID_Background("ANGLE PID"); break;
            default: break;
        }
        last_view = OLED_View_Select;
    }

    // 2. 动态数据层持续刷新
    switch(OLED_View_Select)
    {
        case 3: {
            //  新增：动态颜色报警计算
            uint16_t yaw_color = GREEN;
            if(Debug_Yaw_Diff > 20 || Debug_Yaw_Diff < -20) yaw_color = RED;
            else if(Debug_Yaw_Diff > 10 || Debug_Yaw_Diff < -10) yaw_color = YELLOW;

            // 左侧：速度与任务时间
            TFT_ShowFloat(75, 45,  Basic_Speed,  GREEN, GRAYBLUE, 16);
            TFT_ShowFloat(75, 75,  Task1_Time_Sec, WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(75, 105, Task2_Time_Sec, WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(75, 135, Measure_Distance, WHITE, GRAYBLUE, 16);

            // 右侧：偏航角、灰度、双轮速
            TFT_ShowFloat(225, 45, Debug_Yaw_Diff, yaw_color, GRAYBLUE, 16);        
            TFT_ShowBinNum12(210, 75, Huidu_Datas, CYAN, GRAYBLUE, 16);
            TFT_ShowFloat(225, 105, Motor1_Speed, WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(225, 135, Motor2_Speed, WHITE, GRAYBLUE, 16);
            
            //  新增：绘制进度条 (位于文字下方空白处)
            Draw_Speed_Bar(225, 124, Motor1_Speed);
            Draw_Speed_Bar(225, 154, Motor2_Speed);
            break;
        }
            
        case 2: {
            uint16_t yaw_color = GREEN;
            if(Debug_Yaw_Diff > 20 || Debug_Yaw_Diff < -20) yaw_color = RED;
            else if(Debug_Yaw_Diff > 10 || Debug_Yaw_Diff < -10) yaw_color = YELLOW;

            // ================= 左侧：循迹 PID (pid_Turn) =================
            // 动态调参选择框绘制
            if (Tuning_Mode != last_tuning_mode || Tuning_Cursor != last_tuning_cursor) {
                last_tuning_mode = Tuning_Mode;
                last_tuning_cursor = Tuning_Cursor;
                
                if (Tuning_Mode) {
                    LCD_DrawRectangle(71, 43, 158, 63, (Tuning_Cursor == 0) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(71, 73, 158, 93, (Tuning_Cursor == 1) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(71, 103, 158, 123, (Tuning_Cursor == 2) ? YELLOW : GRAYBLUE);
                    LCD_ShowString(5, 10, (uint8_t *)" TUNING ", BLACK, YELLOW, 16, 0); // 移到左上角
                } else {
                    LCD_DrawRectangle(71, 43, 158, 63, GRAYBLUE);
                    LCD_DrawRectangle(71, 73, 158, 93, GRAYBLUE);
                    LCD_DrawRectangle(71, 103, 158, 123, GRAYBLUE);
                    LCD_ShowString(5, 10, (uint8_t *)"        ", BLACK, BLACK, 16, 0);  // 清除左上角提示
                }
            }

            // 数据全部对齐到 X = 72
            TFT_ShowFloat(72, 45,  pid_Turn.Kp, (Tuning_Mode && Tuning_Cursor == 0) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(72, 75,  pid_Turn.Ki, (Tuning_Mode && Tuning_Cursor == 1) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(72, 105, pid_Turn.Kd, (Tuning_Mode && Tuning_Cursor == 2) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(72, 135, Basic_Speed, WHITE, GRAYBLUE, 16);

            // 右侧：传感器输入与电机输出
            TFT_ShowFloat(225, 45,  Debug_Yaw_Diff, yaw_color, GRAYBLUE, 16);        
            TFT_ShowBinNum12(210, 75, Huidu_Datas, CYAN, GRAYBLUE, 16);
            TFT_ShowFloat(225, 105, Motor1_Speed, WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(225, 135, Motor2_Speed, WHITE, GRAYBLUE, 16);
            
            //  新增：绘制进度条
            Draw_Speed_Bar(225, 124, Motor1_Speed);
            Draw_Speed_Bar(225, 154, Motor2_Speed);
            break;
        }

        case 1: {
            // ================= 左侧：M1 PID，右侧：M2 PID =================
            // 动态调参选择框绘制
            if (Tuning_Mode != last_tuning_mode || Tuning_Loop != last_tuning_loop || Tuning_Cursor != last_tuning_cursor) {
                last_tuning_mode = Tuning_Mode;
                last_tuning_loop = Tuning_Loop;
                last_tuning_cursor = Tuning_Cursor;
                
                if (Tuning_Mode && (Tuning_Loop == 0 || Tuning_Loop == 1)) {
                    uint16_t bx = (Tuning_Loop == 0) ? 69 : 222; // 基础X坐标
                    
                    LCD_DrawRectangle(bx, 41, bx+91, 65, (Tuning_Cursor == 0) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(bx+1, 42, bx+90, 64, (Tuning_Cursor == 0) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(bx+2, 43, bx+89, 63, (Tuning_Cursor == 0) ? YELLOW : GRAYBLUE);
                    
                    LCD_DrawRectangle(bx, 71, bx+91, 95, (Tuning_Cursor == 1) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(bx+1, 72, bx+90, 94, (Tuning_Cursor == 1) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(bx+2, 73, bx+89, 93, (Tuning_Cursor == 1) ? YELLOW : GRAYBLUE);
                    
                    LCD_DrawRectangle(bx, 101, bx+91, 125, (Tuning_Cursor == 2) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(bx+1, 102, bx+90, 124, (Tuning_Cursor == 2) ? YELLOW : GRAYBLUE);
                    LCD_DrawRectangle(bx+2, 103, bx+89, 123, (Tuning_Cursor == 2) ? YELLOW : GRAYBLUE);
                    
                    // 清除另一边的框
                    uint16_t ox = (Tuning_Loop == 0) ? 222 : 69;
                    LCD_DrawRectangle(ox, 41, ox+91, 65, GRAYBLUE);
                    LCD_DrawRectangle(ox+1, 42, ox+90, 64, GRAYBLUE);
                    LCD_DrawRectangle(ox+2, 43, ox+89, 63, GRAYBLUE);
                    LCD_DrawRectangle(ox, 71, ox+91, 95, GRAYBLUE);
                    LCD_DrawRectangle(ox+1, 72, ox+90, 94, GRAYBLUE);
                    LCD_DrawRectangle(ox+2, 73, ox+89, 93, GRAYBLUE);
                    LCD_DrawRectangle(ox, 101, ox+91, 125, GRAYBLUE);
                    LCD_DrawRectangle(ox+1, 102, ox+90, 124, GRAYBLUE);
                    LCD_DrawRectangle(ox+2, 103, ox+89, 123, GRAYBLUE);

                    LCD_ShowString(5, 10, (uint8_t *)" TUNING ", BLACK, YELLOW, 16, 0); // 移到左上角
                } else {
                    LCD_DrawRectangle(69, 41, 160, 65, GRAYBLUE);
                    LCD_DrawRectangle(70, 42, 159, 64, GRAYBLUE);
                    LCD_DrawRectangle(71, 43, 158, 63, GRAYBLUE);
                    LCD_DrawRectangle(69, 71, 160, 95, GRAYBLUE);
                    LCD_DrawRectangle(70, 72, 159, 94, GRAYBLUE);
                    LCD_DrawRectangle(71, 73, 158, 93, GRAYBLUE);
                    LCD_DrawRectangle(69, 101, 160, 125, GRAYBLUE);
                    LCD_DrawRectangle(70, 102, 159, 124, GRAYBLUE);
                    LCD_DrawRectangle(71, 103, 158, 123, GRAYBLUE);
                    
                    LCD_DrawRectangle(222, 41, 313, 65, GRAYBLUE);
                    LCD_DrawRectangle(223, 42, 312, 64, GRAYBLUE);
                    LCD_DrawRectangle(224, 43, 311, 63, GRAYBLUE);
                    LCD_DrawRectangle(222, 71, 313, 95, GRAYBLUE);
                    LCD_DrawRectangle(223, 72, 312, 94, GRAYBLUE);
                    LCD_DrawRectangle(224, 73, 311, 93, GRAYBLUE);
                    LCD_DrawRectangle(222, 101, 313, 125, GRAYBLUE);
                    LCD_DrawRectangle(223, 102, 312, 124, GRAYBLUE);
                    LCD_DrawRectangle(224, 103, 311, 123, GRAYBLUE);

                    LCD_ShowString(5, 10, (uint8_t *)"        ", BLACK, BLACK, 16, 0);  // 清除左上角提示
                }
            }

            // 数据全部对齐到 X = 72
            TFT_ShowFloat(72, 45,  pid_Motor1_Speed.Kp, (Tuning_Mode && Tuning_Loop == 0 && Tuning_Cursor == 0) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(72, 75,  pid_Motor1_Speed.Ki, (Tuning_Mode && Tuning_Loop == 0 && Tuning_Cursor == 1) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(72, 105, pid_Motor1_Speed.Kd, (Tuning_Mode && Tuning_Loop == 0 && Tuning_Cursor == 2) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(72, 135, Motor1_Speed, WHITE, GRAYBLUE, 16);

            // ================= 右侧：M2 PID =================
            TFT_ShowFloat(225, 45,  pid_Motor2_Speed.Kp, (Tuning_Mode && Tuning_Loop == 1 && Tuning_Cursor == 0) ? YELLOW : WHITE, GRAYBLUE, 16);        
            TFT_ShowFloat(225, 75,  pid_Motor2_Speed.Ki, (Tuning_Mode && Tuning_Loop == 1 && Tuning_Cursor == 1) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(225, 105, pid_Motor2_Speed.Kd, (Tuning_Mode && Tuning_Loop == 1 && Tuning_Cursor == 2) ? YELLOW : WHITE, GRAYBLUE, 16);
            TFT_ShowFloat(225, 135, Motor2_Speed, WHITE, GRAYBLUE, 16);
            
            //  绘制进度条
            Draw_Speed_Bar(72, 154, Motor1_Speed);
            Draw_Speed_Bar(225, 154, Motor2_Speed);
            
            // 临时在左上角/顶栏显示超声波距离
            TFT_ShowFloat(80, 10, Global_Ultrasonic_Distance, WHITE, BLACK, 16);
            LCD_ShowString(150, 10, (uint8_t *)"cm", GREEN, BLACK, 16, 0);

            break;
        }

        case 4: {
            static int wave_idx = 0;
            static int last_y_m1 = 140, last_y_m2 = 140, last_y_base = 140;

            int x_start = 20;
            int y_start = 220; // Bottom
            int y_mid = 140;   // Middle
            int max_speed = 50; 

            // 数值显示 (覆盖在顶部)
            TFT_ShowFloat(65, 45, Motor1_Speed, GREEN, BLACK, 16);
            TFT_ShowFloat(165, 45, Motor2_Speed, RED, BLACK, 16);
            TFT_ShowFloat(265, 45, Basic_Speed, YELLOW, BLACK, 16);
            
            // 映射到坐标系 (以 140 为 0 点，这样正负速度都能显示)
            int y_m1 = 140 - (int)((Motor1_Speed / max_speed) * 75);
            int y_m2 = 140 - (int)((Motor2_Speed / max_speed) * 75);
            int y_base = 140 - (int)((Basic_Speed / max_speed) * 75);

            if(y_m1 < 65) y_m1 = 65; if(y_m1 > 220) y_m1 = 220;
            if(y_m2 < 65) y_m2 = 65; if(y_m2 > 220) y_m2 = 220;
            if(y_base < 65) y_base = 65; if(y_base > 220) y_base = 220;

            int cur_x = x_start + wave_idx;
            int next_idx = (wave_idx + 1) % 280;
            int sweep_x = x_start + next_idx;

            if (wave_idx == 0) {
                // 一轮的起始点：不需要连线，直接描点
                LCD_DrawPoint(cur_x, y_m1, GREEN);
                LCD_DrawPoint(cur_x, y_m2, RED);
                LCD_DrawPoint(cur_x, y_base, YELLOW);
            } else {
                // 与上一个点平滑连线
                LCD_DrawLine(cur_x - 1, last_y_m1, cur_x, y_m1, GREEN);
                LCD_DrawLine(cur_x - 1, last_y_m2, cur_x, y_m2, RED);
                LCD_DrawLine(cur_x - 1, last_y_base, cur_x, y_base, YELLOW);
            }

            // 更新历史坐标
            last_y_m1 = y_m1;
            last_y_m2 = y_m2;
            last_y_base = y_base;

            // 清除下一列（即扫除前方的旧波形和白线）
            LCD_DrawLine(sweep_x, 65, sweep_x, 222, BLACK);
            
            // 补画中线基准点
            if (next_idx % 4 == 0) LCD_DrawPoint(sweep_x, y_mid, DARKBLUE);

            // 画新的扫描前沿引导线
            if(sweep_x + 1 <= x_start + 279) {
                LCD_DrawLine(sweep_x + 1, 65, sweep_x + 1, 220, WHITE);
            }

            wave_idx = next_idx;
            break;
        }

        case 5: Update_App_UI_PID_Values(pid_Motor1_Speed.Kp, pid_Motor1_Speed.Ki, pid_Motor1_Speed.Kd); break;
        case 6: Update_App_UI_PID_Values(pid_Motor2_Speed.Kp, pid_Motor2_Speed.Ki, pid_Motor2_Speed.Kd); break;
        case 7: Update_App_UI_PID_Values(pid_Turn.Kp, pid_Turn.Ki, pid_Turn.Kd); break;
        case 8: Update_App_UI_PID_Values(pid_Distance.Kp, pid_Distance.Ki, pid_Distance.Kd); break;
        case 9: Update_App_UI_PID_Values(pid_Gyro.Kp, pid_Gyro.Ki, pid_Gyro.Kd); break;
        case 10: Update_App_UI_PID_Values(pid_Angle.Kp, pid_Angle.Ki, pid_Angle.Kd); break;
        default: break;
    }
}