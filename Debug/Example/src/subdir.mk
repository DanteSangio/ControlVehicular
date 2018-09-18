################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Example/src/MFRC522.c \
../Example/src/SD.c \
../Example/src/cr_startup_lpc175x_6x.c \
../Example/src/crp.c \
../Example/src/fat32.c \
../Example/src/rfid_utils.c \
../Example/src/sdcard.c \
../Example/src/sysinit.c 

OBJS += \
./Example/src/MFRC522.o \
./Example/src/SD.o \
./Example/src/cr_startup_lpc175x_6x.o \
./Example/src/crp.o \
./Example/src/fat32.o \
./Example/src/rfid_utils.o \
./Example/src/sdcard.o \
./Example/src/sysinit.o 

C_DEPS += \
./Example/src/MFRC522.d \
./Example/src/SD.d \
./Example/src/cr_startup_lpc175x_6x.d \
./Example/src/crp.d \
./Example/src/fat32.d \
./Example/src/rfid_utils.d \
./Example/src/sdcard.d \
./Example/src/sysinit.d 


# Each subdirectory must supply rules for building sources it contributes
Example/src/%.o: ../Example/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DNO_BOARD_LIB -D__LPC17XX__ -D__REDLIB__ -I"D:\Facultad\Digitales II\Repositorio MCU\lpc_chip_175x_6x" -I"C:\Users\kevin\git\ControlVehicular\Example\inc" -I"C:\Users\kevin\git\ControlVehicular\freeRTOS\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\FRTOS\Example\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\lpc_chip_175x_6x\inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


