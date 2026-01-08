.intel_syntax noprefix

.global long_mode_start
.extern kernel_main

.section .text
.code64
long_mode_start:
    mov rsp, OFFSET stack_top
    
    /* Clear segment registers */
    xor ax, ax
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    /* System V ABI: 1st arg in RDI, 2nd in RSI.
       In boot.S: 
       - EAX held the magic (0x36d76289)
       - ESI was saved as the MBI pointer
    */
    mov rdi, rsi    /* RDI = MBI pointer (1st param) */
    mov rsi, rax    /* RSI = Magic value (2nd param) */
    
    call kernel_main
    hlt
