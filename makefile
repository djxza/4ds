# ========================
# 4DS Console OS Makefile
# GCC Assembler Edition
# ========================

# Output
KERNEL      := build/kernel.elf
IMG         := bin/4ds.img

# Tools
CC          := x86_64-elf-gcc
LD          := x86_64-elf-ld
GRUBMKRESCUE:= grub-mkrescue

# Flags
CFLAGS  := -std=gnu2x -ffreestanding -O2 -Wall -Wextra \
           -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel

ASFLAGS := -ffreestanding

LDFLAGS := -T linker.ld -nostdlib

# Directories
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin

# Sources
C_SOURCES  := $(wildcard $(SRC_DIR)/*.c)
S_SOURCES  := $(wildcard $(SRC_DIR)/*.S)

OBJECTS := \
  $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(C_SOURCES)) \
  $(patsubst $(SRC_DIR)/%.S, $(BUILD_DIR)/%.o, $(S_SOURCES))

# ========================
# Targets
# ========================

all: $(IMG)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile C
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble .S using GCC
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.S | $(BUILD_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

# Link kernel
$(KERNEL): $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) -o $@

# Create bootable image
$(IMG): $(KERNEL) | $(BIN_DIR)
	rm -rf build/isodir
	mkdir -p build/isodir/boot/grub
	cp $(KERNEL) build/isodir/boot/kernel.elf
	echo 'set timeout=0'                 >  build/isodir/boot/grub/grub.cfg
	echo 'set default=0'                 >> build/isodir/boot/grub/grub.cfg
	echo 'set gfxpayload=keep'           >> build/isodir/boot/grub/grub.cfg
	echo 'menuentry "4DS Console OS" {'  >> build/isodir/boot/grub/grub.cfg
	echo '  multiboot2 /boot/kernel.elf' >> build/isodir/boot/grub/grub.cfg
	echo '  boot'                        >> build/isodir/boot/grub/grub.cfg
	echo '}'                            >> build/isodir/boot/grub/grub.cfg
	$(GRUBMKRESCUE) -o $(IMG) build/isodir > /dev/null

run: $(IMG)
	qemu-system-x86_64 -cdrom $(IMG)

clean:
	rm -rf build
	rm -f $(IMG)

.PHONY: all run clean

