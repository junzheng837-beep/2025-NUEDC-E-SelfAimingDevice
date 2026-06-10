################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
middle/%.o: ../middle/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/TI_Files/DS_E03_Show06 copy/middle" -I"E:/TI_Files/DS_E03_Show06 copy/Hardware/mpu6050" -I"E:/TI_Files/DS_E03_Show06 copy/Hardware" -I"E:/TI_Files/DS_E03_Show06 copy/app" -I"E:/TI_Files/DS_E03_Show06 copy" -I"E:/TI_Files/DS_E03_Show06 copy/Debug" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source" -gdwarf-3 -MMD -MP -MF"middle/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


