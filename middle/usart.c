/***
	*******************************************************************************************************************************************************************
	* @file    usart.c
	* @version V2.1
	* @date    2024-7-22
	* @author  御龙	
	* @brief   MSPM0G3507小车PID调试通用模板
   *************************************************************************************************
   *  @description
	*	
	*  接口配置可以使用Sysconfig看
   *
>>>>> 其他说明：未经允许不可擅自转发、售卖本套代码，大家都是我国的有为青年，请保持好自己的初心，在此，向你表达我的感谢
	*************************************************************************************************************************************************************
***/
#include <stdbool.h>
#include "usart.h"
#include "protocol.h"
#include "timer.h"
#include "task.h"
#include "key.h"

// 确保在文件最上面声明一下解析函数（或者把它放进头文件里）
extern void K230_Parse_Data(uint8_t byte);

uint8_t uart_data = 0;
//                                  帧头（2）       编号    方向        转动的步（2）   校验        帧尾（2）
volatile uint8_t rx_order[] = {[0]=0x00,[1]=0x00,[2]=0x00,[3]=0x00,[4]=0x00,[5]=0x00,[6]=0x02,[7]=0xFF,[8]=0xFA};
volatile uint8_t last_data = 0x00;
volatile uint8_t pt = 0x00;

// 定义全局变量，用于存储接收到的帧（需要在对应的头文件或 task.c 中 extern 声明）
volatile bool g_rx_frame_flag = false;
volatile uint8_t g_rx_len = 0;
uint8_t g_rx_cmd[128]; // 接收缓冲区
// ================= 新增：电机2的接收缓存 =================
volatile bool g_rx2_frame_flag = false;
volatile uint8_t g_rx2_len = 0;
uint8_t g_rx2_cmd[128];
// ================= 新增：UART_1 的接收缓存与变量 =================
uint8_t uart1_data = 0;
volatile bool g_rx_uart1_flag = false;


/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------步进电机串口--------------------------------------------*/
/*--------------------------步进电机串口发送测试：smd_send_data("12345\n",10);-------------------*/
/*-------------------------------------------------------------------------------------------*/


