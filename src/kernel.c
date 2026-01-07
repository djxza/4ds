#include <stdint.h>

void kernel_main64(void) {
  uint32_t *fb = (uint32_t *)0x00000000E0000000;

  for (uint32_t i = 0; i < 800 * 600; i++)
    fb[i] = 0x00FF0000;

  for (;;)
    __asm__("hlt");
}
