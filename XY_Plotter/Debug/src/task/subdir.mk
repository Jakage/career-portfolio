################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/task/ArmTask.cpp \
../src/task/BaseTask.cpp \
../src/task/PrinterTask.cpp \
../src/task/RITTask.cpp \
../src/task/USBTask.cpp 

OBJS += \
./src/task/ArmTask.o \
./src/task/BaseTask.o \
./src/task/PrinterTask.o \
./src/task/RITTask.o \
./src/task/USBTask.o 

CPP_DEPS += \
./src/task/ArmTask.d \
./src/task/BaseTask.d \
./src/task/PrinterTask.d \
./src/task/RITTask.d \
./src/task/USBTask.d 


# Each subdirectory must supply rules for building sources it contributes
src/task/%.o: ../src/task/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C++ Compiler'
	arm-none-eabi-c++ -std=c++11 -D__NEWLIB__ -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DCPP_USE_HEAP -D__LPC15XX__ -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\PlotterProject\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_board_nxp_lpcxpresso_1549\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_chip_15xx\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\freertos\inc" -I"C:\Users\akai__000\Documents\MCUXpressoIDE_10.0.2_411\workspace\lpc_chip_15xx\inc\usbd" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -mcpu=cortex-m3 -mthumb -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


