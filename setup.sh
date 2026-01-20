#!/bin/bash
# Setup script for bootloader project

echo "Setting up bootloader project..."

# Create necessary directories
mkdir -p src/boot src/kernel obj/boot obj/kernel bin iso/boot/grub

# Save the bootloader assembly (s0.S)
cat > src/boot/s0.S << 'EOF'
/* ============================================================================
 * Minimal 64-bit Bootloader (s0.S)
 * ============================================================================ */

.code16
.section .text
.global _start
_start:
    jmp     start
    nop

/* BIOS Parameter Block */
bpb:
    .skip   62

start:
    /* Setup */
    cli
    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss
    movw    $0x7C00, %sp
    sti
    
    /* Save drive */
    movb    %dl, drive_number
    
    /* Print message */
    movw    $msg_booting, %si
    call    print_string
    
    /* Load kernel (sectors 2-65) */
    xorw    %ax, %ax
    int     $0x13
    jc      disk_error
    
    movw    $0x7E0, %ax
    movw    %ax, %es
    xorw    %bx, %bx
    
    movb    $64, %al
    movb    $0x02, %cl
    movb    $0x00, %ch
    movb    $0x00, %dh
    movb    drive_number, %dl
    
    movb    $0x02, %ah
    int     $0x13
    jc      disk_error
    
    movw    $msg_kernel_loaded, %si
    call    print_string
    
    /* Enable A20 */
    movw    $0x2401, %ax
    int     $0x15
    jnc     a20_enabled
    
    call    wait_kbd
    movb    $0xD1, %al
    outb    $0x64
    call    wait_kbd
    movb    $0xDF, %al
    outb    $0x60
    
a20_enabled:
    /* Switch to protected mode */
    cli
    lgdt    gdt32_ptr
    
    movl    %cr0, %eax
    orl     $0x1, %eax
    movl    %eax, %cr0
    
    ljmp    $0x08, $protected_mode

.code32
protected_mode:
    movw    $0x10, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss
    movl    $0x7C00, %esp
    
    /* Copy kernel to 1MB */
    movl    $0x7E00, %esi
    movl    $0x100000, %edi
    movl    $(64 * 512), %ecx
    cld
    rep     movsb
    
    /* Check 64-bit support */
    pushfl
    popl    %eax
    movl    %eax, %ecx
    xorl    $0x200000, %eax
    pushl   %eax
    popfl
    pushfl
    popl    %eax
    pushl   %ecx
    popfl
    xorl    %ecx, %eax
    jz      no_long_mode
    
    movl    $0x80000000, %eax
    cpuid
    cmpl    $0x80000001, %eax
    jb      no_long_mode
    
    movl    $0x80000001, %eax
    cpuid
    testl   $(1 << 29), %edx
    jz      no_long_mode
    
    /* Setup paging */
    movl    $0x90000, %edi
    movl    $0x1000, %ecx
    xorl    %eax, %eax
    rep     stosl
    
    movl    $0x91000 | 0x3, 0x90000
    movl    $0x92000 | 0x3, 0x91000
    movl    $0x00000000 | 0x83, 0x92000
    
    movl    $0x90000, %eax
    movl    %eax, %cr3
    
    movl    %cr4, %eax
    orl     $(1 << 5), %eax
    movl    %eax, %cr4
    
    movl    $0xC0000080, %ecx
    rdmsr
    orl     $(1 << 8), %eax
    wrmsr
    
    movl    %cr0, %eax
    orl     $0x80000001, %eax
    movl    %eax, %cr0
    
    lgdt    gdt64_ptr
    ljmp    $0x08, $long_mode

no_long_mode:
    movl    $msg_no_64bit, %esi
    movl    $0xB8000, %edi
    movb    $0x4F, %ah
1:  lodsb
    testb   %al, %al
    jz      2f
    stosw
    jmp     1b
2:  hlt
    jmp     2b