/*-------------------------------------------------------------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
int LOG_Debug_Out(const char* __file, const char* __func, int __line, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    // 前缀信息
    char log_buff[64] = {0};
    sprintf(log_buff, "[%s Func:%s Line:%d] ",__file,__func,__line);

    // 创建一个足够大的缓冲区来存储格式化后的字符串
    char buffer[512] = {0};
    strcpy(buffer, log_buff); // 使用strcpy来复制前缀信息
    int len = vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, args); // 追加到buffer中

    va_end(args);

    // 发送格式化后的字符串
    char temp_buff[] = "\r\n";
    strcat(buffer, temp_buff);
    uart0_send_string(buffer);

    return len;
}



int lc_printf(char* format,...)
{
    va_list args;
    va_start(args, format);

    // 创建一个足够大的缓冲区来存储格式化后的字符串
    char buffer[512] = {0};
    int len = vsnprintf(buffer, sizeof(buffer), format, args);

    va_end(args);

    // 发送格式化后的字符串
    uart0_send_string(buffer);

    return len;
}
//串口发送单个字符
void uart0_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_0_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(UART_0_INST, ch);
}

//串口发送字符串
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart0_send_char(*str++);
    }
}

void usart0_send_byte(unsigned char byte)
{
	DL_UART_Main_transmitDataBlocking(UART_0_INST, byte);
}
void usart0_send_bytes(unsigned char *buf, int len)
{
  while(len--)
  {
		DL_UART_Main_transmitDataBlocking(UART_0_INST, *buf);
    buf++;
  }
}
uint8_t HAL_UART_Transmit( uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	uint8_t  *pdata8bits;
	uint8_t TxXferCount;
    pdata8bits  = pData;
	TxXferCount = Size;
    while(TxXferCount > 0U)
    {
		if(pdata8bits != NULL)
		{
			DL_UART_Main_transmitDataBlocking(UART_1_INST, *pdata8bits);
			pdata8bits++;
		}
		TxXferCount--;
		// 等待传输寄存器为空   
		while((UART_STAT_TXFE_MASK & (1 << UART_STAT_TXFF_OFS)) != 0);
    }
    return 1;
}

int fputc(int ch, FILE *f)
{
  DL_UART_Main_transmitDataBlocking(UART_0_INST, ch);
  return ch;
}
// ================= 新增：UART_1 发送相关代码 =================

// UART_1 发送单个字符
void uart1_send_char(char ch)
{
    //当串口忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_1_INST) == true );
    DL_UART_Main_transmitData(UART_1_INST, ch);
}

// UART_1 发送字符串
void uart1_send_string(char* str)
{
    while(*str!=0&&str!=0)
    {
        uart1_send_char(*str++);
    }
}

// UART_1 发送单个字节 (阻塞模式)
void usart1_send_byte(unsigned char byte)
{
    DL_UART_Main_transmitDataBlocking(UART_1_INST, byte);
}

// ==========================================================
/*-------------------------------------------------------------------------------------------*/
/*----------------------也可以使用VOFA+观察波形，选择JustFloat协议---------------------------*/
/*-------------------------------------------------------------------------------------------*/
/*
要点提示:
1. float和unsigned long具有相同的数据结构长度
2. union据类型里的数据存放在相同的物理空间
*/
typedef union{
    float fdata;
    unsigned long ldata;
} FloatLongType;
void JustFloat_SendArray(uint8_t *string,uint8_t length)
{
	while(length--)
	{
	DL_UART_Main_transmitDataBlocking(UART_0_INST, *string++);
	}
}
/*将浮点数f转化为4个字节数据存放在byte[4]中*/
void Float_to_Byte(float f,unsigned char byte[]){
    FloatLongType fl;
    fl.fdata=f;
    byte[0]=(unsigned char)fl.ldata;
    byte[1]=(unsigned char)(fl.ldata>>8);
    byte[2]=(unsigned char)(fl.ldata>>16);
    byte[3]=(unsigned char)(fl.ldata>>24);
}
void JustFloat_Test(void)	//justfloat 数据协议测试
{
    float a=1,b=2;	//发送的数据 两个通道
	
	u8 byte[4]={0};		//float转化为4个字节数据
	u8 tail[4]={0x00, 0x00, 0x80, 0x7f};	//帧尾
	
	//向上位机发送两个通道数据
	Float_to_Byte(a,byte);
	//u1_printf("%f\r\n",a);
	JustFloat_SendArray(byte,4);	//1转化为4字节数据 就是  0x00 0x00 0x80 0x3F
	
	Float_to_Byte(b,byte);
	JustFloat_SendArray(byte,4);	//2转换为4字节数据 就是  0x00 0x00 0x00 0x40 
	
	//发送帧尾
	JustFloat_SendArray(tail,4);	//帧尾为 0x00 0x00 0x80 0x7f

}
//向vofa发送数据  三个数据  三个通道  可视化显示  帧尾
void vofa_sendData(float a,float b,float c){
    u8 byte[4]= {0};//float转化为4个字节数据
    u8 tail[4]= {0x00, 0x00, 0x80, 0x7f};//帧尾

    //向上位机发送通道数据
    Float_to_Byte(a,byte);
    JustFloat_SendArray(byte,4);	

    Float_to_Byte(b,byte);
    JustFloat_SendArray(byte,4);	

    Float_to_Byte(c,byte);
    JustFloat_SendArray(byte,4);
    //发送帧尾
    JustFloat_SendArray(tail,4);	//帧尾为 0x00 0x00 0x80 0x7f
}


#define BUFFER_SIZE 32
char serial_buffer[BUFFER_SIZE];
uint8_t buffer_index = 0;
bool command_received = false;

