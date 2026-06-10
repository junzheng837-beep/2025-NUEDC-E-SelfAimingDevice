/*
 * Copyright (c) 2023, Texas Instruments Incorporated
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
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

DL_TimerA_backupConfig gPWM_0Backup;
DL_TimerG_backupConfig gTIMER_2Backup;
DL_TimerA_backupConfig gTIMER_MOTORBackup;
DL_UART_Main_backupConfig gUART_MOTORBackup;
DL_SPI_backupConfig gSPI_LCDBackup;

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_PWM_0_init();
    SYSCFG_DL_TIMER_0_init();
    SYSCFG_DL_TIMER_1_init();
    SYSCFG_DL_TIMER_2_init();
    SYSCFG_DL_TIMER_MOTOR_init();
    SYSCFG_DL_UART_0_init();
    SYSCFG_DL_UART_MOTOR_init();
    SYSCFG_DL_UART_MOTOR_2_init();
    SYSCFG_DL_UART_1_init();
    SYSCFG_DL_SPI_LCD_init();
    SYSCFG_DL_SYSCTL_CLK_init();
    /* Ensure backup structures have no valid state */
	gPWM_0Backup.backupRdy 	= false;
	gTIMER_2Backup.backupRdy 	= false;
	gTIMER_MOTORBackup.backupRdy 	= false;
	gUART_MOTORBackup.backupRdy 	= false;
	gSPI_LCDBackup.backupRdy 	= false;

}
/*
 * User should take care to save and restore register configuration in application.
 * See Retention Configuration section for more details.
 */
SYSCONFIG_WEAK bool SYSCFG_DL_saveConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_saveConfiguration(PWM_0_INST, &gPWM_0Backup);
	retStatus &= DL_TimerG_saveConfiguration(TIMER_2_INST, &gTIMER_2Backup);
	retStatus &= DL_TimerA_saveConfiguration(TIMER_MOTOR_INST, &gTIMER_MOTORBackup);
	retStatus &= DL_UART_Main_saveConfiguration(UART_MOTOR_INST, &gUART_MOTORBackup);
	retStatus &= DL_SPI_saveConfiguration(SPI_LCD_INST, &gSPI_LCDBackup);

    return retStatus;
}


