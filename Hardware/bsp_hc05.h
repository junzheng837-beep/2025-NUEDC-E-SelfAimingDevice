#ifndef __BSP_HC05_H__
#define __BSP_HC05_H__

#include "ti_msp_dl_config.h"
#include <string.h>

#define  BLERX_LEN_MAX  200

extern unsigned char BLERX_BUFF[BLERX_LEN_MAX];
extern unsigned char BLERX_FLAG;
extern unsigned char BLERX_LEN;

void Bluetooth_Init(void);
void Receive_Bluetooth_Data(void);
void BLE_send_String(unsigned char *str);
void Clear_BLERX_BUFF(void);

#endif