.code64
long_mode:
    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs
    movw    %ax, %ss
    movq    $0x7C00, %rsp
    
    /* Clear screen */
    movq    $0xB8000, %rdi
    movq    $0x0720072007200720, %rax
    movq    $1000, %rcx
    rep     stosq
    
    /* Print message */
    movq    $0xB8000, %rdi
    movq    $msg_64bit, %rsi
    movb    $0x0A, %ah
1:  lodsb
    testb   %al, %al
    jz      2f
    stosw
    jmp     1b
2:
    
    /* Jump to kernel */
    movq    $0x100000, %rax
    jmp     *%rax

.code16
print_string:
    pusha
    movb    $0x0E, %ah
    xorb    %bh, %bh
1:  lodsb
    testb   %al, %al
    jz      2f
    int     $0x10
    jmp     1b
2:  popa
    ret

wait_kbd:
    inb     $0x64, %al
    testb   $0x2, %al
    jnz     wait_kbd
    ret

disk_error:
    movw    $msg_disk_error, %si
    call    print_string
1:  hlt
    jmp     1b

/* Data */
msg_booting:        .asciz "Boot: Loading kernel...\r\n"
msg_kernel_loaded:  .asciz "Boot: Kernel loaded.\r\n"
msg_disk_error:     .asciz "Boot: Disk error!\r\n"
msg_no_64bit:       .asciz "No 64-bit support"
msg_64bit:          .asciz "64-bit mode"

drive_number:       .byte 0

.align 4
gdt32:
    .quad   0x0000000000000000
gdt32_code:
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0x9A
    .byte   0xCF
    .byte   0x00
gdt32_data:
    .word   0xFFFF
    .word   0x0000
    .byte   0x00
    .byte   0x92
    .byte   0xCF
    .byte   0x00
gdt32_end:

gdt32_ptr:
    .word   gdt32_end - gdt32 - 1
    .long   gdt32

gdt64:
    .quad   0x0000000000000000
gdt64_code:
    .word   0x0000
    .word   0x0000
    .byte   0x00
    .byte   0x9A
    .byte   0x20
    .byte   0x00
gdt64_data:
    .word   0x0000
    .word   0x0000
    .byte   0x00
    .byte   0x92
    .byte   0x00
    .byte   0x00
gdt64_end:

gdt64_ptr:
    .word   gdt64_end - gdt64 - 1
    .quad   gdt64

.fill 510 - (. - _start), 1, 0
.word 0xAA55
EOF

echo "✓ Created bootloader (src/boot/s0.S)"

# Save the Makefile
cat > Makefile << 'EOF'
# ============================================================================
# Bootloader Development Makefile
# ============================================================================

AS = x86_64-elf-as
LD = x86_64-elf-ld
CC = x86_64-elf-gcc
OBJCOPY = x86_64-elf-objcopy

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin

BOOT_SRC = $(SRC_DIR)/boot/s0.S
BOOT_OBJ = $(OBJ_DIR)/boot/s0.o
BOOT_BIN = $(BIN_DIR)/bootloader.bin
BOOT_IMG = $(BIN_DIR)/boot.img

KERNEL_SRC = $(SRC_DIR)/kernel/kernel.c
KERNEL_OBJ = $(OBJ_DIR)/kernel/kernel.o
KERNEL_ELF = $(BIN_DIR)/kernel.elf
KERNEL_BIN = $(BIN_DIR)/kernel.bin

all: $(BOOT_IMG)

$(BOOT_IMG): $(BOOT_BIN) $(KERNEL_BIN)
	@echo "Creating bootable image..."
	dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	dd if=$(BOOT_BIN) of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	dd if=$(KERNEL_BIN) of=$@ bs=512 seek=2 conv=notrunc 2>/dev/null
	@echo "✓ Boot image created"

$(BOOT_BIN): $(BOOT_OBJ)
	@echo "Building bootloader..."
	$(LD) -Ttext 0x7C00 --oformat=binary -o $@ $(BOOT_OBJ)
	@SIZE=$$(stat -c%s "$@"); \
	if [ $$SIZE -ne 512 ]; then \
		echo "ERROR: Bootloader is $$SIZE bytes (should be 512)"; \
		exit 1; \
	fi
	@echo "✓ Bootloader: $$SIZE bytes"

