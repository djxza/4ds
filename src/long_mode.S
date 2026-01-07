.intel_syntax noprefix
.global enter_long_mode
.extern kernel_main64

.section .text
.code32
enter_long_mode:
    cli

    # Load GDT
    lgdt [gdt_desc]

    # Enable PAE
    mov eax, cr4
    or  eax, 1 << 5
    mov cr4, eax

    # Load PML4 base
    mov eax, pml4_table
    mov cr3, eax

    # Enable long mode (EFER.LME)
    mov ecx, 0xC0000080
    rdmsr
    or  eax, 1 << 8
    wrmsr

    # Enable paging
    mov eax, cr0
    or  eax, 1 << 31
    mov cr0, eax

    # Far jump into 64-bit code
    ljmp 0x08, long_mode_entry


.section .text
.code64
long_mode_entry:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov ss, ax

    lea rsp, [stack_top]

    call kernel_main64

.hang:
    hlt
    jmp .hang

# --------------------------------------------------
# GDT
# --------------------------------------------------
.section .rodata
.align 8

gdt:
    .quad 0x0000000000000000
    .quad 0x00AF9A000000FFFF    # code = 0x08
    .quad 0x00AF92000000FFFF    # data = 0x10

gdt_desc:
    .word (gdt_end - gdt - 1)
    .quad gdt
gdt_end:


# --------------------------------------------------
# Paging tables (identity map first 1GiB)
# --------------------------------------------------
.section .data
.align 4096

pml4_table:
    .quad pdpt_table + 0x3
    .fill 511,8,0

.align 4096
pdpt_table:
    .quad pd_table + 0x3          # first 1 GiB
    .quad pd_table + 0x3 + (1<<30)  # second 1 GiB
    .quad pd_table + 0x3 + (2<<30)  # third 1 GiB
    .quad pd_table + 0x3 + (3<<30)  # fourth 1 GiB
    .fill 508,8,0

.align 4096
pd_table:
    .set i, 0
    .rept 512
        .quad (i << 21) + 0x83
        .set i, i + 1
    .endr


# --------------------------------------------------
# Stack
# --------------------------------------------------
.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

