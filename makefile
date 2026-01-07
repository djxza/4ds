#ARCH=x86_64-elf
ARCH=i386-elf
CC=$(ARCH)-gcc
AS=$(ARCH)-as
LD=$(ARCH)-ld

CFLAGS=-ffreestanding -O2 -Wall -Wextra
ASFLAGS=
LDFLAGS=-T linker.ld -nostdlib
SRC_DIR=src
OBJ_DIR=obj
BIN_DIR=bin
ISO_DIR=iso

# Discover all sources
C_SRCS := $(wildcard $(SRC_DIR)/*.c)
S_SRCS := $(wildcard $(SRC_DIR)/*.S)

# Map to object files
C_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRCS))
S_OBJS := $(patsubst $(SRC_DIR)/%.S,$(OBJ_DIR)/%.o,$(S_SRCS))

OBJS := $(S_OBJS) $(C_OBJS)

IMG =  $(BIN_DIR)/4ds.img

all: $(IMG)

# Directories
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Compile C
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assemble (GAS)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S | $(OBJ_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# Link kernel
$(BIN_DIR)/kernel.elf: $(OBJS) | $(BIN_DIR)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

# Create bootable image
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

.PHONY: all clean

