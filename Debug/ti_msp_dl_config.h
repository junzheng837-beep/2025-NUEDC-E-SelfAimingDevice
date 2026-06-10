/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)


#define GPIO_HFXT_PORT                                                     GPIOA
#define GPIO_HFXIN_PIN                                             DL_GPIO_PIN_5
#define GPIO_HFXIN_IOMUX                                         (IOMUX_PINCM10)
#define GPIO_HFXOUT_PIN                                            DL_GPIO_PIN_6
#define GPIO_HFXOUT_IOMUX                                        (IOMUX_PINCM11)
#define CPUCLK_FREQ                                                     80000000



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMA1
#define PWM_0_INST_IRQHandler                                   TIMA1_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMA1_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                              4000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOB
#define GPIO_PWM_0_C0_PIN                                          DL_GPIO_PIN_4
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM17)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM17_PF_TIMA1_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOB
#define GPIO_PWM_0_C1_PIN                                          DL_GPIO_PIN_1
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM13)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM13_PF_TIMA1_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMG8)
#define TIMER_0_INST_IRQHandler                                 TIMG8_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMG8_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                           (624U)
/* Defines for TIMER_1 */
#define TIMER_1_INST                                                     (TIMG0)
#define TIMER_1_INST_IRQHandler                                 TIMG0_IRQHandler
#define TIMER_1_INST_INT_IRQN                                   (TIMG0_INT_IRQn)
#define TIMER_1_INST_LOAD_VALUE                                           (149U)
/* Defines for TIMER_2 */
#define TIMER_2_INST                                                     (TIMG7)
#define TIMER_2_INST_IRQHandler                                 TIMG7_IRQHandler
#define TIMER_2_INST_INT_IRQN                                   (TIMG7_INT_IRQn)
#define TIMER_2_INST_LOAD_VALUE                                            (12U)
/* Defines for TIMER_MOTOR */
#define TIMER_MOTOR_INST                                                 (TIMA0)
#define TIMER_MOTOR_INST_IRQHandler                             TIMA0_IRQHandler
#define TIMER_MOTOR_INST_INT_IRQN                               (TIMA0_INT_IRQn)
#define TIMER_MOTOR_INST_LOAD_VALUE                                      (6249U)



