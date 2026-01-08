.intel_syntax noprefix

#ifndef MULTIBOOT2_HEADER_MAGIC
# define MULTIBOOT2_HEADER_MAGIC 0xe85250d6
#endif

#define mb2_magic  MULTIBOOT2_HEADER_MAGIC
#define mb2_length (mb2_header_end - mb2_header_start)

.section .multiboot
.align 8

mb2_header_start:
    .long mb2_magic
    .long 0                  /* architecture = i386 */
    .long mb2_length
    .long -(mb2_magic + 0 + mb2_length)

    /* -----------------------------
     * End tag
     * ----------------------------- */
    .align 8
    .short 0                 /* type = END */
    .short 0                 /* flags */
    .long 8                  /* size */

mb2_header_end:
