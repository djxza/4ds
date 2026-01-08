.intel_syntax noprefix

.section .multiboot
.align 8
mb2_header_start:
    .long 0xe85250d6
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(0xe85250d6 + 0 + (mb2_header_end - mb2_header_start))

    # framebuffer request tag
    .align 8
    .short 5
    .short 0
    .long 20
    .long 1024
    .long 768
    .long 32

    # end tag
    .align 8
    .short 0
    .short 0
    .long 8
mb2_header_end:

# -------------------------------------------------

.section .text
.code64
.global _start
.extern kernel_main64

_start:
    cli
    lea rsp, [stack_top]

    # -------------------------------------------------
    # Simple VGA test: write 'OK' in top-left corner
    # -------------------------------------------------

    mov rax, 0xB8000          # VGA text memory
    mov word ptr [rax], 0x2F4F   # 'O' (0x4F) green on black
    mov word ptr [rax+2], 0x2F4B # 'K' (0x4B)

    # -------------------------------------------------
    # Multiboot2 info pointer is in RBX
    # Pass it to kernel_main64(mb_info)
    # -------------------------------------------------
    mov rdi, rbx
    //call kernel_main64

.hang:
    hlt
    jmp .hang

# -------------------------------------------------

.section .bss
.align 16
stack_bottom:
    .skip 16384
stack_top:

