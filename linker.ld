/* Kernel linker script - places kernel at 1MB */
OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH(i386:x86-64)
ENTRY(_start)

SECTIONS
{
    /* Load kernel at 1MB (0x100000) */
    . = 0x100000;
    
    /* Kernel start */
    kernel_start = .;
    
    /* Text section (code) */
    .text : ALIGN(4K) {
        *(.text*)
        *(.gnu.linkonce.t*)
    }
    
    /* Read-only data */
    .rodata : ALIGN(4K) {
        *(.rodata*)
        *(.gnu.linkonce.r*)
    }
    
    /* Data section (initialized variables) */
    .data : ALIGN(4K) {
        *(.data*)
        *(.gnu.linkonce.d*)
    }
    
    /* BSS section (uninitialized variables) */
    .bss : ALIGN(4K) {
        *(COMMON)
        *(.bss*)
        *(.gnu.linkonce.b*)
    }
    
    /* Kernel end */
    kernel_end = .;
    
    /* Debug information (discarded in final binary) *
    /DISCARD/ : {
        *(.comment)
        *(.note*)
        *(.eh_frame*)
        *(.interp)
        *(.dynamic)
        *(.dynsym)
        *(.dynstr)
        *(.hash)
        *(.gnu.hash)
    }*/
}