/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           40000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_0_FBRD_40_MHZ_115200_BAUD                                      (45)
/* Defines for UART_MOTOR */
#define UART_MOTOR_INST                                                    UART3
#define UART_MOTOR_INST_FREQUENCY                                       80000000
#define UART_MOTOR_INST_IRQHandler                              UART3_IRQHandler
#define UART_MOTOR_INST_INT_IRQN                                  UART3_INT_IRQn
#define GPIO_UART_MOTOR_RX_PORT                                            GPIOB
#define GPIO_UART_MOTOR_TX_PORT                                            GPIOB
#define GPIO_UART_MOTOR_RX_PIN                                    DL_GPIO_PIN_13
#define GPIO_UART_MOTOR_TX_PIN                                    DL_GPIO_PIN_12
#define GPIO_UART_MOTOR_IOMUX_RX                                 (IOMUX_PINCM30)
#define GPIO_UART_MOTOR_IOMUX_TX                                 (IOMUX_PINCM29)
#define GPIO_UART_MOTOR_IOMUX_RX_FUNC                  IOMUX_PINCM30_PF_UART3_RX
#define GPIO_UART_MOTOR_IOMUX_TX_FUNC                  IOMUX_PINCM29_PF_UART3_TX
#define UART_MOTOR_BAUD_RATE                                            (115200)
#define UART_MOTOR_IBRD_80_MHZ_115200_BAUD                                  (43)
#define UART_MOTOR_FBRD_80_MHZ_115200_BAUD                                  (26)
/* Defines for UART_MOTOR_2 */
#define UART_MOTOR_2_INST                                                  UART2
#define UART_MOTOR_2_INST_FREQUENCY                                     40000000
#define UART_MOTOR_2_INST_IRQHandler                            UART2_IRQHandler
#define UART_MOTOR_2_INST_INT_IRQN                                UART2_INT_IRQn
#define GPIO_UART_MOTOR_2_RX_PORT                                          GPIOA
#define GPIO_UART_MOTOR_2_TX_PORT                                          GPIOA
#define GPIO_UART_MOTOR_2_RX_PIN                                  DL_GPIO_PIN_24
#define GPIO_UART_MOTOR_2_TX_PIN                                  DL_GPIO_PIN_21
#define GPIO_UART_MOTOR_2_IOMUX_RX                               (IOMUX_PINCM54)
#define GPIO_UART_MOTOR_2_IOMUX_TX                               (IOMUX_PINCM46)
#define GPIO_UART_MOTOR_2_IOMUX_RX_FUNC                IOMUX_PINCM54_PF_UART2_RX
#define GPIO_UART_MOTOR_2_IOMUX_TX_FUNC                IOMUX_PINCM46_PF_UART2_TX
#define UART_MOTOR_2_BAUD_RATE                                          (115200)
#define UART_MOTOR_2_IBRD_40_MHZ_115200_BAUD                                (21)
#define UART_MOTOR_2_FBRD_40_MHZ_115200_BAUD                                (45)
/* Defines for UART_1 */
#define UART_1_INST                                                        UART1
#define UART_1_INST_FREQUENCY                                           40000000
#define UART_1_INST_IRQHandler                                  UART1_IRQHandler
#define UART_1_INST_INT_IRQN                                      UART1_INT_IRQn
#define GPIO_UART_1_RX_PORT                                                GPIOB
#define GPIO_UART_1_TX_PORT                                                GPIOB
#define GPIO_UART_1_RX_PIN                                         DL_GPIO_PIN_7
#define GPIO_UART_1_TX_PIN                                         DL_GPIO_PIN_6
#define GPIO_UART_1_IOMUX_RX                                     (IOMUX_PINCM24)
#define GPIO_UART_1_IOMUX_TX                                     (IOMUX_PINCM23)
#define GPIO_UART_1_IOMUX_RX_FUNC                      IOMUX_PINCM24_PF_UART1_RX
#define GPIO_UART_1_IOMUX_TX_FUNC                      IOMUX_PINCM23_PF_UART1_TX
#define UART_1_BAUD_RATE                                                (115200)
#define UART_1_IBRD_40_MHZ_115200_BAUD                                      (21)
#define UART_1_FBRD_40_MHZ_115200_BAUD                                      (45)




/* Defines for SPI_LCD */
#define SPI_LCD_INST                                                       SPI1
#define SPI_LCD_INST_IRQHandler                                 SPI1_IRQHandler
#define SPI_LCD_INST_INT_IRQN                                     SPI1_INT_IRQn
#define GPIO_SPI_LCD_PICO_PORT                                            GPIOB
#define GPIO_SPI_LCD_PICO_PIN                                     DL_GPIO_PIN_8
#define GPIO_SPI_LCD_IOMUX_PICO                                 (IOMUX_PINCM25)
#define GPIO_SPI_LCD_IOMUX_PICO_FUNC                 IOMUX_PINCM25_PF_SPI1_PICO
/* GPIO configuration for SPI_LCD */
#define GPIO_SPI_LCD_SCLK_PORT                                            GPIOB
#define GPIO_SPI_LCD_SCLK_PIN                                     DL_GPIO_PIN_9
#define GPIO_SPI_LCD_IOMUX_SCLK                                 (IOMUX_PINCM26)
#define GPIO_SPI_LCD_IOMUX_SCLK_FUNC                 IOMUX_PINCM26_PF_SPI1_SCLK



/* Port definition for Pin Group BEEP */
#define BEEP_PORT                                                        (GPIOB)

