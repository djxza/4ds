.section .multiboot
.align 8

.long 0xe85250d6
.long 0
.long header_end - header_start
.long -(0xe85250d6 + 0 + (header_end - header_start))

header_start:

.align 8
.long 5
.long 20
.long 1024
.long 768
.long 32

.align 8
.long 0
.long 8

header_end:

.intel_syntax noprefix
.code64
.global _start
.extern kernel_main64

.section .text
_start:
    cli
    lea rsp, [stack_top]
    mov rdi, rbx          # PASS multiboot_info pointer
    call kernel_main64

.hang:
    hlt
    jmp .hang

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

