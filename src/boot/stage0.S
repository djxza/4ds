.code16
.global start
start:
    cli
    movw $0x00, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7c00, %sp
    movw $msg, %si
    sti

print:
    lodsb
    cmpb $0, %al
    je done
    movb $0x0E, %ah
    int $0x10
    jmp print

done:
    cli
    hlt

msg:
    .asciz "Hello World!"
    .byte 0

.org 0x7c00 + 510
.word 0xAA55