/* Defines for PIN_0: GPIOB.17 with pinCMx 43 on package pin 14 */
#define BEEP_PIN_0_PIN                                          (DL_GPIO_PIN_17)
#define BEEP_PIN_0_IOMUX                                         (IOMUX_PINCM43)
/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOB)

/* Defines for LED3: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED_LED3_PIN                                            (DL_GPIO_PIN_22)
#define LED_LED3_IOMUX                                           (IOMUX_PINCM50)
/* Defines for LED1: GPIOB.27 with pinCMx 58 on package pin 29 */
#define LED_LED1_PIN                                            (DL_GPIO_PIN_27)
#define LED_LED1_IOMUX                                           (IOMUX_PINCM58)
/* Defines for KEY1: GPIOB.21 with pinCMx 49 on package pin 20 */
#define KEY_KEY1_PORT                                                    (GPIOB)
#define KEY_KEY1_PIN                                            (DL_GPIO_PIN_21)
#define KEY_KEY1_IOMUX                                           (IOMUX_PINCM49)
/* Defines for KEY2: GPIOA.26 with pinCMx 59 on package pin 30 */
#define KEY_KEY2_PORT                                                    (GPIOA)
#define KEY_KEY2_PIN                                            (DL_GPIO_PIN_26)
#define KEY_KEY2_IOMUX                                           (IOMUX_PINCM59)
/* Defines for KEY3: GPIOA.25 with pinCMx 55 on package pin 26 */
#define KEY_KEY3_PORT                                                    (GPIOA)
#define KEY_KEY3_PIN                                            (DL_GPIO_PIN_25)
#define KEY_KEY3_IOMUX                                           (IOMUX_PINCM55)
/* Port definition for Pin Group Encoder */
#define Encoder_PORT                                                     (GPIOA)

