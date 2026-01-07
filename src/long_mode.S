.global enter_long_mode
.extern kernel_main64

.code32
enter_long_mode:
    cli

    # Enable PAE
    mov %cr4, %eax
    or $0x20, %eax
    mov %eax, %cr4

    # Load PML4
    mov $pml4_table, %eax
    mov %eax, %cr3

    # Enable long mode
    mov $0xC0000080, %ecx
    rdmsr
    or $0x100, %eax
    wrmsr

    # Enable paging + protected mode
    mov %cr0, %eax
    or $0x80000001, %eax
    mov %eax, %cr0

    ljmp $0x08, $long_mode_entry


.code64
long_mode_entry:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss

    call kernel_main64
    hlt


# --------------------------------------------------
# Paging tables MUST be in .data (not .bss)
# --------------------------------------------------

.section .data
.align 4096

pml4_table:
    .quad pdpt_table + 0x3
    .fill 511,8,0

.align 4096
pdpt_table:
    .quad pd_table + 0x3
    .fill 511,8,0

.align 4096
pd_table:
    .set i, 0
    .rept 512
        .quad (i << 21) + 0x83
        .set i, i + 1
    .endr

