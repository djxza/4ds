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
.long 0
.long 0
.long 32

.align 8
.long 0
.long 8

header_end:

.section .text
.global _start
# .code32

_start:
    cli
    mov $stack_top, %esp

    push %ebx        # multiboot info
    push %eax        # magic

    call enter_long_mode
    hlt

# -----------------------
# Stack
# -----------------------
.section .bss
.align 16
stack_bottom:
    .skip 16384      # 16 KiB stack
stack_top:

