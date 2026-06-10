#include "bsp_hc05.h"
#include "usart.h" 
#include <stdio.h>

unsigned char ble_ring_buf[1024];
volatile uint16_t ble_ring_head = 0;
volatile uint16_t ble_ring_tail = 0;
volatile uint8_t ble_rx_idle_cnt = 0;

void BLE_Send_Bit(unsigned char ch)
{
    // 淇锛氫笉瑕佷娇鐢?DL_UART_isBusy锛屽洜涓哄畠鍦ㄦ帴鏀舵暟鎹椂涔熶細杩斿洖 true 瀵艰嚧姝婚攣鎸傝捣
    // 浣跨敤闃诲鍙戦€佸嚱鏁帮紝瀹冨唴閮ㄥ彧妫€娴嬪彂閫?FIFO 鏄惁宸叉弧
    DL_UART_Main_transmitDataBlocking(UART_1_INST, ch);
}

void BLE_send_String(unsigned char *str)
{
    while( str && *str ) 
    {
        BLE_Send_Bit(*str++);
    }
}

void Clear_BLERX_BUFF(void)
{
    BLERX_LEN = 0;
    BLERX_FLAG = 0;
}

void Bluetooth_Init(void)
{
    NVIC_ClearPendingIRQ(UART_1_INST_INT_IRQN);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
}

#include <stdlib.h> // For atof
#include "motor_ctrl.h" // For pid_Turn etc.

volatile uint8_t telemetry_enabled = 0; // 0: OFF, 1: ON