/* Defines for A: GPIOA.15 with pinCMx 37 on package pin 8 */
// pins affected by this interrupt request:["A","B","C","D"]
#define Encoder_INT_IRQN                                        (GPIOA_INT_IRQn)
#define Encoder_INT_IIDX                        (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define Encoder_A_IIDX                                      (DL_GPIO_IIDX_DIO15)
#define Encoder_A_PIN                                           (DL_GPIO_PIN_15)
#define Encoder_A_IOMUX                                          (IOMUX_PINCM37)
/* Defines for B: GPIOA.16 with pinCMx 38 on package pin 9 */
#define Encoder_B_IIDX                                      (DL_GPIO_IIDX_DIO16)
#define Encoder_B_PIN                                           (DL_GPIO_PIN_16)
#define Encoder_B_IOMUX                                          (IOMUX_PINCM38)
/* Defines for C: GPIOA.17 with pinCMx 39 on package pin 10 */
#define Encoder_C_IIDX                                      (DL_GPIO_IIDX_DIO17)
#define Encoder_C_PIN                                           (DL_GPIO_PIN_17)
#define Encoder_C_IOMUX                                          (IOMUX_PINCM39)
/* Defines for D: GPIOA.22 with pinCMx 47 on package pin 18 */
#define Encoder_D_IIDX                                      (DL_GPIO_IIDX_DIO22)
#define Encoder_D_PIN                                           (DL_GPIO_PIN_22)
#define Encoder_D_IOMUX                                          (IOMUX_PINCM47)
/* Defines for AIN1: GPIOA.13 with pinCMx 35 on package pin 6 */
#define Motor_Ctrl_AIN1_PORT                                             (GPIOA)
#define Motor_Ctrl_AIN1_PIN                                     (DL_GPIO_PIN_13)
#define Motor_Ctrl_AIN1_IOMUX                                    (IOMUX_PINCM35)
/* Defines for AIN2: GPIOA.12 with pinCMx 34 on package pin 5 */
#define Motor_Ctrl_AIN2_PORT                                             (GPIOA)
#define Motor_Ctrl_AIN2_PIN                                     (DL_GPIO_PIN_12)
#define Motor_Ctrl_AIN2_IOMUX                                    (IOMUX_PINCM34)
/* Defines for BIN1: GPIOB.0 with pinCMx 12 on package pin 47 */
#define Motor_Ctrl_BIN1_PORT                                             (GPIOB)
#define Motor_Ctrl_BIN1_PIN                                      (DL_GPIO_PIN_0)
#define Motor_Ctrl_BIN1_IOMUX                                    (IOMUX_PINCM12)
/* Defines for BIN2: GPIOB.16 with pinCMx 33 on package pin 4 */
#define Motor_Ctrl_BIN2_PORT                                             (GPIOB)
#define Motor_Ctrl_BIN2_PIN                                     (DL_GPIO_PIN_16)
#define Motor_Ctrl_BIN2_IOMUX                                    (IOMUX_PINCM33)
/* Defines for IN1: GPIOB.18 with pinCMx 44 on package pin 15 */
#define Huidu_IN1_PORT                                                   (GPIOB)
#define Huidu_IN1_PIN                                           (DL_GPIO_PIN_18)
#define Huidu_IN1_IOMUX                                          (IOMUX_PINCM44)
/* Defines for IN2: GPIOB.19 with pinCMx 45 on package pin 16 */
#define Huidu_IN2_PORT                                                   (GPIOB)
#define Huidu_IN2_PIN                                           (DL_GPIO_PIN_19)
#define Huidu_IN2_IOMUX                                          (IOMUX_PINCM45)
/* Defines for IN3: GPIOB.15 with pinCMx 32 on package pin 3 */
#define Huidu_IN3_PORT                                                   (GPIOB)
#define Huidu_IN3_PIN                                           (DL_GPIO_PIN_15)
#define Huidu_IN3_IOMUX                                          (IOMUX_PINCM32)
/* Defines for IN4: GPIOA.14 with pinCMx 36 on package pin 7 */
#define Huidu_IN4_PORT                                                   (GPIOA)
#define Huidu_IN4_PIN                                           (DL_GPIO_PIN_14)
#define Huidu_IN4_IOMUX                                          (IOMUX_PINCM36)
/* Defines for IN5: GPIOB.20 with pinCMx 48 on package pin 19 */
#define Huidu_IN5_PORT                                                   (GPIOB)
#define Huidu_IN5_PIN                                           (DL_GPIO_PIN_20)
#define Huidu_IN5_IOMUX                                          (IOMUX_PINCM48)
/* Defines for IN6: GPIOB.23 with pinCMx 51 on package pin 22 */
#define Huidu_IN6_PORT                                                   (GPIOB)
#define Huidu_IN6_PIN                                           (DL_GPIO_PIN_23)
#define Huidu_IN6_IOMUX                                          (IOMUX_PINCM51)
/* Defines for IN7: GPIOB.24 with pinCMx 52 on package pin 23 */
#define Huidu_IN7_PORT                                                   (GPIOB)
#define Huidu_IN7_PIN                                           (DL_GPIO_PIN_24)
#define Huidu_IN7_IOMUX                                          (IOMUX_PINCM52)
/* Defines for IN8: GPIOB.25 with pinCMx 56 on package pin 27 */
#define Huidu_IN8_PORT                                                   (GPIOB)
#define Huidu_IN8_PIN                                           (DL_GPIO_PIN_25)
#define Huidu_IN8_IOMUX                                          (IOMUX_PINCM56)
/* Defines for IN9: GPIOA.27 with pinCMx 60 on package pin 31 */
#define Huidu_IN9_PORT                                                   (GPIOA)
#define Huidu_IN9_PIN                                           (DL_GPIO_PIN_27)
#define Huidu_IN9_IOMUX                                          (IOMUX_PINCM60)
/* Defines for IN10: GPIOA.28 with pinCMx 3 on package pin 35 */
#define Huidu_IN10_PORT                                                  (GPIOA)
#define Huidu_IN10_PIN                                          (DL_GPIO_PIN_28)
#define Huidu_IN10_IOMUX                                          (IOMUX_PINCM3)
/* Defines for IN11: GPIOA.31 with pinCMx 6 on package pin 39 */
#define Huidu_IN11_PORT                                                  (GPIOA)
#define Huidu_IN11_PIN                                          (DL_GPIO_PIN_31)
#define Huidu_IN11_IOMUX                                          (IOMUX_PINCM6)
/* Defines for IN12: GPIOA.9 with pinCMx 20 on package pin 55 */
#define Huidu_IN12_PORT                                                  (GPIOA)
#define Huidu_IN12_PIN                                           (DL_GPIO_PIN_9)
#define Huidu_IN12_IOMUX                                         (IOMUX_PINCM20)
/* Port definition for Pin Group GPIO_LCD */
#define GPIO_LCD_PORT                                                    (GPIOB)

