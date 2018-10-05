################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../RFID/src/MFRC522.c \
../RFID/src/RFID.c \
../RFID/src/rfid_utils.c 

OBJS += \
./RFID/src/MFRC522.o \
./RFID/src/RFID.o \
./RFID/src/rfid_utils.o 

C_DEPS += \
./RFID/src/MFRC522.d \
./RFID/src/RFID.d \
./RFID/src/rfid_utils.d 


# Each subdirectory must supply rules for building sources it contributes
RFID/src/%.o: ../RFID/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -DDEBUG -D__CODE_RED -DCORE_M3 -D__USE_LPCOPEN -DNO_BOARD_LIB -D__LPC17XX__ -D__REDLIB__ -I"C:\Users\kevin\git\ControlVehicular\RFID\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\lpc_chip_175x_6x" -I"C:\Users\kevin\git\ControlVehicular\Example\inc" -I"C:\Users\kevin\git\ControlVehicular\freeRTOS\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\FRTOS\Example\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\lpc_chip_175x_6x\inc" -I"D:\Facultad\Digitales II\Repositorio MCU\FRTOS\freeRTOS\inc" -O0 -fno-common -g3 -Wall -c -fmessage-length=0 -fno-builtin -ffunction-sections -fdata-sections -mcpu=cortex-m3 -mthumb -D__REDLIB__ -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


