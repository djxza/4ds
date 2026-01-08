.intel_syntax noprefix

.section .multiboot
.align 8
mb2_header_start:
    .long 0xe85250d6
    .long 0
    .long mb2_header_end - mb2_header_start
    .long -(0xe85250d6 + 0 + (mb2_header_end - mb2_header_start))

    # framebuffer request tag
    # .align 8
    # .short 5
    # .short 0
    # .long 20
    # .long 1024
    # .long 768
    # .long 32

    # end tag
    .align 8
    .short 0
    .short 0
    .long 8
mb2_header_end:
