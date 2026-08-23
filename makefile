
# Build and Object Directories

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj


# Submodule Paths

STM32CubeL4_CMSIS := submodules/STM32CubeL4/Drivers/CMSIS
FreeRTOS_Kernel := submodules/FreeRTOS/FreeRTOS/Source
FreeRTOS_Plus := submodules/FreeRTOS/FreeRTOS-Plus/Source
CMSIS_DSP := submodules/STM32CubeL4/Drivers/CMSIS/DSP


# All Include Directories

INC_DIRS := inc
INC_DIRS += $(STM32CubeL4_CMSIS)/Device/ST/STM32L4xx/Include
INC_DIRS +=	$(STM32CubeL4_CMSIS)/Include
INC_DIRS +=	$(FreeRTOS_Kernel)/include
INC_DIRS +=	$(FreeRTOS_Kernel)/portable/GCC/ARM_CM4F
INC_DIRS +=	$(FreeRTOS_Plus)/FreeRTOS-Plus-CLI
INC_DIRS +=	$(CMSIS_DSP)/Include

INCS := $(addprefix -I, $(INC_DIRS))


# All Source Files

SRC_DIR := src

SINGLE_SRCS := 	$(STM32CubeL4_CMSIS)/Device/ST/STM32L4xx/Source/Templates/system_stm32l4xx.c
SINGLE_SRCS +=	$(FreeRTOS_Kernel)/portable/GCC/ARM_CM4F/port.c
SINGLE_SRCS +=	$(FreeRTOS_Kernel)/portable/MemMang/heap_4.c
SINGLE_SRCS +=	$(FreeRTOS_Plus)/FreeRTOS-Plus-CLI/FreeRTOS_CLI.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/CommonTables/CommonTables.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/TransformFunctions/arm_rfft_init_q15.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/TransformFunctions/arm_rfft_q15.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/TransformFunctions/arm_cfft_q15.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/TransformFunctions/arm_cfft_radix4_q15.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/TransformFunctions/arm_bitreversal2.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/FastMathFunctions/arm_sqrt_q15.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/StatisticsFunctions/arm_rms_q15.c
SINGLE_SRCS +=	$(CMSIS_DSP)/Source/ComplexMathFunctions/arm_cmplx_mag_q15.c

SRCS := $(SINGLE_SRCS)
SRCS += $(wildcard $(SRC_DIR)/*.c)
SRCS += $(wildcard $(FreeRTOS_Kernel)/*.c)


# All Assembly Files

SINGLE_ASMS := $(STM32CubeL4_CMSIS)/Device/ST/STM32L4xx/Source/Templates/gcc/startup_stm32l432xx.s

ASMS :=	$(SINGLE_ASMS)
ASMS += $(wildcard $(SRC_DIR)/*.s)

COBJS := $(patsubst %.c, $(OBJ_DIR)/%.o, $(SRCS))
AOBJS := $(patsubst %.s, $(OBJ_DIR)/%.o, $(ASMS))


# Compiler and Linker Flags

ARCH_DEFS := -mthumb -mcpu=cortex-m4 -march=armv7e-m
FPU_DEFS := -mfpu=fpv4-sp-d16 -mfloat-abi=hard
COMPILER_DEFS := -g -Os -fdata-sections -ffunction-sections -specs=nano.specs -specs=nosys.specs
CMSIS_DSP_DEFS := -DARM_DSP_CONFIG_TABLES -DARM_FFT_ALLOW_TABLES -DARM_TABLE_REALCOEF_Q15 -DARM_TABLE_TWIDDLECOEF_Q15_16 -DARM_TABLE_BITREVIDX_FXT_16
DDEFS := -DSTM32L432xx

CDEFS := $(ARCH_DEFS)
CDEFS += $(FPU_DEFS)
CDEFS += $(COMPILER_DEFS)
CDEFS += $(CMSIS_DSP_DEFS)
CDEFS += $(DDEFS)
CDEFS += $(INCS)


# Build Targets

TARGET := firmware
all: $(BUILD_DIR)/$(TARGET).elf

$(COBJS): $(OBJ_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "gcc -c $(notdir $^) -o $(notdir $@)"
	@arm-none-eabi-gcc $(CDEFS) -c $^ -o $@ 


$(AOBJS): $(OBJ_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	@echo "gcc -c $(notdir $^) -o $(notdir $@)"
	@arm-none-eabi-gcc $(CDEFS) -c $^ -o $@

$(BUILD_DIR)/$(TARGET).elf: $(COBJS) $(AOBJS)
	arm-none-eabi-gcc $(CDEFS) -T STM32L432KCUX.ld $^ -Wl,--gc-sections -Wl,-Map=$(BUILD_DIR)/$(TARGET).map -o $@
	arm-none-eabi-objdump -S -d $@ > $(BUILD_DIR)/$(TARGET).asm
	arm-none-eabi-objdump -h $@ > $(BUILD_DIR)/$(TARGET).lst
	arm-none-eabi-size $@


clean:
	rm -rf $(BUILD_DIR)
