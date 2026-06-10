/**
 ****************************************************************************************************
 * @file        process_frame.h
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2025-05-27
 * @brief       步进电机驱动器 数据解析
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 * 修改说明
 * V1.0 20250527
 * 第一次发布
 *
 ****************************************************************************************************
 */
#ifndef __PROCESS_FRAME_H
#define __PROCESS_FRAME_H

#include "stdbool.h"
#include "main.h"

/* 解析帧结构 */
typedef struct
{
    uint8_t slave_addr;      /* 从机地址 */
    uint8_t function_code;   /* 功能码 */
    uint8_t error_code;      /* 错误码 */
    uint8_t data[128];       /* 指令数据缓冲区 */
    uint8_t data_len;        /* 指令数据长度 */
    uint16_t checksum;       /* 校验和 */
} SERIAL_FRAME;

typedef enum
{
    ACK_SUCCEED                 = 0x01,     /* 应答成功 */
    ACK_FRAME_TOO_SHORT         = 0xE1,     /* 帧长度不足（小于最小帧长度） */
    ACK_INVALID_HEADER          = 0xE2,     /* 帧头错误（非0xC5） */
    ACK_INVALID_FOOTER          = 0xE3,     /* 帧尾错误（非0x5C） */
    ACK_CHECKSUM_MISMATCH       = 0xE4,     /* 校验和错误 */
    ACK_UNSUPPORTED_FUNCTION    = 0xE5,     /* 不支持的功能码 */
    ACK_ERR_ILLEGAL_VAL         = 0xE6      /* 数据不合法 */
} ACK_STA;


bool serial_frame_process(uint8_t *buffer, uint8_t len, SERIAL_FRAME *frame);

#endif
