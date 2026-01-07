.intel_syntax noprefix
.global _start
.extern kmain

_start:
    mov rdi, rdi     # rdi already contains multiboot_info
    call _main

.hang:
    hlt
    jmp .hang

