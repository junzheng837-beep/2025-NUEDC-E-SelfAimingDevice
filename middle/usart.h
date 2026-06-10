#ifndef __USART_H__
#define __USART_H__

#include "main.h"

extern uint8_t uart_data;
#define RX_BUFFER_SIZE   128

extern uint8_t uart1_data;
extern volatile bool g_rx_uart1_flag;



void UART_MOTOR_INST_IRQHandler(void);

//串口发送单个字符
void uart0_send_char(char ch);
void uart1_send_char(char ch);
//串口发送字符串
void uart0_send_string(char* str);
void uart1_send_string(char* str);
void usart0_send_byte(unsigned char byte);
void usart0_send_bytes(unsigned char *buf, int len);
void usart1_send_byte(unsigned char byte);


void JustFloat_SendArray(uint8_t *string,uint8_t length);
/*将浮点数f转化为4个字节数据存放在byte[4]中*/
void Float_to_Byte(float f,unsigned char byte[]);

void Mobile_sendData_UART1(float a, float b, float c);

int LOG_Debug_Out(const char* __file, const char* __func, int __line, const char* format, ...);

#define LOG_D(fmt, ...) \
    do { \
        LOG_Debug_Out(__FILE__, (const char*)__func__, __LINE__, fmt, ##__VA_ARGS__); \
    } while (0)



/* 使用可变参数是实现的类printf函数 */

uint8_t HAL_UART_Transmit( uint8_t *pData, uint16_t Size, uint32_t Timeout);















#endif
