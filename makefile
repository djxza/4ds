ARCH = x86_64-elf
CC   = $(ARCH)-gcc
LD   = $(ARCH)-ld

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
ISO_DIR = iso

IMG = $(BIN_DIR)/4ds.img

CFLAGS  = -ffreestanding -O2 -Wall -Wextra -m64 \
          -fno-stack-protector -fno-pic -mno-red-zone -mcmodel=kernel

ASFLAGS = -m64 -ffreestanding

LDFLAGS = -T linker.ld -nostdlib -z max-page-size=0x1000

# -----------------------
# Sources
# -----------------------

C_SRCS   = $(wildcard $(SRC_DIR)/*.c)
ASM_SRCS = $(wildcard $(SRC_DIR)/*.S)

C_OBJS   = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(C_SRCS))
ASM_OBJS = $(patsubst $(SRC_DIR)/%.S,$(OBJ_DIR)/%.o,$(ASM_SRCS))

OBJS = $(ASM_OBJS) $(C_OBJS)

# -----------------------
# Targets
# -----------------------

all: $(IMG)

$(OBJ_DIR) $(BIN_DIR):
	mkdir -p $@

# C compilation
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Assembly (GAS via GCC, 64-bit)
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.S | $(OBJ_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

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
		-debugcon stdio \
		-m 512M \
		-no-reboot \
		-no-shutdown

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(ISO_DIR)

dump:
	chmod +x ./tools/dump_proj.sh
	./tools/dump_proj.sh

.PHONY: all clean emu

