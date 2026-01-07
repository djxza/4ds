.intel_syntax noprefix
.section .multiboot
.align 8

# Mandatory Multiboot2 header
.set MB2_MAGIC,      0xE85250D6
.set MB2_ARCH,       0
.set MB2_HEADER_LEN, (multiboot_end - multiboot_start)
.set MB2_CHECKSUM,   -(MB2_MAGIC + MB2_ARCH + MB2_HEADER_LEN)

.global multiboot_start
multiboot_start:
    .long MB2_MAGIC
    .long MB2_ARCH
    .long MB2_HEADER_LEN
    .long MB2_CHECKSUM

# Framebuffer request (optional)
    .short 5      # type
    .short 0      # flags
    .long 20      # size
    .long 0    # width
    .long 0     # height
    .long 32      # depth

# End tag
    .short 0
    .short 0
    # .long 8 # here

multiboot_end:

