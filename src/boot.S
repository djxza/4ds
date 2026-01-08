.intel_syntax noprefix

# =========================
# Multiboot2 header
# =========================
.section .multiboot
.align 8
mb2_header_start:
    .long 0xe85250d6
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(0xe85250d6 + 0 + (mb2_header_end - mb2_header_start))

    # framebuffer request
    .align 8
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32

    # end
    .align 8
    .short 0
    .short 0
    .long 8
mb2_header_end:

# =========================
# 32-bit entry (GRUB)
# =========================
.section .text
.code32
.global _start
.extern kernel_main64

_start:
    cli

# save multiboot info pointer (RSI in Multiboot2)
    mov dword ptr [mb_info_ptr], esi

    # load temporary stack
    mov esp, stack_top32

    # enable PAE
    mov eax, cr4
    or eax, (1 << 5)
    mov cr4, eax

    # load PML4
    mov eax, pml4
    mov cr3, eax

    # enable long mode
    mov ecx, 0xC0000080      # EFER MSR
    rdmsr
    or eax, (1 << 8)         # LME
    wrmsr

    # enable paging
    mov eax, cr0
    or eax, (1 << 31)
    mov cr0, eax

    # far jump to 64-bit
    ljmp 0x08, long_mode_start

# =========================
# 64-bit code
# =========================
.code64
long_mode_start:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    lea rsp, [stack_top64]

    # pass multiboot info to C
    mov edi, dword ptr [mb_info_ptr]
    call kernel_main64

.hang:
    hlt
    jmp .hang

# =========================
# GDT
# =========================
.section .rodata
.align 8
gdt:
    .quad 0x0000000000000000   # null
    .quad 0x00AF9A000000FFFF   # 64-bit code
    .quad 0x00AF92000000FFFF   # data

gdt_desc:
    .word gdt_desc - gdt - 1
    .quad gdt

# =========================
# Paging (identity map 0–1GB using 2MiB pages)
# =========================
.section .data
.align 4096

pml4:
    .quad pdpt + 0x03

.align 4096
pdpt:
    .quad pd + 0x03

.align 4096
pd:
    .set i, 0
.rept 512
    .quad (i * 0x200000) | 0x83
    .set i, i + 1
.endr

# =========================
# Stacks
# =========================
.section .bss
.align 16
stack_bottom32:
    .skip 4096
stack_top32:

.align 16
stack_bottom64:
    .skip 16384
stack_top64:

mb_info_ptr:
    .long 0

