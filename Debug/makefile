################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

CG_TOOL_ROOT := E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS

GEN_OPTS__FLAG := @"./device.opt" 
GEN_CMDS__FLAG := -Wl,-l"./device_linker.cmd" 

ORDERED_OBJS += \
"./empty.o" \
"./ti_msp_dl_config.o" \
"./startup_mspm0g350x_ticlang.o" \
"./Hardware/Encoder.o" \
"./Hardware/bsp_gyro.o" \
"./Hardware/bsp_hc05.o" \
"./Hardware/gw_gray.o" \
"./Hardware/hw_lcd.o" \
"./Hardware/hw_spi.o" \
"./Hardware/key.o" \
"./Hardware/motor_ctrl.o" \
"./Hardware/process_frame.o" \
"./Hardware/protocol.o" \
"./Hardware/smd.o" \
"./Hardware/task.o" \
"./app/app_lcd.o" \
"./app/app_protocol.o" \
"./middle/delay.o" \
"./middle/timer.o" \
"./middle/usart.o" \
$(GEN_CMDS__FLAG) \
-Wl,-ldevice.cmd.genlibs \
-Wl,-llibc.a \

-include ../makefile.init

RM := DEL /F
RMDIR := RMDIR /S/Q

# All of the sources participating in the build are defined here
-include sources.mk
-include subdir_vars.mk
-include Hardware/subdir_vars.mk
-include app/subdir_vars.mk
-include middle/subdir_vars.mk
-include subdir_rules.mk
-include Hardware/subdir_rules.mk
-include app/subdir_rules.mk
-include middle/subdir_rules.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C55_DEPS)),)
-include $(C55_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(S67_DEPS)),)
-include $(S67_DEPS)
endif
ifneq ($(strip $(S62_DEPS)),)
-include $(S62_DEPS)
endif
ifneq ($(strip $(S_DEPS)),)
-include $(S_DEPS)
endif
ifneq ($(strip $(OPT_DEPS)),)
-include $(OPT_DEPS)
endif
ifneq ($(strip $(C??_DEPS)),)
-include $(C??_DEPS)
endif
ifneq ($(strip $(ASM_UPPER_DEPS)),)
-include $(ASM_UPPER_DEPS)
endif
ifneq ($(strip $(S??_DEPS)),)
-include $(S??_DEPS)
endif
ifneq ($(strip $(C64_DEPS)),)
-include $(C64_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(S64_DEPS)),)
-include $(S64_DEPS)
endif
ifneq ($(strip $(CLA_DEPS)),)
-include $(CLA_DEPS)
endif
ifneq ($(strip $(S55_DEPS)),)
-include $(S55_DEPS)
endif
ifneq ($(strip $(SV7A_DEPS)),)
-include $(SV7A_DEPS)
endif
ifneq ($(strip $(C62_DEPS)),)
-include $(C62_DEPS)
endif
ifneq ($(strip $(C67_DEPS)),)
-include $(C67_DEPS)
endif
ifneq ($(strip $(K_DEPS)),)
-include $(K_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C43_DEPS)),)
-include $(C43_DEPS)
endif
ifneq ($(strip $(S43_DEPS)),)
-include $(S43_DEPS)
endif
ifneq ($(strip $(ASM_DEPS)),)
-include $(ASM_DEPS)
endif
ifneq ($(strip $(S_UPPER_DEPS)),)
-include $(S_UPPER_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(SA_DEPS)),)
-include $(SA_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 
EXE_OUTPUTS += \
DS_E03_Show06\ copy.out 

EXE_OUTPUTS__QUOTED += \
"DS_E03_Show06 copy.out" 


# All Target
all: $(OBJS) $(GEN_CMDS)
	@$(MAKE) --no-print-directory -Onone "DS_E03_Show06 copy.out"

# Tool invocations
DS_E03_Show06\ copy.out: $(OBJS) $(GEN_CMDS)
	@echo 'Building target: "$@"'
	@echo 'Invoking: Arm Linker'
	"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/bin/tiarmclang.exe" @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -gdwarf-3 -Wl,-m"DS_E03_Show06 copy.map" -Wl,-i"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source" -Wl,-i"E:/TI_Files/DS_E03_Show06 copy" -Wl,-i"E:/TI_Files/DS_E03_Show06 copy/Debug/syscfg" -Wl,-i"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/lib" -Wl,--diag_wrap=off -Wl,--display_error_number -Wl,--warn_sections -Wl,--xml_link_info="DS_E03_Show06 copy_linkInfo.xml" -Wl,--rom_model -o "DS_E03_Show06 copy.out" $(ORDERED_OBJS)
	@echo 'Finished building target: "$@"'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(GEN_MISC_FILES__QUOTED)$(GEN_FILES__QUOTED)$(EXE_OUTPUTS__QUOTED)
	-$(RM) "empty.o" "ti_msp_dl_config.o" "startup_mspm0g350x_ticlang.o" "Hardware\Encoder.o" "Hardware\bsp_gyro.o" "Hardware\bsp_hc05.o" "Hardware\gw_gray.o" "Hardware\hw_lcd.o" "Hardware\hw_spi.o" "Hardware\key.o" "Hardware\motor_ctrl.o" "Hardware\process_frame.o" "Hardware\protocol.o" "Hardware\smd.o" "Hardware\task.o" "app\app_lcd.o" "app\app_protocol.o" "middle\delay.o" "middle\timer.o" "middle\usart.o" 
	-$(RM) "empty.d" "ti_msp_dl_config.d" "startup_mspm0g350x_ticlang.d" "Hardware\Encoder.d" "Hardware\bsp_gyro.d" "Hardware\bsp_hc05.d" "Hardware\gw_gray.d" "Hardware\hw_lcd.d" "Hardware\hw_spi.d" "Hardware\key.d" "Hardware\motor_ctrl.d" "Hardware\process_frame.d" "Hardware\protocol.d" "Hardware\smd.d" "Hardware\task.d" "app\app_lcd.d" "app\app_protocol.d" "middle\delay.d" "middle\timer.d" "middle\usart.d" 
	-@echo 'Finished clean'
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets

