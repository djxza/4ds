#include <stdint.h>

void kernel_main() {
  // works
  //((uint32_t *)(0xb8000))[0] = 0xFFAA21;
  uint32_t *pixels = (uint32_t *)(0xa0000);

  for (int i = -320 * 200; i < 320 * 400; ++i)
    pixels[i] = 0x00FFFFFF;
}
