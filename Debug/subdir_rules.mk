################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/TI_Files/DS_E03_Show06/middle" -I"E:/TI_Files/DS_E03_Show06/Hardware/mpu6050" -I"E:/TI_Files/DS_E03_Show06/Hardware" -I"E:/TI_Files/DS_E03_Show06/app" -I"E:/TI_Files/DS_E03_Show06" -I"E:/TI_Files/DS_E03_Show06/Debug" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

build-664438861: ../empty.syscfg
	@echo 'Building file: "$<"'
	@echo 'Invoking: SysConfig'
	"E:/TI_02/CCS/sysconfig_1.24.0/sysconfig_cli.bat" -s "E:/TI_02/CCS/mspm0_sdk_2_05_01_00/.metadata/product.json" --script "E:/TI_Files/DS_E03_Show06/empty.syscfg" -o "." --compiler ticlang
	@echo 'Finished building: "$<"'
	@echo ' '

device_linker.cmd: build-664438861 ../empty.syscfg
device.opt: build-664438861
device.cmd.genlibs: build-664438861
ti_msp_dl_config.c: build-664438861
ti_msp_dl_config.h: build-664438861
Event.dot: build-664438861

%.o: ./%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/TI_Files/DS_E03_Show06/middle" -I"E:/TI_Files/DS_E03_Show06/Hardware/mpu6050" -I"E:/TI_Files/DS_E03_Show06/Hardware" -I"E:/TI_Files/DS_E03_Show06/app" -I"E:/TI_Files/DS_E03_Show06" -I"E:/TI_Files/DS_E03_Show06/Debug" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

startup_mspm0g350x_ticlang.o: E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source/ti/devices/msp/m0p/startup_system_files/ticlang/startup_mspm0g350x_ticlang.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"E:/TI_02/CCS/ccs/tools/compiler/ti-cgt-armllvm_4.0.2.LTS/bin/tiarmclang.exe" -c @"device.opt"  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -O0 -I"E:/TI_Files/DS_E03_Show06/middle" -I"E:/TI_Files/DS_E03_Show06/Hardware/mpu6050" -I"E:/TI_Files/DS_E03_Show06/Hardware" -I"E:/TI_Files/DS_E03_Show06/app" -I"E:/TI_Files/DS_E03_Show06" -I"E:/TI_Files/DS_E03_Show06/Debug" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source/third_party/CMSIS/Core/Include" -I"E:/TI_02/CCS/mspm0_sdk_2_05_01_00/source" -gdwarf-3 -MMD -MP -MF"$(basename $(<F)).d_raw" -MT"$(@)"  $(GEN_OPTS__FLAG) -o"$@" "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