$(OBJ_DIR)/boot/s0.o: $(BOOT_SRC)
	@mkdir -p $(dir $@)
	$(AS) --64 -c $< -o $@

$(KERNEL_BIN): $(KERNEL_ELF)
	$(OBJCOPY) -O binary -j .text -j .rodata -j .data $< $@
	@echo "✓ Kernel: $$(stat -c%s $@) bytes"

$(KERNEL_ELF): $(KERNEL_OBJ) linker.ld
	$(LD) -T linker.ld -m elf_x86_64 -nostdlib -o $@ $(KERNEL_OBJ)

$(OBJ_DIR)/kernel/kernel.o: $(KERNEL_SRC)
	@mkdir -p $(dir $@)
	$(CC) -m64 -ffreestanding -nostdlib -fno-stack-protector -mno-red-zone -c $< -o $@

run: $(BOOT_IMG)
	@echo "Starting QEMU..."
	qemu-system-x86_64 -drive format=raw,file=$(BOOT_IMG),index=0,if=floppy -serial stdio

debug: $(BOOT_IMG)
	@echo "Starting QEMU for debugging..."
	qemu-system-x86_64 -drive format=raw,file=$(BOOT_IMG),index=0,if=floppy -s -S -serial stdio

hex: $(BOOT_BIN)
	@echo "Bootloader hex:"
	hexdump -C $(BOOT_BIN) | head -10

verify: $(BOOT_BIN)
	@echo "Verifying..."
	@SIZE=$$(stat -c%s "$(BOOT_BIN)"); \
	if [ $$SIZE -eq 512 ]; then \
		echo "✓ Size: 512 bytes"; \
		tail -c 2 $(BOOT_BIN) | hexdump -v -e '1/2 "%04x"' | grep -q aa55 && \
		echo "✓ Signature: 0xAA55" || echo "✗ No signature"; \
	else \
		echo "✗ Size: $$SIZE bytes"; \
	fi

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

.PHONY: all run debug hex verify clean
EOF

echo "✓ Created Makefile"

# Save linker script if it doesn't exist
if [ ! -f linker.ld ]; then
cat > linker.ld << 'EOF'
OUTPUT_FORMAT("elf64-x86-64")
ENTRY(_start)

SECTIONS
{
    . = 0x100000;
    
    .text : {
        *(.text*)
    }
    
    .rodata : {
        *(.rodata*)
    }
    
    .data : {
        *(.data*)
    }
    
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
    }
}
EOF
echo "✓ Created linker.ld"
fi

# Create a simple kernel if it doesn't exist
if [ ! -f src/kernel/kernel.c ]; then
cat > src/kernel/kernel.c << 'EOF'
/* Simple kernel for testing */
void kernel_main(void);

void _start(void) {
    kernel_main();
}

void kernel_main(void) {
    /* VGA text buffer at 0xB8000 */
    volatile char *vga = (volatile char*)0xB8000;
    
    /* Clear screen */
    for (int i = 0; i < 80 * 25 * 2; i += 2) {
        vga[i] = ' ';
        vga[i + 1] = 0x07;
    }
    
    /* Print message */
    const char *msg = "64-bit Kernel Running!";
    for (int i = 0; msg[i] != '\0'; i++) {
        vga[i * 2] = msg[i];
        vga[i * 2 + 1] = 0x0A;
    }
    
    /* Hang */
    while (1) {
        __asm__ volatile("hlt");
    }
}
EOF
echo "✓ Created kernel (src/kernel/kernel.c)"
fi

echo ""
echo "Setup complete! Now run:"
echo "1. make clean        # Clean any old files"
echo "2. make              # Build everything"
echo "3. make verify       # Check bootloader signature"
echo "4. make run          # Test in QEMU"
echo ""
echo "If you get toolchain errors, install:"
echo "  x86_64-elf-gcc, x86_64-elf-binutils, qemu-system-x86_64"
