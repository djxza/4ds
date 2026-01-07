.section .multiboot
.align 8

multiboot_header:
    .long 0xe85250d6          # magic
    .long 0                  # architecture (i386)
    .long header_end - multiboot_header
    .long -(0xe85250d6 + 0 + (header_end - multiboot_header))

    # Framebuffer REQUEST (type 5)
    .short 5
    .short 0
    .long 20
    .long 0                  # width (any)
    .long 0                  # height (any)
    .long 32                 # bpp

    # END TAG (MANDATORY)
    .short 0
    .short 0
    .long 8

header_end:

.section .text
.global _start
.extern kmain

_start:
    mov %rsp, %rbp           # sanity
    call kmain               # multiboot info ptr already in %rdi
1:
    hlt
    jmp 1b

