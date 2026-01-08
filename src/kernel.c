#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_ADDR 0xB8000

static volatile uint16_t *const vga = (volatile uint16_t *)VGA_ADDR;

static uint16_t cursor = 0;

static inline uint16_t vga_entry(char c, uint8_t color) {
  return (uint16_t)c | ((uint16_t)color << 8);
}

void vga_putc(char c) {
  if (c == '\n') {
    cursor += VGA_WIDTH - (cursor % VGA_WIDTH);
    return;
  }

  vga[cursor++] = vga_entry(c, 0x0F);

  if (cursor >= VGA_WIDTH * VGA_HEIGHT)
    cursor = 0;
}

void vga_print(const char *s) {
  while (*s)
    vga_putc(*s++);
}

void kernel_main64(void) {
  vga_print("LONG MODE OK\n");
  while (1)
    __asm__ volatile("hlt");
}
