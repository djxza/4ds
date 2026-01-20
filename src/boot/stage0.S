.code16

/* Define segment offsets for code and data */
.equ CODE_OFFSET, 0x8    /* Set the code segment offset to 0x8 */
.equ DATA_OFFSET, 0x10   /* Set the data segment offset to 0x10 */

.text

.global _start
_start:
    cli
    movw $0x00, %ax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %ss
    movw $0x7c00, %sp
    movw $msg, %si
    sti
    jmp print

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