SYSCONFIG_WEAK bool SYSCFG_DL_restoreConfiguration(void)
{
    bool retStatus = true;

	retStatus &= DL_TimerA_restoreConfiguration(PWM_0_INST, &gPWM_0Backup, false);
	retStatus &= DL_TimerG_restoreConfiguration(TIMER_2_INST, &gTIMER_2Backup, false);
	retStatus &= DL_TimerA_restoreConfiguration(TIMER_MOTOR_INST, &gTIMER_MOTORBackup, false);
	retStatus &= DL_UART_Main_restoreConfiguration(UART_MOTOR_INST, &gUART_MOTORBackup);
	retStatus &= DL_SPI_restoreConfiguration(SPI_LCD_INST, &gSPI_LCDBackup);

    return retStatus;
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_GPIO_reset(GPIOB);
    DL_TimerA_reset(PWM_0_INST);
    DL_TimerG_reset(TIMER_0_INST);
    DL_TimerG_reset(TIMER_1_INST);
    DL_TimerG_reset(TIMER_2_INST);
    DL_TimerA_reset(TIMER_MOTOR_INST);
    DL_UART_Main_reset(UART_0_INST);
    DL_UART_Main_reset(UART_MOTOR_INST);
    DL_UART_Main_reset(UART_MOTOR_2_INST);
    DL_UART_Main_reset(UART_1_INST);
    DL_SPI_reset(SPI_LCD_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_GPIO_enablePower(GPIOB);
    DL_TimerA_enablePower(PWM_0_INST);
    DL_TimerG_enablePower(TIMER_0_INST);
    DL_TimerG_enablePower(TIMER_1_INST);
    DL_TimerG_enablePower(TIMER_2_INST);
    DL_TimerA_enablePower(TIMER_MOTOR_INST);
    DL_UART_Main_enablePower(UART_0_INST);
    DL_UART_Main_enablePower(UART_MOTOR_INST);
    DL_UART_Main_enablePower(UART_MOTOR_2_INST);
    DL_UART_Main_enablePower(UART_1_INST);
    DL_SPI_enablePower(SPI_LCD_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralAnalogFunction(GPIO_HFXIN_IOMUX);
    DL_GPIO_initPeripheralAnalogFunction(GPIO_HFXOUT_IOMUX);

    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_0_C0_IOMUX,GPIO_PWM_0_C0_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_0_C0_PORT, GPIO_PWM_0_C0_PIN);
    DL_GPIO_initPeripheralOutputFunction(GPIO_PWM_0_C1_IOMUX,GPIO_PWM_0_C1_IOMUX_FUNC);
    DL_GPIO_enableOutput(GPIO_PWM_0_C1_PORT, GPIO_PWM_0_C1_PIN);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_0_IOMUX_TX, GPIO_UART_0_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_0_IOMUX_RX, GPIO_UART_0_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_MOTOR_IOMUX_TX, GPIO_UART_MOTOR_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_MOTOR_IOMUX_RX, GPIO_UART_MOTOR_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_MOTOR_2_IOMUX_TX, GPIO_UART_MOTOR_2_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_MOTOR_2_IOMUX_RX, GPIO_UART_MOTOR_2_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_UART_1_IOMUX_TX, GPIO_UART_1_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_UART_1_IOMUX_RX, GPIO_UART_1_IOMUX_RX_FUNC);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_LCD_IOMUX_SCLK, GPIO_SPI_LCD_IOMUX_SCLK_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_SPI_LCD_IOMUX_PICO, GPIO_SPI_LCD_IOMUX_PICO_FUNC);

    DL_GPIO_initDigitalOutput(BEEP_PIN_0_IOMUX);

    DL_GPIO_initDigitalOutputFeatures(LED_LED3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalOutputFeatures(LED_LED1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_DOWN,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(KEY_KEY3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Encoder_A_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Encoder_B_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Encoder_C_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Encoder_D_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(Motor_Ctrl_AIN1_IOMUX);

    DL_GPIO_initDigitalOutput(Motor_Ctrl_AIN2_IOMUX);

    DL_GPIO_initDigitalOutput(Motor_Ctrl_BIN1_IOMUX);

    DL_GPIO_initDigitalOutput(Motor_Ctrl_BIN2_IOMUX);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN1_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN2_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN3_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN4_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN5_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN6_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN7_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN8_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN9_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN10_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN11_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalInputFeatures(Huidu_IN12_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_initDigitalOutput(GPIO_LCD_PIN_RES_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_PIN_DC_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_PIN_CS_IOMUX);

    DL_GPIO_initDigitalOutput(GPIO_LCD_PIN_BLK_IOMUX);

    DL_GPIO_initDigitalOutputFeatures(IIC_Software_JYSCL_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalOutputFeatures(IIC_Software_JYSDA_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
		 DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);

    DL_GPIO_initDigitalOutput(STEP_MOTOR_M1_DIR_IOMUX);

    DL_GPIO_initDigitalOutput(STEP_MOTOR_M2_DIR_IOMUX);

    DL_GPIO_clearPins(GPIOA, Motor_Ctrl_AIN1_PIN |
		Motor_Ctrl_AIN2_PIN |
		STEP_MOTOR_M1_DIR_PIN |
		STEP_MOTOR_M2_DIR_PIN);
    DL_GPIO_setPins(GPIOA, IIC_Software_JYSCL_PIN |
		IIC_Software_JYSDA_PIN);
    DL_GPIO_enableOutput(GPIOA, Motor_Ctrl_AIN1_PIN |
		Motor_Ctrl_AIN2_PIN |
		IIC_Software_JYSCL_PIN |
		IIC_Software_JYSDA_PIN |
		STEP_MOTOR_M1_DIR_PIN |
		STEP_MOTOR_M2_DIR_PIN);
    DL_GPIO_setLowerPinsPolarity(GPIOA, DL_GPIO_PIN_15_EDGE_RISE_FALL);
    DL_GPIO_setUpperPinsPolarity(GPIOA, DL_GPIO_PIN_16_EDGE_RISE_FALL |
		DL_GPIO_PIN_17_EDGE_RISE_FALL |
		DL_GPIO_PIN_22_EDGE_RISE_FALL);
    DL_GPIO_clearInterruptStatus(GPIOA, Encoder_A_PIN |
		Encoder_B_PIN |
		Encoder_C_PIN |
		Encoder_D_PIN);
    DL_GPIO_enableInterrupt(GPIOA, Encoder_A_PIN |
		Encoder_B_PIN |
		Encoder_C_PIN |
		Encoder_D_PIN);
    DL_GPIO_clearPins(GPIOB, BEEP_PIN_0_PIN |
		LED_LED3_PIN |
		Motor_Ctrl_BIN1_PIN |
		Motor_Ctrl_BIN2_PIN |
		GPIO_LCD_PIN_RES_PIN |
		GPIO_LCD_PIN_DC_PIN |
		GPIO_LCD_PIN_BLK_PIN);
    DL_GPIO_setPins(GPIOB, LED_LED1_PIN |
		GPIO_LCD_PIN_CS_PIN);
    DL_GPIO_enableOutput(GPIOB, BEEP_PIN_0_PIN |
		LED_LED3_PIN |
		LED_LED1_PIN |
		Motor_Ctrl_BIN1_PIN |
		Motor_Ctrl_BIN2_PIN |
		GPIO_LCD_PIN_RES_PIN |
		GPIO_LCD_PIN_DC_PIN |
		GPIO_LCD_PIN_CS_PIN |
		GPIO_LCD_PIN_BLK_PIN);

}


static const DL_SYSCTL_SYSPLLConfig gSYSPLLConfig = {
    .inputFreq              = DL_SYSCTL_SYSPLL_INPUT_FREQ_32_48_MHZ,
	.rDivClk2x              = 1,
	.rDivClk1               = 0,
	.rDivClk0               = 0,
	.enableCLK2x            = DL_SYSCTL_SYSPLL_CLK2X_DISABLE,
	.enableCLK1             = DL_SYSCTL_SYSPLL_CLK1_DISABLE,
	.enableCLK0             = DL_SYSCTL_SYSPLL_CLK0_ENABLE,
	.sysPLLMCLK             = DL_SYSCTL_SYSPLL_MCLK_CLK0,
	.sysPLLRef              = DL_SYSCTL_SYSPLL_REF_HFCLK,
	.qDiv                   = 3,
	.pDiv                   = DL_SYSCTL_SYSPLL_PDIV_1
};
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);
    DL_SYSCTL_setFlashWaitState(DL_SYSCTL_FLASH_WAIT_STATE_2);

    
	DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
	/* Set default configuration */
	DL_SYSCTL_disableHFXT();
	DL_SYSCTL_disableSYSPLL();
    DL_SYSCTL_setHFCLKSourceHFXTParams(DL_SYSCTL_HFXT_RANGE_32_48_MHZ,10, false);
    DL_SYSCTL_configSYSPLL((DL_SYSCTL_SYSPLLConfig *) &gSYSPLLConfig);
    DL_SYSCTL_setULPCLKDivider(DL_SYSCTL_ULPCLK_DIV_2);
    DL_SYSCTL_setMCLKSource(SYSOSC, HSCLK, DL_SYSCTL_HSCLK_SOURCE_SYSPLL);

}
SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_CLK_init(void) {
    while ((DL_SYSCTL_getClockStatus() & (DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD
		 | DL_SYSCTL_CLK_STATUS_HFCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_HSCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_LFOSC_GOOD))
	       != (DL_SYSCTL_CLK_STATUS_SYSPLL_GOOD
		 | DL_SYSCTL_CLK_STATUS_HFCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_HSCLK_GOOD
		 | DL_SYSCTL_CLK_STATUS_LFOSC_GOOD))
	{
		/* Ensure that clocks are in default POR configuration before initialization.
		* Additionally once LFXT is enabled, the internal LFOSC is disabled, and cannot
		* be re-enabled other than by executing a BOOTRST. */
		;
	}
}



/*
 * Timer clock configuration to be sourced by  / 5 (16000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   4000000 Hz = 16000000 Hz / (5 * (3 + 1))
 */
static const DL_TimerA_ClockConfig gPWM_0ClockConfig = {
    .clockSel = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_5,
    .prescale = 3U
};

static const DL_TimerA_PWMConfig gPWM_0Config = {
    .pwmMode = DL_TIMER_PWM_MODE_EDGE_ALIGN_UP,
    .period = 10000,
    .isTimerWithFourCC = false,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_PWM_0_init(void) {

    DL_TimerA_setClockConfig(
        PWM_0_INST, (DL_TimerA_ClockConfig *) &gPWM_0ClockConfig);

    DL_TimerA_initPWMMode(
        PWM_0_INST, (DL_TimerA_PWMConfig *) &gPWM_0Config);

    // Set Counter control to the smallest CC index being used
    DL_TimerA_setCounterControl(PWM_0_INST,DL_TIMER_CZC_CCCTL0_ZCOND,DL_TIMER_CAC_CCCTL0_ACOND,DL_TIMER_CLC_CCCTL0_LCOND);

    DL_TimerA_setCaptureCompareOutCtl(PWM_0_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_0_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_0_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_0_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, DL_TIMER_CC_0_INDEX);

    DL_TimerA_setCaptureCompareOutCtl(PWM_0_INST, DL_TIMER_CC_OCTL_INIT_VAL_LOW,
		DL_TIMER_CC_OCTL_INV_OUT_DISABLED, DL_TIMER_CC_OCTL_SRC_FUNCVAL,
		DL_TIMERA_CAPTURE_COMPARE_1_INDEX);

    DL_TimerA_setCaptCompUpdateMethod(PWM_0_INST, DL_TIMER_CC_UPDATE_METHOD_IMMEDIATE, DL_TIMERA_CAPTURE_COMPARE_1_INDEX);
    DL_TimerA_setCaptureCompareValue(PWM_0_INST, 0, DL_TIMER_CC_1_INDEX);

    DL_TimerA_enableClock(PWM_0_INST);


    
    DL_TimerA_setCCPDirection(PWM_0_INST , DL_TIMER_CC0_OUTPUT | DL_TIMER_CC1_OUTPUT );


}



/*
 * Timer clock configuration to be sourced by BUSCLK /  (5000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   625000 Hz = 5000000 Hz / (8 * (7 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_0ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 7U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_0_INST_LOAD_VALUE = (1 ms * 625000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_0TimerConfig = {
    .period     = TIMER_0_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC_UP,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_0_init(void) {

    DL_TimerG_setClockConfig(TIMER_0_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_0ClockConfig);

    DL_TimerG_initTimerMode(TIMER_0_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_0TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_0_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(TIMER_0_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (5000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   50000 Hz = 5000000 Hz / (8 * (99 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_1ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 99U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_1_INST_LOAD_VALUE = (3 ms * 50000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_1TimerConfig = {
    .period     = TIMER_1_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_1_init(void) {

    DL_TimerG_setClockConfig(TIMER_1_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_1ClockConfig);

    DL_TimerG_initTimerMode(TIMER_1_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_1TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_1_INST , DL_TIMERG_INTERRUPT_ZERO_EVENT);
    DL_TimerG_enableClock(TIMER_1_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (10000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1250000 Hz = 10000000 Hz / (8 * (7 + 1))
 */
static const DL_TimerG_ClockConfig gTIMER_2ClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 7U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_2_INST_LOAD_VALUE = (0.01 ms * 1250000 Hz) - 1
 */
static const DL_TimerG_TimerConfig gTIMER_2TimerConfig = {
    .period     = TIMER_2_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_PERIODIC_UP,
    .startTimer = DL_TIMER_START,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_2_init(void) {

    DL_TimerG_setClockConfig(TIMER_2_INST,
        (DL_TimerG_ClockConfig *) &gTIMER_2ClockConfig);

    DL_TimerG_initTimerMode(TIMER_2_INST,
        (DL_TimerG_TimerConfig *) &gTIMER_2TimerConfig);
    DL_TimerG_enableInterrupt(TIMER_2_INST , DL_TIMERG_INTERRUPT_LOAD_EVENT);
    DL_TimerG_enableClock(TIMER_2_INST);





}

/*
 * Timer clock configuration to be sourced by BUSCLK /  (10000000 Hz)
 * timerClkFreq = (timerClkSrc / (timerClkDivRatio * (timerClkPrescale + 1)))
 *   1250000 Hz = 10000000 Hz / (8 * (7 + 1))
 */
static const DL_TimerA_ClockConfig gTIMER_MOTORClockConfig = {
    .clockSel    = DL_TIMER_CLOCK_BUSCLK,
    .divideRatio = DL_TIMER_CLOCK_DIVIDE_8,
    .prescale    = 7U,
};

/*
 * Timer load value (where the counter starts from) is calculated as (timerPeriod * timerClockFreq) - 1
 * TIMER_MOTOR_INST_LOAD_VALUE = (5 ms * 1250000 Hz) - 1
 */
static const DL_TimerA_TimerConfig gTIMER_MOTORTimerConfig = {
    .period     = TIMER_MOTOR_INST_LOAD_VALUE,
    .timerMode  = DL_TIMER_TIMER_MODE_ONE_SHOT,
    .startTimer = DL_TIMER_STOP,
};

SYSCONFIG_WEAK void SYSCFG_DL_TIMER_MOTOR_init(void) {

    DL_TimerA_setClockConfig(TIMER_MOTOR_INST,
        (DL_TimerA_ClockConfig *) &gTIMER_MOTORClockConfig);

    DL_TimerA_initTimerMode(TIMER_MOTOR_INST,
        (DL_TimerA_TimerConfig *) &gTIMER_MOTORTimerConfig);
    DL_TimerA_enableInterrupt(TIMER_MOTOR_INST , DL_TIMERA_INTERRUPT_ZERO_EVENT);
    DL_TimerA_enableClock(TIMER_MOTOR_INST);





}


static const DL_UART_Main_ClockConfig gUART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_0_init(void)
{
    DL_UART_Main_setClockConfig(UART_0_INST, (DL_UART_Main_ClockConfig *) &gUART_0ClockConfig);

    DL_UART_Main_init(UART_0_INST, (DL_UART_Main_Config *) &gUART_0Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_0_INST, UART_0_IBRD_40_MHZ_115200_BAUD, UART_0_FBRD_40_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_0_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);
    /* Setting the Interrupt Priority */
    NVIC_SetPriority(UART_0_INST_INT_IRQN, 0);


    DL_UART_Main_enable(UART_0_INST);
}
static const DL_UART_Main_ClockConfig gUART_MOTORClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_MOTORConfig = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_MOTOR_init(void)
{
    DL_UART_Main_setClockConfig(UART_MOTOR_INST, (DL_UART_Main_ClockConfig *) &gUART_MOTORClockConfig);

    DL_UART_Main_init(UART_MOTOR_INST, (DL_UART_Main_Config *) &gUART_MOTORConfig);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_MOTOR_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_MOTOR_INST, UART_MOTOR_IBRD_80_MHZ_115200_BAUD, UART_MOTOR_FBRD_80_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_MOTOR_INST,
                                 DL_UART_MAIN_INTERRUPT_RX |
                                 DL_UART_MAIN_INTERRUPT_TX);

    /* Configure FIFOs */
    DL_UART_Main_enableFIFOs(UART_MOTOR_INST);
    DL_UART_Main_setRXFIFOThreshold(UART_MOTOR_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(UART_MOTOR_INST, DL_UART_TX_FIFO_LEVEL_1_2_EMPTY);

    DL_UART_Main_enable(UART_MOTOR_INST);
}
static const DL_UART_Main_ClockConfig gUART_MOTOR_2ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_MOTOR_2Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_MOTOR_2_init(void)
{
    DL_UART_Main_setClockConfig(UART_MOTOR_2_INST, (DL_UART_Main_ClockConfig *) &gUART_MOTOR_2ClockConfig);

    DL_UART_Main_init(UART_MOTOR_2_INST, (DL_UART_Main_Config *) &gUART_MOTOR_2Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_MOTOR_2_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_MOTOR_2_INST, UART_MOTOR_2_IBRD_40_MHZ_115200_BAUD, UART_MOTOR_2_FBRD_40_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_MOTOR_2_INST,
                                 DL_UART_MAIN_INTERRUPT_RX |
                                 DL_UART_MAIN_INTERRUPT_TX);

    /* Configure FIFOs */
    DL_UART_Main_enableFIFOs(UART_MOTOR_2_INST);
    DL_UART_Main_setRXFIFOThreshold(UART_MOTOR_2_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(UART_MOTOR_2_INST, DL_UART_TX_FIFO_LEVEL_1_2_EMPTY);

    DL_UART_Main_enable(UART_MOTOR_2_INST);
}
static const DL_UART_Main_ClockConfig gUART_1ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gUART_1Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_UART_1_init(void)
{
    DL_UART_Main_setClockConfig(UART_1_INST, (DL_UART_Main_ClockConfig *) &gUART_1ClockConfig);

    DL_UART_Main_init(UART_1_INST, (DL_UART_Main_Config *) &gUART_1Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115190.78
     */
    DL_UART_Main_setOversampling(UART_1_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(UART_1_INST, UART_1_IBRD_40_MHZ_115200_BAUD, UART_1_FBRD_40_MHZ_115200_BAUD);


    /* Configure Interrupts */
    DL_UART_Main_enableInterrupt(UART_1_INST,
                                 DL_UART_MAIN_INTERRUPT_RX);

    /* Configure FIFOs */
    DL_UART_Main_enableFIFOs(UART_1_INST);
    DL_UART_Main_setRXFIFOThreshold(UART_1_INST, DL_UART_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(UART_1_INST, DL_UART_TX_FIFO_LEVEL_1_2_EMPTY);

    DL_UART_Main_enable(UART_1_INST);
}

static const DL_SPI_Config gSPI_LCD_config = {
    .mode        = DL_SPI_MODE_CONTROLLER,
    .frameFormat = DL_SPI_FRAME_FORMAT_MOTO3_POL0_PHA0,
    .parity      = DL_SPI_PARITY_NONE,
    .dataSize    = DL_SPI_DATA_SIZE_8,
    .bitOrder    = DL_SPI_BIT_ORDER_MSB_FIRST,
};

static const DL_SPI_ClockConfig gSPI_LCD_clockConfig = {
    .clockSel    = DL_SPI_CLOCK_BUSCLK,
    .divideRatio = DL_SPI_CLOCK_DIVIDE_RATIO_1
};

SYSCONFIG_WEAK void SYSCFG_DL_SPI_LCD_init(void) {
    DL_SPI_setClockConfig(SPI_LCD_INST, (DL_SPI_ClockConfig *) &gSPI_LCD_clockConfig);

    DL_SPI_init(SPI_LCD_INST, (DL_SPI_Config *) &gSPI_LCD_config);

    /* Configure Controller mode */
    /*
     * Set the bit rate clock divider to generate the serial output clock
     *     outputBitRate = (spiInputClock) / ((1 + SCR) * 2)
     *     40000000 = (80000000)/((1 + 0) * 2)
     */
    DL_SPI_setBitRateSerialClockDivider(SPI_LCD_INST, 0);
    /* Set RX and TX FIFO threshold levels */
    DL_SPI_setFIFOThreshold(SPI_LCD_INST, DL_SPI_RX_FIFO_LEVEL_1_2_FULL, DL_SPI_TX_FIFO_LEVEL_1_2_EMPTY);

    /* Enable module */
    DL_SPI_enable(SPI_LCD_INST);
}

