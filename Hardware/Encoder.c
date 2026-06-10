/***
	*******************************************************************************************************************************************************************
	* @file    Encoder.c
	* @version V2.1
	* @date    2024-7-22
	* @author  ����	
	* @brief   MSPM0G3507С��PID����ͨ��ģ��
   *************************************************************************************************
   *  @description
	*	
	*  ֻ��Ҫ����MEASURE_MOTORS_SPEED()��������Motor1_Speed��Motor2_SpeedΪ����ʵ���ٶ�
   *
>>>>> ����˵����δ��������������ת�����������״��룬���Ҷ����ҹ�����Ϊ���꣬�뱣�ֺ��Լ��ĳ��ģ��ڴˣ����������ҵĸ�л
	*************************************************************************************************************************************************************
***/
#include "Encoder.h"
// #include "mpu6050.h"
// #include "bsp_gyro.h"
#include "key.h"

int32_t Motor1_Encoder_Value=0;
int32_t Motor2_Encoder_Value=0;

float Motor1_Speed = 0;
float Motor2_Speed = 0;
//�ⲿ�ж϶�ȡ��������ֵ
void GROUP1_IRQHandler(void){
	if(DL_Interrupt_getStatusGroup(DL_INTERRUPT_GROUP_1,DL_INTERRUPT_GROUP1_GPIOA)){
		uint32_t Encoder_GPIO_Int = DL_GPIO_getEnabledInterruptStatus(Encoder_PORT,Encoder_A_PIN | Encoder_B_PIN | Encoder_C_PIN | Encoder_D_PIN);
		//ͨ��1 ����A��
		if ((Encoder_GPIO_Int & Encoder_A_PIN) == Encoder_A_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT, Encoder_A_PIN);
			if(Read_Encoder_A == 1){ //������
				if (Read_Encoder_B == 0){
					Motor1_Encoder_Value--;
				}
				else if(Read_Encoder_B == 1){
					Motor1_Encoder_Value++;
				}
			}
			else if(Read_Encoder_A == 0){//�½���
				if (Read_Encoder_B == 0){
					Motor1_Encoder_Value++;
				}
				else if(Read_Encoder_B == 1){
					Motor1_Encoder_Value--;
				}
			}
		}		
		//ͨ��2 ����B��
		if ((Encoder_GPIO_Int & Encoder_B_PIN) == Encoder_B_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT,Encoder_B_PIN);
			if(Read_Encoder_B == 1){ //������
				if (Read_Encoder_A == 0){
					Motor1_Encoder_Value++;
				}
				else if (Read_Encoder_A == 1){
					Motor1_Encoder_Value--;
				}
			}
			else if(Read_Encoder_B == 0){//�½���
				if (Read_Encoder_A == 0){
					Motor1_Encoder_Value--;
				}
				else if (Read_Encoder_A == 1){
					Motor1_Encoder_Value++;
				}
			}
		}
		//ͨ��3 ����A��		
		if ((Encoder_GPIO_Int & Encoder_C_PIN) == Encoder_C_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT, Encoder_C_PIN);
			if(Read_Encoder_C == 1){ //������
				if (Read_Encoder_D  == 0){
					Motor2_Encoder_Value--;
				}
				else if (Read_Encoder_D  == 1){
					Motor2_Encoder_Value++;
				}
			}
			else if(Read_Encoder_C == 0){//�½���
				if (Read_Encoder_D  == 0){
					Motor2_Encoder_Value++;
				}
				else if (Read_Encoder_D  == 1){
					Motor2_Encoder_Value--;
				}
			}
		}		
		//ͨ��4 ����B��
		if ((Encoder_GPIO_Int & Encoder_D_PIN) == Encoder_D_PIN){
			DL_GPIO_clearInterruptStatus(Encoder_PORT,Encoder_D_PIN);
			if(Read_Encoder_D == 1){ //������
				if (Read_Encoder_C  == 0){
					Motor2_Encoder_Value++;
				}
				else if (Read_Encoder_C  == 1){
					Motor2_Encoder_Value--;
				}
			}
			else if(Read_Encoder_D == 0){//�½���
				if (Read_Encoder_C  == 0){
					Motor2_Encoder_Value--;
				}
				else if (Read_Encoder_C  == 1){
					Motor2_Encoder_Value++;
				}
			}
		}
	}
}
//������1�����ٶ�
void Motor1_Get_Speed(void){
    short Encoder_TIM = 0;
    float Speed = 0;
    Encoder_TIM= Motor1_Encoder_Value;
    Motor1_Encoder_Value=0;
    Speed =(float)Encoder_TIM/(CC)*PI*RR;//�����ٶ�
    Motor1_Speed = Speed;
}
//������2�����ٶ�
void Motor2_Get_Speed(void){
    short Encoder_TIM = 0;
    float Speed = 0;
    Encoder_TIM= Motor2_Encoder_Value;
    Motor2_Encoder_Value=0;
    Speed =(float)Encoder_TIM/(CC)*PI*RR;//�����ٶ�
    Motor2_Speed = -Speed;
}
float Motor1_Lucheng,Motor2_Lucheng;
float Measure_Distance = 0;

// 计算当前小车行驶的里程
void MEASURE_MOTORS_SPEED(void){
	Motor1_Get_Speed();
    Motor2_Get_Speed();
	
	Motor1_Lucheng += Motor1_Speed*SAMPLE_TIME;//路程累计
	Motor2_Lucheng += Motor2_Speed*SAMPLE_TIME;//路程累计
	
    // 获取未经任何系数修饰的底层绝对平均里程
    float raw_distance = Motor1_Lucheng/2.0 + Motor2_Lucheng/2.0;
    
	// 🌟 核心修复：分段线性补偿 (解决起步打滑导致的非线性问题)
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
	
	if(Measure_Distance > 10000)
	{
		Measure_Distance = 0;
		Motor1_Lucheng = 0;
		Motor2_Lucheng = 0;
	}
}
