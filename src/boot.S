.intel_syntax noprefix
.global _start
.extern long_mode_start

/* =========================
 * 32-bit entry
 * ========================= */
.section .text
.code32

.intel_syntax noprefix
.global _start
.extern long_mode_start

/* =========================
 * 32-bit entry
 * ========================= */
.section .text
.code32

_start:
    mov esp, OFFSET stack_top

    /* Save multiboot2 info pointer IMMEDIATELY (passed in EBX) */
    push ebx                    /* Save on stack */
    
    /* Check multiboot2 magic */
    cmp eax, 0x36d76289
    jne no_multiboot
    
    /* Restore and save to non-volatile register */
    pop ebx
    push ebx                    /* Save again for later */

    call check_cpuid
    call check_long_mode
    call setup_page_tables
    call enable_paging

    lgdt gdt64_pointer

    /* Get the saved MBI pointer */
    pop esi                     /* ESI = MBI pointer (will be preserved) */
    
    ljmp 0x08, OFFSET long_mode_start

    hlt

/* ... rest of check_multiboot removed since we do it at start ... */

no_multiboot:
    mov al, 'M'
    jmp error

/* ... rest of boot.S remains similar ... */
check_cpuid:
    pushfd
    pop eax
    mov ecx, eax
    xor eax, (1 << 21)
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je no_cpuid
    ret

no_cpuid:
    mov al, 'C'
    jmp error


check_long_mode:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb no_long_mode

    mov eax, 0x80000001
    cpuid
    test edx, (1 << 29)
    jz no_long_mode
    ret

no_long_mode:
    mov al, 'L'
    jmp error


/* =========================
 * Page tables
 * ========================= */

setup_page_tables:
    mov eax, OFFSET page_table_l3
    or  eax, 0x3                      /* present + writable */
    mov DWORD PTR [page_table_l4], eax

    mov eax, OFFSET page_table_l2
    or  eax, 0x3
    mov DWORD PTR [page_table_l3], eax

    xor ecx, ecx                     /* counter = 0 */

pt_loop:
    mov eax, 0x200000                /* 2 MiB */
    mul ecx
    or  eax, 0x83                    /* present + writable + huge */
    mov DWORD PTR [page_table_l2 + ecx*8], eax

    inc ecx
    cmp ecx, 512 // sus ahh line
    jne pt_loop
    ret


enable_paging:
    /* load PML4 */
    mov eax, OFFSET page_table_l4
    mov cr3, eax

    /* enable PAE */
    mov eax, cr4
    or  eax, (1 << 5)
    mov cr4, eax

    /* enable long mode */
    mov ecx, 0xC0000080              /* EFER MSR */
    rdmsr
    or  eax, (1 << 8)                /* LME */
    wrmsr

    /* enable paging */
    mov eax, cr0
    or  eax, (1 << 31)
    mov cr0, eax
    ret


/* =========================
 * Error print (VGA text)
 * ========================= */

error:
    mov DWORD PTR [0xB8000], 0x4F524F45   /* 'E''R''R' */
    mov DWORD PTR [0xB8004], 0x4F3A4F52   /* ':' */
    mov DWORD PTR [0xB8008], 0x4F204F20   /* ' ' */
    mov BYTE  PTR [0xB800A], al
    hlt


/* =========================
 * BSS (zeroed memory)
 * ========================= */

.section .bss
.global stack_top

.align 4096
page_table_l4:
    .skip 4096
page_table_l3:
    .skip 4096
page_table_l2:
    .skip 4096

stack_bottom:
    .skip 4096 * 4
stack_top:


/* =========================
 * GDT
 * ========================= */

.section .rodata
.align 8
gdt64:
    .quad 0x0000000000000000         /* null descriptor */
    .quad 0x00AF9A000000FFFF         /* 64-bit code segment */

gdt64_pointer:
    .word (gdt64_end - gdt64 - 1)
    .long gdt64

gdt64_end:

