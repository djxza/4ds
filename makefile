# Simple OS Makefile - Custom Bootloader Only
#
ARCH = x86_64-elf-
AS = $(ARCH)as
CC = $(ARCH)gcc
LD = $(ARCH)ld
OBJCOPY = $(ARCH)objcopy

CFLAGS = -m64 -ffreestanding -nostdlib -fno-stack-protector -Os -g -I./include -Wall -Wextra
ASFLAGS = --64
LDFLAGS = -nostdlib

# Targets
IMG = os.img
BOOT = ./bin/boot.bin
KERNEL = ./bin/kernel.bin
KERNEL_ELF = ./bin/kernel.elf

# Source file discovery
BOOT_SRC_S  = $(wildcard src/boot/*.S)
BOOT_SRC_C  = $(wildcard src/boot/*.c)
BOOT_SRC    = $(BOOT_SRC_S) $(BOOT_SRC_C)
BOOT_OBJ    = $(patsubst src/%.S,obj/%.o,$(BOOT_SRC_S)) \
              $(patsubst src/%.c,obj/%.o,$(BOOT_SRC_C))

KERNEL_SRC_S = $(wildcard src/kernel/*.S)
KERNEL_SRC_C = $(wildcard src/kernel/*.c)
KERNEL_SRC   = $(KERNEL_SRC_S) $(KERNEL_SRC_C)
KERNEL_OBJ   = $(patsubst src/%.S,obj/%.o,$(KERNEL_SRC_S)) \
               $(patsubst src/%.c,obj/%.o,$(KERNEL_SRC_C))

# All object files
ALL_OBJ = $(BOOT_OBJ) $(KERNEL_OBJ)

# Default target - build floppy image
all: $(IMG)

# Ensure directories exist
bin:
	@mkdir -p bin

obj:
	@mkdir -p obj/boot obj/kernel

# Compilation rules for bootloader
obj/boot/%.o: src/boot/%.S | obj
	@echo "  AS    $<"
	$(AS) $(ASFLAGS) -c $< -o $@

obj/boot/%.o: src/boot/%.c | obj
	@echo "  CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Compilation rules for kernel
obj/kernel/%.o: src/kernel/%.S | obj
	@echo "  AS    $<"
	$(AS) $(ASFLAGS) -c $< -o $@

obj/kernel/%.o: src/kernel/%.c | obj
	@echo "  CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

# Link bootloader - MUST be exactly 512 bytes with boot signature
$(BOOT): $(BOOT_OBJ) | bin
	@echo "  LD    $@ (Custom Bootloader)"
	$(LD) -Ttext 0x7C00 --oformat=binary -nostdlib -o $@ $(BOOT_OBJ)
	@size=$$(stat -c%s "$@" 2>/dev/null || stat -f%z "$@"); \
	echo "Bootloader raw size: $$size bytes"; \
	# Pad to exactly 510 bytes (boot sector minus signature)
	if [ $$size -lt 510 ]; then \
		echo "Padding to 510 bytes..."; \
		dd if=/dev/zero bs=1 count=$$((510 - size)) >> "$@" 2>/dev/null; \
	elif [ $$size -gt 510 ]; then \
		echo "ERROR: Bootloader too large! Must be ≤ 510 bytes, got $$size bytes"; \
		hexdump -C "$@" | tail -5; \
		exit 1; \
	fi
	# Add boot signature (0xAA55 in little-endian) at byte 510
	printf '\x55\xAA' | dd of="$@" bs=1 seek=510 conv=notrunc 2>/dev/null
	@final_size=$$(stat -c%s "$@" 2>/dev/null || stat -f%z "$@"); \
	echo "Bootloader final size: $$final_size bytes (with boot signature)"; \
	echo "✓ Boot sector ready"

# Link kernel (64-bit protected mode)
$(KERNEL_ELF): $(KERNEL_OBJ) linker.ld | bin
	@echo "  LD    $@"
	$(LD) $(LDFLAGS) -T linker.ld -o $@ $(KERNEL_OBJ)

# Create binary kernel image
$(KERNEL): $(KERNEL_ELF)
	@echo "  OBJCOPY $@"
	$(OBJCOPY) -O binary $(KERNEL_ELF) $@
	@size=$$(stat -c%s "$@" 2>/dev/null || stat -f%z "$@"); \
	echo "Kernel size: $$size bytes"; \
	sectors=$$(( (size + 511) / 512 )); \
	echo "Kernel occupies $$sectors sector(s) on disk"

# Create bootable floppy image
$(IMG): $(BOOT) $(KERNEL)
	@echo "  IMG   $@"
	@echo "Creating 1.44MB floppy image..."
	@echo "Bootloader: sector 0 (0x0000-0x01FF)"
	@echo "Kernel: starting at sector 1 (0x0200)"
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(BOOT) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL) of=$@ bs=512 seek=1 conv=notrunc 2>/dev/null
	@echo "✓ Created bootable image: $@"

# Run with QEMU
run: $(IMG)
	@echo "=== Starting QEMU ==="
	@echo "Image: $(IMG)"
	@echo "Boot: Custom bootloader (16-bit) → Kernel (64-bit)"
	@echo "Memory: 512MB"
	@echo ""
	qemu-system-x86_64 \
		-drive format=raw,file=$(IMG),index=0,if=floppy \
		-serial stdio \
		-m 512M \
		-no-reboot \
		-no-shutdown 
#		-serial chardev:char0
#		-chardev stdio,id=char0,mux=on \


# Debug with QEMU
debug: $(IMG)
	@echo "=== Starting QEMU in Debug Mode ==="
	@echo "Image: $(IMG)"
	@echo "GDB server: localhost:1234"
	@echo "Press Ctrl+A then X to exit QEMU"
	@echo ""
	qemu-system-x86_64 \
		-drive format=raw,file=$(IMG),index=0,if=floppy \
		-serial stdio \
		-m 64M \
		-s -S \
		-no-reboot \
		-no-shutdown

# Debug with GDB
gdb: $(KERNEL_ELF)
	@echo "=== Starting GDB ==="
	@echo "Target: localhost:1234"
	@echo "Symbols: $(KERNEL_ELF)"
	@echo ""
	$(ARCH)gdb -ex "target remote localhost:1234" \
	            -ex "symbol-file $(KERNEL_ELF)" \
	            -ex "break _start" \
	            -ex "continue"

# Disassemble kernel for debugging
disasm: $(KERNEL_ELF)
	@echo "=== Disassembling Kernel ==="
	$(ARCH)objdump -d $(KERNEL_ELF)

# Disassemble bootloader
disasm-boot: $(BOOT)
	@echo "=== Disassembling Bootloader ==="
	$(ARCH)objdump -b binary -m i8086 -D $(BOOT)

# Hex dump of boot sector
hexdump-boot: $(BOOT)
	@echo "=== Boot Sector Hex Dump ==="
	hexdump -C $(BOOT)

# Hex dump of kernel (first 512 bytes)
hexdump-kernel: $(KERNEL)
	@echo "=== Kernel Hex Dump (first 512 bytes) ==="
	dd if=$(KERNEL) bs=512 count=1 2>/dev/null | hexdump -C

# Print all source files
print-src:
	@echo "=== Source Files ==="
	@echo ""
	@echo "Bootloader Sources:"
	@if [ -z "$(BOOT_SRC)" ]; then \
		echo "  No bootloader source files found"; \
	else \
		for file in $(sort $(BOOT_SRC)); do \
			echo "  $$file"; \
		done; \
	fi
	@echo ""
	@echo "Kernel Sources:"
	@if [ -z "$(KERNEL_SRC)" ]; then \
		echo "  No kernel source files found"; \
	else \
		for file in $(sort $(KERNEL_SRC)); do \
			echo "  $$file"; \
		done; \
	fi
	@echo ""
	@echo "Total source files: $$(echo $(BOOT_SRC) $(KERNEL_SRC) | wc -w 2>/dev/null || echo 0)"
	@echo "  Bootloader: $$(echo $(BOOT_SRC) | wc -w 2>/dev/null || echo 0) files"
	@echo "  Kernel: $$(echo $(KERNEL_SRC) | wc -w 2>/dev/null || echo 0) files"

# Print all object files
print-obj:
	@echo "=== Object Files ==="
	@echo ""
	@echo "Bootloader Objects:"
	@if [ -z "$(BOOT_OBJ)" ]; then \
		echo "  No bootloader objects defined"; \
	else \
		for file in $(sort $(BOOT_OBJ)); do \
			if [ -e "$$file" ]; then \
				size=$$(stat -c%s "$$file" 2>/dev/null || stat -f%z "$$file"); \
				echo "  $$file ($$size bytes)"; \
			else \
				echo "  $$file (not built)"; \
			fi; \
		done; \
	fi
	@echo ""
	@echo "Kernel Objects:"
	@if [ -z "$(KERNEL_OBJ)" ]; then \
		echo "  No kernel objects defined"; \
	else \
		for file in $(sort $(KERNEL_OBJ)); do \
			if [ -e "$$file" ]; then \
				size=$$(stat -c%s "$$file" 2>/dev/null || stat -f%z "$$file"); \
				echo "  $$file ($$size bytes)"; \
			else \
				echo "  $$file (not built)"; \
			fi; \
		done; \
	fi

# Print all output files
print-out:
	@echo "=== Output Files ==="
	@echo ""
	@echo "Generated Files:"
	@for file in $(sort $(BOOT) $(KERNEL) $(KERNEL_ELF) $(IMG)); do \
		if [ -e "$$file" ]; then \
			size=$$(stat -c%s "$$file" 2>/dev/null || stat -f%z "$$file"); \
			echo "  $$file ($$size bytes)"; \
		else \
			echo "  $$file (not built)"; \
		fi; \
	done
	@echo ""
	@echo "Build Directories:"
	@if [ -d "obj" ]; then \
		echo "  obj/ exists"; \
		obj_count=$$(find obj -name "*.o" 2>/dev/null | wc -l); \
		echo "  Contains $$obj_count object file(s)"; \
	else \
		echo "  obj/ does not exist"; \
	fi
	@if [ -d "bin" ]; then \
		echo "  bin/ exists"; \
		bin_count=$$(find bin -type f 2>/dev/null | wc -l); \
		echo "  Contains $$bin_count binary file(s)"; \
	else \
		echo "  bin/ does not exist"; \
	fi

# Print build configuration
print-config:
	@echo "=== Build Configuration ==="
	@echo ""
	@echo "Toolchain:"
	@echo "  ARCH: $(ARCH)"
	@echo "  AS: $(AS)"
	@echo "  CC: $(CC)"
	@echo "  LD: $(LD)"
	@echo "  OBJCOPY: $(OBJCOPY)"
	@echo ""
	@echo "Flags:"
	@echo "  CFLAGS: $(CFLAGS)"
	@echo "  ASFLAGS: $(ASFLAGS)"
	@echo "  LDFLAGS: $(LDFLAGS)"
	@echo ""
	@echo "Targets:"
	@echo "  Image: $(IMG)"
	@echo "  Bootloader: $(BOOT)"
	@echo "  Kernel Binary: $(KERNEL)"
	@echo "  Kernel ELF: $(KERNEL_ELF)"
	@echo ""
	@echo "Memory Layout:"
	@echo "  Bootloader: 0x7C00 (512 bytes with boot signature)"
	@echo "  Kernel: Loaded at 1MB (0x100000) via linker.ld"
	@echo "  VGA Buffer: 0xB8000 (text mode)"
	@echo ""
	@echo "Boot Process:"
	@echo "  1. BIOS loads boot sector (512 bytes) to 0x7C00"
	@echo "  2. Bootloader jumps to kernel at 1MB"
	@echo "  3. Kernel runs in 64-bit protected mode"

# Print bootloader info in detail
print-bootinfo: $(BOOT)
	@echo "=== Bootloader Info ==="
	@size=$$(stat -c%s "$(BOOT)" 2>/dev/null || stat -f%z "$(BOOT)"); \
	echo "Size: $$size bytes"
	@echo "Breakdown:"
	@echo "  Code size: $$((size - 2)) bytes (without signature)"
	@echo "  Boot signature: 2 bytes (0x55AA at offset 510)"
	@echo ""
	@echo "Checking boot signature..."
	@dd if="$(BOOT)" bs=1 skip=510 count=2 2>/dev/null | od -An -tx1 | grep -q "55 aa" && \
		echo "✓ Boot signature is correct (0x55AA)" || \
		echo "✗ Boot signature is missing or incorrect!"
	@echo ""
	@echo "Hex dump of last 16 bytes:"
	@dd if="$(BOOT)" bs=1 skip=496 count=16 2>/dev/null | hexdump -C

# Clean build artifacts
clean:
	@echo "=== Cleaning Build ==="
	rm -rf obj bin *.img
	@echo "Removed: obj/, bin/, *.img"
	@echo "Clean complete."

# Clean everything
distclean: clean
	@rm -f tags
	@echo "Removed: tags"
	@echo "Distclean complete."

# Create tags for code navigation
tags:
	@echo "Generating tags..."
	find . -name "*.c" -o -name "*.h" -o -name "*.S" | ctags -L -
	@echo "Tags generated"

# Quick build test
test: clean all
	@echo ""
	@echo "=== Build Test Complete ==="
	@echo "All targets built successfully!"
	@make print-bootinfo

# Help target
help:
	@echo "=== Available Targets ==="
	@echo ""
	@echo "Build Targets:"
	@echo "  all           - Build everything (default)"
	@echo "  test          - Clean build and verify"
	@echo ""
	@echo "Run Targets:"
	@echo "  run           - Run in QEMU"
	@echo "  debug         - Debug in QEMU (GDB port 1234)"
	@echo "  gdb           - Connect GDB to running QEMU"
	@echo ""
	@echo "Info Targets:"
	@echo "  print-src     - Print all source files"
	@echo "  print-obj     - Print all object files"
	@echo "  print-out     - Print all output files"
	@echo "  print-config  - Print build configuration"
	@echo "  print-bootinfo- Detailed bootloader info"
	@echo ""
	@echo "Debug Targets:"
	@echo "  disasm        - Disassemble kernel"
	@echo "  disasm-boot   - Disassemble bootloader"
	@echo "  hexdump-boot  - Hex dump boot sector"
	@echo "  hexdump-kernel- Hex dump kernel start"
	@echo ""
	@echo "Maintenance:"
	@echo "  clean         - Remove build artifacts"
	@echo "  distclean     - Remove all generated files"
	@echo "  tags          - Generate tags for code navigation"
	@echo "  help          - Show this help"

.PHONY: all run debug gdb disasm disasm-boot hexdump-boot hexdump-kernel \
        print-src print-obj print-out print-config print-bootinfo test \
        clean distclean tags help
