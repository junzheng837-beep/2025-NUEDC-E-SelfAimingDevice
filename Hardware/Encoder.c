#include "Encoder.h"
#include "bsp_sr04.h"
#include "timer.h"

volatile uint32_t motor1_latest_pulse_micros = 0;
volatile uint32_t motor2_latest_pulse_micros = 0;
int32_t Motor1_Encoder_Value = 0;
int32_t Motor2_Encoder_Value = 0;

float Motor1_Speed = 0;
float Motor2_Speed = 0;
float Motor1_Lucheng = 0;
float Motor2_Lucheng = 0;
float Measure_Distance = 0;

// 外部中断读取编码器脉冲值
void GROUP1_IRQHandler(void){
	if(DL_Interrupt_getStatusGroup(DL_INTERRUPT_GROUP_1,DL_INTERRUPT_GROUP1_GPIOA)){
		uint32_t Encoder_GPIO_Int = DL_GPIO_getEnabledInterruptStatus(GPIOA, 0xFFFFFFFF);
        
        // --- SR04 Hook ---
        if ((Encoder_GPIO_Int & SR04_ECHO_PIN) == SR04_ECHO_PIN){
            DL_GPIO_clearInterruptStatus(SR04_PORT, SR04_ECHO_PIN);
            SR04_ECHO_IRQHandler();
        }
        // -----------------
		
		// 通道1 原左轮A相（现映射为右轮A相，即 Motor2）
		if ((Encoder_GPIO_Int & Encoder_A_PIN) == Encoder_A_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT, Encoder_A_PIN);
			if(Read_Encoder_A == 1){ // 上升沿
				if (Read_Encoder_B == 0){
					Motor2_Encoder_Value++;
				}
				else if(Read_Encoder_B == 1){
					Motor2_Encoder_Value--;
				}
			}
			else if(Read_Encoder_A == 0){ // 下降沿
				if (Read_Encoder_B == 0){
					Motor2_Encoder_Value--;
				}
				else if(Read_Encoder_B == 1){
					Motor2_Encoder_Value++;
				}
			}
            motor2_latest_pulse_micros = get_micros();
		}		
		
		// 通道2 原左轮B相（现映射为右轮B相，即 Motor2）
		if ((Encoder_GPIO_Int & Encoder_B_PIN) == Encoder_B_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT,Encoder_B_PIN);
			if(Read_Encoder_B == 1){ // 上升沿
				if (Read_Encoder_A == 0){
					Motor2_Encoder_Value--;
				}
				else if (Read_Encoder_A == 1){
					Motor2_Encoder_Value++;
				}
			}
			else if(Read_Encoder_B == 0){ // 下降沿
				if (Read_Encoder_A == 0){
					Motor2_Encoder_Value++;
				}
				else if (Read_Encoder_A == 1){
					Motor2_Encoder_Value--;
				}
			}
            motor2_latest_pulse_micros = get_micros();
		}
		
		// 通道3 原右轮A相（现映射为左轮A相，即 Motor1）
		if ((Encoder_GPIO_Int & Encoder_C_PIN) == Encoder_C_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT, Encoder_C_PIN);
			if(Read_Encoder_C == 1){ // 上升沿
				if (Read_Encoder_D  == 0){
					Motor1_Encoder_Value++;
				}
				else if (Read_Encoder_D  == 1){
					Motor1_Encoder_Value--;
				}
			}
			else if(Read_Encoder_C == 0){ // 下降沿
				if (Read_Encoder_D  == 0){
					Motor1_Encoder_Value--;
				}
				else if (Read_Encoder_D  == 1){
					Motor1_Encoder_Value++;
				}
			}
            motor1_latest_pulse_micros = get_micros();
		}		
		
		// 通道4 原右轮B相（现映射为左轮B相，即 Motor1）
		if ((Encoder_GPIO_Int & Encoder_D_PIN) == Encoder_D_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT,Encoder_D_PIN);
			if(Read_Encoder_D == 1){ // 上升沿
				if (Read_Encoder_C  == 0){
					Motor1_Encoder_Value--;
				}
				else if (Read_Encoder_C  == 1){
					Motor1_Encoder_Value++;
				}
			}
			else if(Read_Encoder_D == 0){ // 下降沿
				if (Read_Encoder_C  == 0){
					Motor1_Encoder_Value++;
				}
				else if (Read_Encoder_C  == 1){
					Motor1_Encoder_Value--;
				}
			}
            motor1_latest_pulse_micros = get_micros();
		}
	}
}

