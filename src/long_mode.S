.intel_syntax noprefix

.global long_mode_start
.extern kernel_main

.section .text
.code64
long_mode_start:
    /* Set up 64-bit stack */
    mov rsp, OFFSET stack_top
    
    /* Clear segment registers */
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    /* ESI has magic, EDI has MBI pointer */
    /* Pass both to kernel_main */
    mov rdx, 0
    mov edx, esi            /* RDX = magic value */
    mov rdi, 0
    mov edi, edi            /* RDI = MBI pointer (zero-extend) */

    mov rdi, rsi    # RDI = MBI pointer (1st param)
    mov rsi, rax    # RSI = Magic value (2nd param)
    
    /* Call kernel_main with: RDI = MBI pointer, RDX = magic */
    call kernel_main

    hlt