/* Defines for PIN_RES: GPIOB.10 with pinCMx 27 on package pin 62 */
#define GPIO_LCD_PIN_RES_PIN                                    (DL_GPIO_PIN_10)
#define GPIO_LCD_PIN_RES_IOMUX                                   (IOMUX_PINCM27)
/* Defines for PIN_DC: GPIOB.11 with pinCMx 28 on package pin 63 */
#define GPIO_LCD_PIN_DC_PIN                                     (DL_GPIO_PIN_11)
#define GPIO_LCD_PIN_DC_IOMUX                                    (IOMUX_PINCM28)
/* Defines for PIN_CS: GPIOB.14 with pinCMx 31 on package pin 2 */
#define GPIO_LCD_PIN_CS_PIN                                     (DL_GPIO_PIN_14)
#define GPIO_LCD_PIN_CS_IOMUX                                    (IOMUX_PINCM31)
/* Defines for PIN_BLK: GPIOB.26 with pinCMx 57 on package pin 28 */
#define GPIO_LCD_PIN_BLK_PIN                                    (DL_GPIO_PIN_26)
#define GPIO_LCD_PIN_BLK_IOMUX                                   (IOMUX_PINCM57)
/* Port definition for Pin Group IIC_Software */
#define IIC_Software_PORT                                                (GPIOA)

/* Defines for JYSCL: GPIOA.1 with pinCMx 2 on package pin 34 */
#define IIC_Software_JYSCL_PIN                                   (DL_GPIO_PIN_1)
#define IIC_Software_JYSCL_IOMUX                                  (IOMUX_PINCM2)
/* Defines for JYSDA: GPIOA.0 with pinCMx 1 on package pin 33 */
#define IIC_Software_JYSDA_PIN                                   (DL_GPIO_PIN_0)
#define IIC_Software_JYSDA_IOMUX                                  (IOMUX_PINCM1)
/* Port definition for Pin Group STEP_MOTOR */
#define STEP_MOTOR_PORT                                                  (GPIOA)

/* Defines for M1_DIR: GPIOA.8 with pinCMx 19 on package pin 54 */
#define STEP_MOTOR_M1_DIR_PIN                                    (DL_GPIO_PIN_8)
#define STEP_MOTOR_M1_DIR_IOMUX                                  (IOMUX_PINCM19)
/* Defines for M2_DIR: GPIOA.7 with pinCMx 14 on package pin 49 */
#define STEP_MOTOR_M2_DIR_PIN                                    (DL_GPIO_PIN_7)
#define STEP_MOTOR_M2_DIR_IOMUX                                  (IOMUX_PINCM14)

/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_SYSCTL_CLK_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_TIMER_1_init(void);
void SYSCFG_DL_TIMER_2_init(void);
void SYSCFG_DL_TIMER_MOTOR_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_UART_MOTOR_init(void);
void SYSCFG_DL_UART_MOTOR_2_init(void);
void SYSCFG_DL_UART_1_init(void);
void SYSCFG_DL_SPI_LCD_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