/*-------------------------------------------------------------------------------------------*/
/*---------------------------串口接收中断 (对接电机通信)-------------------------------------*/
/*-------------------------------------------------------------------------------------------*/
// 这是 SysConfig 为 UART_MOTOR 生成的中断服务函数名
void UART_MOTOR_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_MOTOR_INST)) 
    {
        case DL_UART_MAIN_IIDX_RX:
            uint8_t rx_data = DL_UART_Main_receiveData(UART_MOTOR_INST);
            
            // 🌟 1. 严格帧头过滤：如果还没收到有效的帧头 0xC5，直接丢弃游离的垃圾数据
            if (g_rx_len == 0 && rx_data != 0xC5) {
                return; 
            }
            
            // 【新增】：防止数组越界卡死单片机
            if(g_rx_len < 128) {
                g_rx_cmd[g_rx_len++] = rx_data;
            } else {
                g_rx_len = 0; // 溢出强制清零
            }
            
            // 🌟 2. 修正帧尾判定为 0x5C (正点原子协议)
            // 同时增加最小长度约束(>=6字节)，防止数据有效载荷中偶然出现的 0x5C 导致提前断帧！
            if (g_rx_len >= 6 && rx_data == 0x5C) 
            {
                g_rx_frame_flag = true; 
            }
            break;
            
        default:
            break;
    }
}
// ================= 新增：电机2的串口中断服务函数 =================
void UART_MOTOR_2_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(UART_MOTOR_2_INST)) 
    {
        case DL_UART_MAIN_IIDX_RX:
            uint8_t rx_data = DL_UART_Main_receiveData(UART_MOTOR_2_INST);
            
            // 🌟 1. 严格帧头过滤：必须以 0xC5 开头
            if (g_rx2_len == 0 && rx_data != 0xC5) {
                return; 
            }
            
            // 越界保护
            if(g_rx2_len < 128) {
                g_rx2_cmd[g_rx2_len++] = rx_data;
            } else {
                g_rx2_len = 0; // 溢出清零
            }
            
            // 🌟 2. 修正帧尾判定为 0x5C，且加入最小长度约束
            if (g_rx2_len >= 6 && rx_data == 0x5C) {
                g_rx2_frame_flag = true; 
            }
            break;
            
        default:
            break;
    }
}

void UART_0_INST_IRQHandler(void)
{
    //如果产生了串口中断
    switch( DL_UART_getPendingInterrupt(UART_0_INST) )
    {
        case DL_UART_IIDX_RX://如果是接收中断
            
            // 1. 读取接收到的 1 个字节数据
            uart_data = DL_UART_Main_receiveData(UART_0_INST);
            
            // 2. 【核心】：喂给状态机解析！这步将唤醒你的步进电机！
            K230_Parse_Data(uart_data); 

            break;
        default://其他的串口中断
            break;
    }
}

// ================= 新增：专门给蓝牙 (UART_1) 发送 VOFA+ 数据的底层函数 =================
void JustFloat_SendArray_UART1(uint8_t *string, uint8_t length)
{
    while(length--)
    {
        DL_UART_Main_transmitDataBlocking(UART_1_INST, *string++);
    }
}

// 专为手机 App (如蓝牙调试器、微信小程序) 设计的通用波形发送函数 (CSV格式)
void Mobile_sendData_UART1(float a, float b, float c)
{
    char send_buf[64];
    
    const char* sign_a = (a < 0) ? "-" : "";
    if (a < 0) a = -a;
    int int_a = (int)a;
    int frac_a = (int)((a - int_a) * 100);
    
    const char* sign_b = (b < 0) ? "-" : "";
    if (b < 0) b = -b;
    int int_b = (int)b;
    int frac_b = (int)((b - int_b) * 100);
    
    const char* sign_c = (c < 0) ? "-" : "";
    if (c < 0) c = -c;
    int int_c = (int)c;
    int frac_c = (int)((c - int_c) * 100);
    
    sprintf(send_buf, "[plot,%s%d.%02d,%s%d.%02d,%s%d.%02d]", 
            sign_a, int_a, frac_a, 
            sign_b, int_b, frac_b, 
            sign_c, int_c, frac_c);
    
    char *p = send_buf;
    while (*p != '\0')
    {
        uart1_send_char(*p++);
    }
}

// 专门给蓝牙发 3 个通道波形的函数 (X, Y, Z 轴)
void vofa_sendData_UART1(float a, float b, float c)
{
    uint8_t byte[4] = {0};
    uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7f}; // VOFA+ 的 JustFloat 帧尾

    Float_to_Byte(a, byte); JustFloat_SendArray_UART1(byte, 4);    
    Float_to_Byte(b, byte); JustFloat_SendArray_UART1(byte, 4);    
    Float_to_Byte(c, byte); JustFloat_SendArray_UART1(byte, 4);
    
    JustFloat_SendArray_UART1(tail, 4); // 发送帧尾
}
