#include "gw_gray.h"

// 定义灰度探头读取端口映射 (IO1~IO12 从左至右排列)
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
    // 调整探头读取顺序为倒序 (bit11=IO12 ... bit0=IO1) 以匹配物理朝向
    Huidu_Datas = (Read_Huidu_IO12<<11) | (Read_Huidu_IO11<<10) | (Read_Huidu_IO10<<9) | (Read_Huidu_IO9<<8) |
                  (Read_Huidu_IO8<<7)   | (Read_Huidu_IO7<<6)   | (Read_Huidu_IO6<<5)  | (Read_Huidu_IO5<<4) |
                  (Read_Huidu_IO4<<3)   | (Read_Huidu_IO3<<2)   | (Read_Huidu_IO2<<1)  | (Read_Huidu_IO1);
    
    return Huidu_Datas;
}

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
    static uint8_t White_Blind_Count = 0; //  静态变量：记录连续遇到全白的周期数
    float total_weight = 0;
    
    Huidu_Sum = 0;
    for(int i=0; i<12; i++)
    {
        // 硬件电平：白=1，黑=0。因此我们检测0来判断是否压中黑线
        // bit11 对应 i=0(权重-5.5)，bit0 对应 i=11(权重5.5)
        if(((huidu_data >> (11 - i)) & 0x01) == 0) 
        {
            Huidu_Sum++;
            total_weight += Huidu_Weights[i];
        }
    }
    
    // 基础误差计算
    if(Huidu_Sum >= 1 && Huidu_Sum <= 7) // 正常循迹，探测到1~7个黑点都算有效线宽
    {
        White_Blind_Count = 0; // 检测到有效信号，清零干扰计数器
        
        // 加权平均计算误差
        Huidu_Error = total_weight / Huidu_Sum;
        
        // 放大边缘误差（如果偏离过大，加强回正力度）
        if (Huidu_Error > 3.0f || Huidu_Error < -3.0f) {
            Huidu_Error *= 1.2f; 
        }

        // 非线性死区平滑：衰减中心区域微调，防止振荡偏航
        if (Huidu_Error > -1.0f && Huidu_Error < 1.0f) {
            Huidu_Error *= 0.5f; 
        }
    }
    else if(Huidu_Sum > 7)  // 探头触发过多，可能压到十字路口或者大面积黑块
    {
        White_Blind_Count = 0; // 检测到路口特征，清零计数器
        Huidu_Error = huidu_lasterror; // 冲过路口时维持原有误差直行
    }
    else if(Huidu_Sum == 0) // 丢线（全白）
    {
        White_Blind_Count++; // 累计全白周期
        
        // 抗局部干扰逻辑 (盲区补偿)
        if (White_Blind_Count <= 8) 
        {
            // 维持异常前最后一刻的有效偏差状态，执行惯性补偿
            Huidu_Error = huidu_lasterror; 
        }
        else 
        {
            // 当连续丢线时间过长，判定为物理脱轨
            // 触发极限偏差纠正机制：
            if (huidu_lasterror > 0)
                Huidu_Error = 6.0f;  // 施加最大补偿力矩强制纠偏
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