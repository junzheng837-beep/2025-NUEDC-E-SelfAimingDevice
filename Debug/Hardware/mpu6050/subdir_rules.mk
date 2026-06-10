################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Hardware/mpu6050/%.o: ../Hardware/mpu6050/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/TI_Files/DS_E02/middle" -I"E:/TI_Files/DS_E02/Hardware/mpu6050" -I"E:/TI_Files/DS_E02/Hardware" -I"E:/TI_Files/DS_E02/app" -I"E:/TI_Files/DS_E02" -I"E:/TI_Files/DS_E02/Debug" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source" -gdwarf-3 -MMD -MP -MF"Hardware/mpu6050/$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


