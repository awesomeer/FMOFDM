
# Build and Object Directories

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj


# Submodule Paths

STM32CubeL4_CMSIS := submodules/STM32CubeL4/Drivers/CMSIS


# All Include Directories

INC_DIRS := inc
INC_DIRS += $(STM32CubeL4_CMSIS)/Device/ST/STM32L4xx/Include
INC_DIRS +=	$(STM32CubeL4_CMSIS)/Include

INCS := $(addprefix -I, $(INC_DIRS))


# All Source Files

SRC_DIR := src

SINGLE_SRCS := 	$(STM32CubeL4_CMSIS)/Device/ST/STM32L4xx/Source/Templates/system_stm32l4xx.c

SRCS := $(SINGLE_SRCS)
SRCS += $(wildcard $(SRC_DIR)/*.c)


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
DDEFS := -DSTM32L432xx

CDEFS := $(ARCH_DEFS)
CDEFS += $(FPU_DEFS)
CDEFS += $(COMPILER_DEFS)
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