void Receive_Bluetooth_Data(void)
{
    // Used to remember loops
    static uint8_t v_loop = 0; // 0:M1, 1:M2, 2:Turn, 3:Angle, 4:Dist
    static uint8_t v_param = 0; // 0:Kp, 1:Ki, 2:Kd
    
    static unsigned char local_buf[128];
    static uint8_t local_len = 0;

    // Timeout logic
    extern volatile uint8_t ble_rx_idle_cnt;
    if (local_len > 0 && ble_rx_idle_cnt >= 20) {
        local_buf[local_len] = '\0';
        local_len = 0;
        ble_rx_idle_cnt = 0;
    }

    // Drain ring buffer
    while (ble_ring_tail != ble_ring_head) {
        ble_rx_idle_cnt = 0; // reset idle timeout
        char ch = ble_ring_buf[ble_ring_tail];
        ble_ring_tail = (ble_ring_tail + 1) % 1024;
        
        if (local_len < 127) {
            local_buf[local_len++] = ch;
        }
        
        if (ch == '\n' || ch == ']' || local_len >= 127) {
            local_buf[local_len] = '\0';

        // ================= 鑷姩鍚屾闈㈡澘涓庤皟鍙傚璞?=================
        extern uint8_t OLED_View_Select;
        if (OLED_View_Select == 2 || OLED_View_Select == 7) {
            v_loop = 2; // 浠匱urn
        } else if (OLED_View_Select == 5) {
            v_loop = 0; // 浠匨1
        } else if (OLED_View_Select == 6) {
            v_loop = 1; // 浠匨2
        } else if (OLED_View_Select == 8) {
            v_loop = 4; // 浠匘ist
        } else if (OLED_View_Select == 10) {
            v_loop = 3; // 浠匒ngle
        } else if (OLED_View_Select == 1) {
            // 鍦ㄦ祴璇曠湅鏉块〉锛寁_loop 鍙兘鏄?M1 鎴?M2锛岄槻姝㈣秺鐣?            if (v_loop != 0 && v_loop != 1) v_loop = 0;
        }
        extern uint8_t Tuning_Loop;
        Tuning_Loop = v_loop;
        // =======================================================

        // 浜や簰寮?PID 璋冨弬瑙ｆ瀽
        float val = 0.0f;
        char msg[64];
        if (strncmp((char *)local_buf, "TP=", 3) == 0 || strncmp((char *)local_buf, "tp=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Turn.Kp = val;
            sprintf(msg, "Turn Kp set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "TI=", 3) == 0 || strncmp((char *)local_buf, "ti=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Turn.Ki = val;
            sprintf(msg, "Turn Ki set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "TD=", 3) == 0 || strncmp((char *)local_buf, "td=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Turn.Kd = val;
            sprintf(msg, "Turn Kd set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        // Distance PID (DP, DI, DD)
        else if (strncmp((char *)local_buf, "DP=", 3) == 0 || strncmp((char *)local_buf, "dp=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Distance.Kp = val;
            sprintf(msg, "Dist Kp set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "DI=", 3) == 0 || strncmp((char *)local_buf, "di=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Distance.Ki = val;
            sprintf(msg, "Dist Ki set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "DD=", 3) == 0 || strncmp((char *)local_buf, "dd=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Distance.Kd = val;
            sprintf(msg, "Dist Kd set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        // Angle PID (AP, AI, AD) - Gyro Turn
        else if (strncmp((char *)local_buf, "AP=", 3) == 0 || strncmp((char *)local_buf, "ap=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Angle.Kp = val;
            sprintf(msg, "Angle Kp set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "AI=", 3) == 0 || strncmp((char *)local_buf, "ai=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Angle.Ki = val;
            sprintf(msg, "Angle Ki set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "AD=", 3) == 0 || strncmp((char *)local_buf, "ad=", 3) == 0) {
            val = atof((char *)&local_buf[3]);
            pid_Angle.Kd = val;
            sprintf(msg, "Angle Kd set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        // Motor 1 Speed PID (M1P, M1I, M1D)
        else if (strncmp((char *)local_buf, "M1P=", 4) == 0 || strncmp((char *)local_buf, "m1p=", 4) == 0) {
            val = atof((char *)&local_buf[4]);
            pid_Motor1_Speed.Kp = val;
            sprintf(msg, "M1 Kp set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "M1I=", 4) == 0 || strncmp((char *)local_buf, "m1i=", 4) == 0) {
            val = atof((char *)&local_buf[4]);
            pid_Motor1_Speed.Ki = val;
            sprintf(msg, "M1 Ki set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "M1D=", 4) == 0 || strncmp((char *)local_buf, "m1d=", 4) == 0) {
            val = atof((char *)&local_buf[4]);
            pid_Motor1_Speed.Kd = val;
            sprintf(msg, "M1 Kd set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        // Motor 2 Speed PID (M2P, M2I, M2D)
        else if (strncmp((char *)local_buf, "M2P=", 4) == 0 || strncmp((char *)local_buf, "m2p=", 4) == 0) {
            val = atof((char *)&local_buf[4]);
            pid_Motor2_Speed.Kp = val;
            sprintf(msg, "M2 Kp set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "M2I=", 4) == 0 || strncmp((char *)local_buf, "m2i=", 4) == 0) {
            val = atof((char *)&local_buf[4]);
            pid_Motor2_Speed.Ki = val;
            sprintf(msg, "M2 Ki set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        else if (strncmp((char *)local_buf, "M2D=", 4) == 0 || strncmp((char *)local_buf, "m2d=", 4) == 0) {
            val = atof((char *)&local_buf[4]);
            pid_Motor2_Speed.Kd = val;
            sprintf(msg, "M2 Kd set to: %d.%02d\r\n", (int)val, (int)(val*100)%100);
            BLE_send_String((unsigned char *)msg);
        }
        // 铏氭嫙鎸夐敭璋冨弬閫昏緫
        else if (strncmp((char *)local_buf, "[key,", 5) == 0) {
            char name[10] = {0};
            char action[10] = {0};
            if (sscanf((char *)local_buf, "[key,%9[^,],%9[^]]]", name, action) == 2) {
                if (strcmp(action, "down") == 0) {
                    
                    if (strcmp(name, "1") == 0) { // 鍒囨崲璋冭妭鐜?(闄愬埗鍦ㄩ潰鏉垮瓨鍦ㄧ殑PID鍐?
                        extern uint8_t OLED_View_Select;
                        if (OLED_View_Select == 1) {
                            v_loop = (v_loop == 0) ? 1 : 0; // 娴嬭瘯鐪嬫澘浠匨1鍜孧2
                        } else if (OLED_View_Select == 2) {
                            v_loop = 2; // 浠匱urn
                        } else if (OLED_View_Select == 5) {
                            v_loop = 0;
                        } else if (OLED_View_Select == 6) {
                            v_loop = 1;
                        } else if (OLED_View_Select == 7) {
                            v_loop = 2;
                        } else if (OLED_View_Select == 8) {
                            v_loop = 4;
                        } else if (OLED_View_Select == 10) {
                            v_loop = 3;
                        } else {
                            v_loop = (v_loop + 1) % 5;
                        }
                        extern uint8_t Tuning_Loop;
                        Tuning_Loop = v_loop;
                        v_param = 0; // 鍒囨崲鐜椂榛樿鍥炲埌Kp
                    } else if (strcmp(name, "2") == 0) {
                        v_param = (v_param + 1) % 3;
                    } else if (strcmp(name, "9") == 0) {
                        telemetry_enabled = !telemetry_enabled;
                        if (telemetry_enabled) {
                            BLE_send_String((unsigned char *)"Plot: ON\r\n");
                        } else {
                            BLE_send_String((unsigned char *)"Plot: OFF\r\n");
                        }
                    } else if (strcmp(name, "3") == 0 || strcmp(name, "4") == 0 ||
                               strcmp(name, "5") == 0 || strcmp(name, "6") == 0 ||
                               strcmp(name, "7") == 0 || strcmp(name, "8") == 0) {
                        float delta = 0;
                        if (strcmp(name, "3") == 0) delta = 0.05f;
                        else if (strcmp(name, "4") == 0) delta = -0.05f;
                        else if (strcmp(name, "5") == 0) delta = 1.0f;
                        else if (strcmp(name, "6") == 0) delta = -1.0f;
                        else if (strcmp(name, "7") == 0) delta = 5.0f;
                        else if (strcmp(name, "8") == 0) delta = -5.0f;
                        
                        pid_t *target_pid = NULL;
                        if (v_loop == 0) target_pid = &pid_Motor1_Speed;
                        else if (v_loop == 1) target_pid = &pid_Motor2_Speed;
                        else if (v_loop == 2) target_pid = &pid_Turn;
                        else if (v_loop == 3) target_pid = &pid_Angle;
                        else if (v_loop == 4) target_pid = &pid_Distance;
                        
                        if (target_pid) {
                            if (v_param == 0) {
                                target_pid->Kp += delta;
                                if (target_pid->Kp < 0.0f) target_pid->Kp = 0.0f;
                            }
                            else if (v_param == 1) {
                                target_pid->Ki += delta;
                                if (target_pid->Ki < 0.0f) target_pid->Ki = 0.0f;
                            }
                            else if (v_param == 2) {
                                target_pid->Kd += delta;
                                if (target_pid->Kd < 0.0f) target_pid->Kd = 0.0f;
                            }
                        }
                    }
                    
                    // 状态反馈
                    const char *loop_names[] = {"M1", "M2", "Turn", "Angle", "Dist"};
                    const char *param_names[] = {"Kp", "Ki", "Kd"};
                    pid_t *curr_pid = NULL;
                    if (v_loop == 0) curr_pid = &pid_Motor1_Speed;
                    else if (v_loop == 1) curr_pid = &pid_Motor2_Speed;
                    else if (v_loop == 2) curr_pid = &pid_Turn;
                    else if (v_loop == 3) curr_pid = &pid_Angle;
                    else if (v_loop == 4) curr_pid = &pid_Distance;
                    
                    float curr_val = 0;
                    if (v_param == 0) curr_val = curr_pid->Kp;
                    else if (v_param == 1) curr_val = curr_pid->Ki;
                    else if (v_param == 2) curr_val = curr_pid->Kd;
                    
                    extern volatile uint16_t telemetry_pause_ms;
                    telemetry_pause_ms = 3000; // 鏆傚仠娉㈠舰鍙戦€?绉掞紝浠ュ厤鍒峰睆
                    
                    sprintf(msg, "Sel: %s %s = %d.%02d\r\n", loop_names[v_loop], param_names[v_param], (int)curr_val, abs((int)(curr_val*100)%100));
                    BLE_send_String((unsigned char *)msg);
                }
            }
        }
        // 瑙ｆ瀽婊戞潌鎸囦护锛屾牸寮忎负 [slider,ID,VALUE]
        else if (strncmp((char *)local_buf, "[slider,", 8) == 0) {
            char slider_id[10] = {0};
            char slider_val_str[20] = {0};
            if (sscanf((char *)local_buf, "[slider,%9[^,],%19[^]]]", slider_id, slider_val_str) == 2) {
                float slider_val = atof(slider_val_str);
                
                pid_t *target_pid = NULL;
                if (v_loop == 0) target_pid = &pid_Motor1_Speed;
                else if (v_loop == 1) target_pid = &pid_Motor2_Speed;
                else if (v_loop == 2) target_pid = &pid_Turn;
                else if (v_loop == 3) target_pid = &pid_Angle;
                else if (v_loop == 4) target_pid = &pid_Distance;
                
                if (target_pid) {
                    extern uint8_t Tuning_Mode;
                    extern uint8_t Tuning_Cursor;
                    
                    if (strcmp(slider_id, "1") == 0 || strstr(slider_id, "p") || strstr(slider_id, "P") || strstr(slider_id, "1")) {
                        target_pid->Kp = slider_val;
                        v_param = 0; // 同步屏幕上的黄色高亮光标
                        Tuning_Cursor = 0;
                        Tuning_Mode = 1;
                    }
                    else if (strcmp(slider_id, "2") == 0 || strstr(slider_id, "i") || strstr(slider_id, "I") || strstr(slider_id, "2")) {
                        target_pid->Ki = slider_val;
                        v_param = 1;
                        Tuning_Cursor = 1;
                        Tuning_Mode = 1;
                    }
                    else if (strcmp(slider_id, "3") == 0 || strstr(slider_id, "d") || strstr(slider_id, "D") || strstr(slider_id, "3")) {
                        target_pid->Kd = slider_val;
                        v_param = 2;
                        Tuning_Cursor = 2;
                        Tuning_Mode = 1;
                    }
                    else {
                        // 濡傛灉娌℃湁浣跨敤鐗瑰畾鐨?1, 2, 3 ID锛屽垯鎸夊師閫昏緫鍩轰簬褰撳墠 v_param 淇敼
                        if (v_param == 0) {
                            target_pid->Kp = slider_val;
                        }
                        else if (v_param == 1) {
                            target_pid->Ki = slider_val;
                        }
                        else if (v_param == 2) {
                            target_pid->Kd = slider_val;
                        }
                    }
                }
                
                // 鍥犱负婊戞潌鎷栧姩鏃朵細浜х敓娴烽噺楂橀鏁版嵁锛屽鏋滄瘡娆￠兘鍥炲浼氱灛闂存尋鐖嗚摑鐗欏彂閫侀€氶亾
                // 鎵€浠ュ湪杩欓噷涓嶅洖澶?Sel 鏂囨湰锛屽彧榛橀粯淇敼鍙傛暟骞堕噸缃殏鍋滆鏃跺櫒
                extern volatile uint16_t telemetry_pause_ms;
                telemetry_pause_ms = 3000; // 鏆傚仠娉㈠舰鍙戦€?绉掞紝浠ュ厤鍒峰睆
            }
        }
        else {
            // empty
        }
        local_len = 0;
        local_buf[0] = '\0';
    }
}
}

void UART_1_INST_IRQHandler(void)
{
    switch (DL_UART_getPendingInterrupt(UART_1_INST)) {
        case DL_UART_IIDX_RX:
        case DL_UART_IIDX_RX_TIMEOUT_ERROR:
        {
            while (!DL_UART_isRXFIFOEmpty(UART_1_INST)) {
                uint8_t ch = DL_UART_Main_receiveData(UART_1_INST);
                uint16_t next_head = (ble_ring_head + 1) % 1024;
                if (next_head != ble_ring_tail) {
                    ble_ring_buf[ble_ring_head] = ch;
                    ble_ring_head = next_head;
                }
            }
            break;
        }
        
        case DL_UART_IIDX_OVERRUN_ERROR:
        case DL_UART_IIDX_BREAK_ERROR:
        case DL_UART_IIDX_PARITY_ERROR:
        case DL_UART_IIDX_FRAMING_ERROR:
        case DL_UART_IIDX_NOISE_ERROR:
            // Clear all error flags to prevent the IRQ from looping infinitely and freezing the system
            DL_UART_clearInterruptStatus(UART_1_INST, DL_UART_INTERRUPT_OVERRUN_ERROR |
                                                      DL_UART_INTERRUPT_BREAK_ERROR |
                                                      DL_UART_INTERRUPT_PARITY_ERROR |
                                                      DL_UART_INTERRUPT_FRAMING_ERROR |
                                                      DL_UART_INTERRUPT_NOISE_ERROR);
            {
                volatile uint8_t dump = DL_UART_Main_receiveData(UART_1_INST);
                (void)dump;
            }
            while (!DL_UART_isRXFIFOEmpty(UART_1_INST)) {
                DL_UART_Main_receiveData(UART_1_INST); // flush broken data
            }
            break;

        default:
            DL_UART_clearInterruptStatus(UART_1_INST, 0xFFFFFFFF);
            break;
    }
}

