#include <stdint.h>

void kernel_main() {
  uint32_t *vga = (uint32_t *)0xB8000;
  vga[0] = 0x2F4F;
}
