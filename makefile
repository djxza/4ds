ARCH = x86_64-elf
CC   = $(ARCH)-gcc
AS   = $(ARCH)-as
LD   = $(ARCH)-ld

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
ISO_DIR = iso

IMG = $(BIN_DIR)/4ds.img

CFLAGS   = -ffreestanding -O2 -Wall -Wextra -m64
LDFLAGS  = -T linker.ld -nostdlib -z max-page-size=0x1000

AS32FLAGS = --32
AS64FLAGS =

# -----------------------
# Sources
# -----------------------

C_SRCS = $(filter-out $(SRC_DIR)/kernel32.c,$(wildcard $(SRC_DIR)/*.c))

ASM32_SRCS = $(SRC_DIR)/boot.S
ASM64_SRCS = $(SRC_DIR)/long_mode.S

C_OBJS     = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRCS))
ASM32_OBJS = $(patsubst $(SRC_DIR)/%.S,$(OBJ_DIR)/%.o,$(ASM32_SRCS))
ASM64_OBJS = $(patsubst $(SRC_DIR)/%.S,$(OBJ_DIR)/%.o,$(ASM64_SRCS))

OBJS = $(ASM32_OBJS) $(ASM64_OBJS) $(C_OBJS)

# -----------------------
# Targets
# -----------------------

all: $(IMG)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# C compilation (64-bit)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 32-bit assembly
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S | $(OBJ_DIR)
	$(AS) $(AS32FLAGS) $< -o $@

# 64-bit assembly
$(OBJ_DIR)/long_mode.o: $(SRC_DIR)/long_mode.S | $(OBJ_DIR)
	$(AS) $(AS64FLAGS) $< -o $@

# Link kernel ELF
$(BIN_DIR)/kernel.elf: $(OBJS) | $(BIN_DIR)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# ISO image
$(IMG): $(BIN_DIR)/kernel.elf
	mkdir -p $(ISO_DIR)/boot/grub
	cp $< $(ISO_DIR)/boot/kernel.elf
	cp grub.cfg $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISO_DIR)

emu:
	qemu-system-x86_64 \
		-cdrom $(IMG) \
		-m 512M \
		-no-reboot \
		-no-shutdown \
		-vga std

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(ISO_DIR)

.PHONY: all clean emu

