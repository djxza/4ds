#include <stdint.h>

#include "framebuffer.h"

void _main(void *multiboot_info) {
  if (!framebuffer_init(multiboot_info)) {
    // Framebuffer not found: hang forever
    while (1) {
      __asm__ volatile("hlt");
    }
  }

  // Clear screen to dark blue
  framebuffer_clear(0xFFFFFFFF);

  // Draw a few test pixels
  framebuffer_put_pixel(50, 50, 0x00FF0000); // red
  framebuffer_put_pixel(51, 50, 0x0000FF00); // green
  framebuffer_put_pixel(
      52, 50,
      0x000000FF); // blue
                   //
                   // // Pseudo debug: paint first row to see bytes per pixel
  for (uint32_t y = 0; y < get_fb_height(); y++) {
    for (uint32_t x = 0; x < get_fb_width(); x++) {
      // simple gradient
      uint32_t r = x * 255 / get_fb_width();
      uint32_t g = y * 255 / get_fb_height();
      uint32_t b = 128;
      framebuffer_put_pixel(x, y, (r << 16) | (g << 8) | b);
    }
  }

  framebuffer_put_pixel(10, 10, 0xFFFF0000); // bright red
                                             //
  unsigned int *vga = (unsigned int *)0xB8000;

  vga[0] = 0xFF00FFFF;

  while (1) {
    __asm__ volatile("hlt");
  }

  framebuffer_put_pixel(10, 10, 0xFFFF0000); // bright red
}
