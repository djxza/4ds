.section .multiboot
.align 8

multiboot_header:
    # Fixed header
    .long 0xe85250d6
    .long 0
    .long header_end - multiboot_header
    .long -(0xe85250d6 + 0 + (header_end - multiboot_header))

    # Framebuffer REQUEST tag (type 5, size = 20)
    .short 5
    .short 0
    .long 20
    .long 0
    .long 0
    .long 32

    # REQUIRED padding to 8-byte alignment
    .long 0

    # END tag (type 0, size = 8)
    .short 0
    .short 0
    .long 8

header_end:

.section .text
.global _start
.extern kmain

_start:
    call kmain
.hang:
    hlt
    jmp .hang

