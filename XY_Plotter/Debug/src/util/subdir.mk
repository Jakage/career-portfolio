################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/util/StringUtil.cpp 

C_SRCS += \
../src/util/ITM_write.c \
../src/util/cdc_desc.c \
../src/util/cdc_main.c \
../src/util/cdc_vcom.c 

OBJS += \
./src/util/ITM_write.o \
./src/util/StringUtil.o \
./src/util/cdc_desc.o \
./src/util/cdc_main.o \
./src/util/cdc_vcom.o 

CPP_DEPS += \
./src/util/StringUtil.d 

C_DEPS += \
./src/util/ITM_write.d \
./src/util/cdc_desc.d \
./src/util/cdc_main.d \
./src/util/cdc_vcom.d 


# Each subdirectory must supply rules for building sources it contributes
src/util/%.o: ../src/util/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=c11 -D__NEWLIB__ -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC15XX__ -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\PlotterProject\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_board_nxp_lpcxpresso_1549\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_chip_15xx\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\freertos\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_chip_15xx\inc\usbd" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

src/util/%.o: ../src/util/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -std=c++11 -D__NEWLIB__ -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC15XX__ -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\PlotterProject\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_board_nxp_lpcxpresso_1549\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_chip_15xx\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\freertos\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_chip_15xx\inc\usbd" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


