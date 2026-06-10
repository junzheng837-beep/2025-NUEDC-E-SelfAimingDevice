#include "gw_gray.h"
#include "delay.h"

// 假设从左到右依次为 IO1 ~ IO12，引脚定义需在 SysConfig 中命名为 Huidu_IN1 ~ Huidu_IN12
#define Read_Huidu_IO1   ((DL_GPIO_readPins(Huidu_IN1_PORT, Huidu_IN1_PIN)==Huidu_IN1_PIN)?0:1)
#define Read_Huidu_IO2   ((DL_GPIO_readPins(Huidu_IN2_PORT, Huidu_IN2_PIN)==Huidu_IN2_PIN)?0:1)
#define Read_Huidu_IO3   ((DL_GPIO_readPins(Huidu_IN3_PORT, Huidu_IN3_PIN)==Huidu_IN3_PIN)?0:1)
#define Read_Huidu_IO4   ((DL_GPIO_readPins(Huidu_IN4_PORT, Huidu_IN4_PIN)==Huidu_IN4_PIN)?0:1)
#define Read_Huidu_IO5   ((DL_GPIO_readPins(Huidu_IN5_PORT, Huidu_IN5_PIN)==Huidu_IN5_PIN)?0:1)
#define Read_Huidu_IO6   ((DL_GPIO_readPins(Huidu_IN6_PORT, Huidu_IN6_PIN)==Huidu_IN6_PIN)?0:1)
#define Read_Huidu_IO7   ((DL_GPIO_readPins(Huidu_IN7_PORT, Huidu_IN7_PIN)==Huidu_IN7_PIN)?0:1)
#define Read_Huidu_IO8   ((DL_GPIO_readPins(Huidu_IN8_PORT, Huidu_IN8_PIN)==Huidu_IN8_PIN)?0:1)
#define Read_Huidu_IO9   ((DL_GPIO_readPins(Huidu_IN9_PORT, Huidu_IN9_PIN)==Huidu_IN9_PIN)?0:1)
#define Read_Huidu_IO10  ((DL_GPIO_readPins(Huidu_IN10_PORT, Huidu_IN10_PIN)==Huidu_IN10_PIN)?0:1)
#define Read_Huidu_IO11  ((DL_GPIO_readPins(Huidu_IN11_PORT, Huidu_IN11_PIN)==Huidu_IN11_PIN)?0:1)
#define Read_Huidu_IO12  ((DL_GPIO_readPins(Huidu_IN12_PORT, Huidu_IN12_PIN)==Huidu_IN12_PIN)?0:1)

uint16_t Huidu_Datas;

uint16_t Huidu_Read(void)
{ 
    // 将12个探头数据合并成一个12位二进制数
    // 修复朝向相反问题：改为 bit11(IO12) ... bit0(IO1) 倒序排列
    Huidu_Datas = (Read_Huidu_IO12<<11) | (Read_Huidu_IO11<<10) | (Read_Huidu_IO10<<9) | (Read_Huidu_IO9<<8) |
                  (Read_Huidu_IO8<<7)   | (Read_Huidu_IO7<<6)   | (Read_Huidu_IO6<<5)  | (Read_Huidu_IO5<<4) |
                  (Read_Huidu_IO4<<3)   | (Read_Huidu_IO3<<2)   | (Read_Huidu_IO2<<1)  | (Read_Huidu_IO1);
    
    return Huidu_Datas;
}

float Huidu_Target = 0;
float Huidu_Error;
float huidu_error;
float huidu_lasterror;
int Huidu_Sum;

// 12路灰度探头对应的权重 (从左至右)
// 权重可根据实际传感器间距进行微调
const float Huidu_Weights[12] = {
    -5.5f, -4.5f, -3.5f, -2.5f, -1.5f, -0.5f,
     0.5f,  1.5f,  2.5f,  3.5f,  4.5f,  5.5f
};

float Huidu_Proc(uint16_t huidu_data)
{
    static uint8_t White_Blind_Count = 0; // 🌟 静态变量：记录连续遇到全白的周期数
    float total_weight = 0;
    
    Huidu_Sum = 0;
    for(int i=0; i<12; i++)
    {
        // bit11 对应 i=0(权重-5.5)，bit0 对应 i=11(权重5.5)
        if((huidu_data >> (11 - i)) & 0x01) 
        {
            Huidu_Sum++;
            total_weight += Huidu_Weights[i];
        }
    }
    
    // 基础误差计算
    if(Huidu_Sum >= 1 && Huidu_Sum <= 7) // 正常循迹，探测到1~7个黑点都算有效线宽
    {
        White_Blind_Count = 0; // 🌟 只要看到线，立刻清零盲冲计数器
        
        // 加权平均计算误差
        Huidu_Error = total_weight / Huidu_Sum;
        
        // 放大边缘误差（如果偏离过大，加强回正力度）
        if (Huidu_Error > 3.0f || Huidu_Error < -3.0f) {
            Huidu_Error *= 1.2f; 
        }

        // 非线性平滑处理，对中心区域微小偏差进行衰减，防止左右画龙
        if (Huidu_Error > -1.0f && Huidu_Error < 1.0f) {
            Huidu_Error *= 0.5f; 
        }
    }
    else if(Huidu_Sum > 7)  // 探头触发过多，可能压到十字路口或者大面积黑块
    {
        White_Blind_Count = 0; // 🌟 看到大黑块也清零
        Huidu_Error = huidu_lasterror; // 冲过路口保持原有误差直行
    }
    else if(Huidu_Sum == 0) // 丢线（全白）
    {
        White_Blind_Count++; // 累计全白周期
        
        // 🌟【无视扑克牌干扰核心逻辑】🌟
        if (White_Blind_Count <= 8) 
        {
            // 【盲冲姿态维持】：维持压上干扰物那一瞬间的误差
            Huidu_Error = huidu_lasterror; 
        }
        else 
        {
            // 如果超过 80ms 还是全白，说明是真的脱轨飞出了！
            // 此时立刻重新启动极限救车机制：
            if (huidu_lasterror > 0)
                Huidu_Error = 6.0f;  // 用比边缘更大的力矩强行拽回 (根据实际调整)
            else if (huidu_lasterror < 0)
                Huidu_Error = -6.0f; 
            else
                Huidu_Error = 0;   
        }
    }
    
    huidu_error = Huidu_Error;
    huidu_lasterror = huidu_error;
    
    return huidu_error;
}