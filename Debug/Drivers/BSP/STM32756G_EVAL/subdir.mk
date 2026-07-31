################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.c \
../Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.c 

OBJS += \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.o \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.o 

C_DEPS += \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.d \
./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/BSP/STM32756G_EVAL/%.o Drivers/BSP/STM32756G_EVAL/%.su Drivers/BSP/STM32756G_EVAL/%.cyclo: ../Drivers/BSP/STM32756G_EVAL/%.c Drivers/BSP/STM32756G_EVAL/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F746xx -c -I../Core/Inc -I../FATFS/Target -I../FATFS/App -I../USB_HOST/App -I../USB_HOST/Target -I../Drivers/STM32F7xx_HAL_Driver/Inc -I../Drivers/STM32F7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1 -I../Middlewares/Third_Party/FatFs/src -I../Middlewares/ST/STM32_USB_Host_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Host_Library/Class/CDC/Inc -I../Drivers/CMSIS/Device/ST/STM32F7xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-BSP-2f-STM32756G_EVAL

clean-Drivers-2f-BSP-2f-STM32756G_EVAL:
	-$(RM) ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_audio.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_camera.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_eeprom.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_io.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_lcd.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_nor.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_qspi.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sd.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sdram.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_sram.su ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.cyclo ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.d ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.o ./Drivers/BSP/STM32756G_EVAL/stm32756g_eval_ts.su

.PHONY: clean-Drivers-2f-BSP-2f-STM32756G_EVAL

