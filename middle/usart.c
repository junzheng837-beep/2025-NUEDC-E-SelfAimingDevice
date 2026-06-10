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
            {
                uint8_t rx_data = DL_UART_Main_receiveData(UART_MOTOR_INST);
            
            // 1. 帧头过滤：若未接收到有效帧头 0xC5，则丢弃无效数据
            if (g_rx_len == 0 && rx_data != 0xC5) {
                return; 
            }
            
            // 防止接收缓冲区越界
            if(g_rx_len < 128) {
                g_rx_cmd[g_rx_len++] = rx_data;
            } else {
                g_rx_len = 0; // 溢出强制清零
            }
            
            // 2. 帧尾判定 (0x5C)
            // 增加最小长度约束，避免有效载荷中的数据被误判为帧尾
            if (g_rx_len >= 6 && rx_data == 0x5C) 
            {
                g_rx_frame_flag = true; 
            }
            break;
        }
            
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
            {
                uint8_t rx_data = DL_UART_Main_receiveData(UART_MOTOR_2_INST);
            
            // 1. 帧头过滤：校验起始字节 0xC5
            if (g_rx2_len == 0 && rx_data != 0xC5) {
                return; 
            }
            
            // 越界保护
            if(g_rx2_len < 128) {
                g_rx2_cmd[g_rx2_len++] = rx_data;
            } else {
                g_rx2_len = 0; // 溢出清零
            }
            
            // 2. 帧尾判定 (0x5C) 及长度约束
            if (g_rx2_len >= 6 && rx_data == 0x5C) {
                g_rx2_frame_flag = true; 
            }
            break;
        }
            
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
            
            // 2. 将数据交由状态机解析以触发后续动作
            K230_Parse_Data(uart_data); 

            break;
        default://其他的串口中断
            break;
    }
}

// 蓝牙 (UART_1) 数据底层发送函数
void JustFloat_SendArray_UART1(uint8_t *string, uint8_t length)
{
    while(length--)
    {
        DL_UART_Main_transmitDataBlocking(UART_1_INST, *string++);
    }
}

// 通用波形发送函数 (CSV格式，适配蓝牙调试器等移动端 App)
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