uint32_t motor1_prev_pulse_micros = 0;
uint32_t motor2_prev_pulse_micros = 0;

// 计算左轮当前速度 (M/T法)
void Motor1_Get_Speed(void){
    short Encoder_TIM = Motor1_Encoder_Value;
    Motor1_Encoder_Value = 0;
    
    if (Encoder_TIM == 0) {
        Motor1_Speed = 0.0f;
        motor1_prev_pulse_micros = get_micros();
    } else {
        uint32_t delta_t = motor1_latest_pulse_micros - motor1_prev_pulse_micros;
        motor1_prev_pulse_micros = motor1_latest_pulse_micros;
        
        if (delta_t > 0) {
            float distance = (float)Encoder_TIM * (PI * RR) / (ENCODE_13X * JIANSUBI * BEIPIN);
            float raw_speed = (distance * 1000000.0f) / (float)delta_t;
            
            // 滑动平均滤波，消除低线数编码器磁环不均匀导致的物理空间误差
            static float history[4] = {0};
            static uint8_t idx = 0;
            history[idx] = raw_speed;
            idx = (idx + 1) % 4;
            Motor1_Speed = (history[0] + history[1] + history[2] + history[3]) / 4.0f;
        } else {
            Motor1_Speed = 0.0f;
        }
    }
}

// 计算右轮当前速度 (M/T法)
void Motor2_Get_Speed(void){
    short Encoder_TIM = Motor2_Encoder_Value;
    Motor2_Encoder_Value = 0;
    
    if (Encoder_TIM == 0) {
        Motor2_Speed = 0.0f;
        motor2_prev_pulse_micros = get_micros();
    } else {
        uint32_t delta_t = motor2_latest_pulse_micros - motor2_prev_pulse_micros;
        motor2_prev_pulse_micros = motor2_latest_pulse_micros;
        
        if (delta_t > 0) {
            float distance = (float)Encoder_TIM * (PI * RR) / (ENCODE_13X * JIANSUBI * BEIPIN);
            // 右轮速度反向
            float raw_speed = -(distance * 1000000.0f) / (float)delta_t;
            
            // 滑动平均滤波
            static float history[4] = {0};
            static uint8_t idx = 0;
            history[idx] = raw_speed;
            idx = (idx + 1) % 4;
            Motor2_Speed = (history[0] + history[1] + history[2] + history[3]) / 4.0f;
        } else {
            Motor2_Speed = 0.0f;
        }
    }
}

// 计算当前小车行驶的里程
void MEASURE_MOTORS_SPEED(void){
	Motor1_Get_Speed();
    Motor2_Get_Speed();
	
	Motor1_Lucheng += Motor1_Speed*SAMPLE_TIME; // 路程累计
	Motor2_Lucheng += Motor2_Speed*SAMPLE_TIME; // 路程累计
	
    // 获取未经任何系数修饰的底层绝对平均里程
    float raw_distance = Motor1_Lucheng/2.0f + Motor2_Lucheng/2.0f;
    
	// 分段线性补偿 (解决起步打滑导致的非线性问题)
    if (raw_distance <= 217.0f) 
    {
        // 【第一圈】底层 0 ~ 217 映射到物理 0 ~ 314cm
        Measure_Distance = raw_distance * (314.0f / 217.0f); 
    } 
    else 
    {
        // 【第二圈】超出第一圈的部分，按第二圈的比例 (314 / 191) 进行放大
        Measure_Distance = 314.0f + (raw_distance - 217.0f) * (314.0f / 191.0f);
    }
	
	if(Measure_Distance > 10000.0f)
	{
		Measure_Distance = 0;
		Motor1_Lucheng = 0;
		Motor2_Lucheng = 0;
	}
}
