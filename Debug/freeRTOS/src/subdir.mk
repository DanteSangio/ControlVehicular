################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../freeRTOS/src/FreeRTOSCommonHooks.c \
../freeRTOS/src/croutine.c \
../freeRTOS/src/event_groups.c \
../freeRTOS/src/heap_3.c \
../freeRTOS/src/list.c \
../freeRTOS/src/port.c \
../freeRTOS/src/queue.c \
../freeRTOS/src/tasks.c \
../freeRTOS/src/timers.c 

OBJS += \
./freeRTOS/src/FreeRTOSCommonHooks.o \
./freeRTOS/src/croutine.o \
./freeRTOS/src/event_groups.o \
./freeRTOS/src/heap_3.o \
./freeRTOS/src/list.o \
./freeRTOS/src/port.o \
./freeRTOS/src/queue.o \
./freeRTOS/src/tasks.o \
./freeRTOS/src/timers.o 

C_DEPS += \
./freeRTOS/src/FreeRTOSCommonHooks.d \
./freeRTOS/src/croutine.d \
./freeRTOS/src/event_groups.d \
./freeRTOS/src/heap_3.d \
./freeRTOS/src/list.d \
./freeRTOS/src/port.d \
./freeRTOS/src/queue.d \
./freeRTOS/src/tasks.d \
./freeRTOS/src/timers.d 


# Each subdirectory must supply rules for building sources it contributes
freeRTOS/src/%.o: ../freeRTOS/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DNO_BOARD_LIB -D__LPC17XX__ -D__REDLIB__ -I"D:\Facultad\Digitales II\Repositorio MCU\lpc_chip_175x_6x" -I"C:\Users\kevin\git\ControlVehicular\Example\inc" -I"C:\Users\kevin\git\ControlVehicular\freeRTOS\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\FRTOS\Example\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\lpc_chip_175x_6x\inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


