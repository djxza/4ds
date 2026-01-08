#include <stdint.h>

#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER 5
#define COM1 0x3F8
#define VGA_BUFFER 0xB8000

/* Simple VGA functions */
void vga_clear() {
  uint16_t *vga = (uint16_t *)VGA_BUFFER;
  for (int i = 0; i < 80 * 25; i++) {
    vga[i] = 0x0F00 | ' ';
  }
}

void vga_putc(int x, int y, char c, uint8_t color) {
  uint16_t *vga = (uint16_t *)VGA_BUFFER;
  vga[y * 80 + x] = (color << 8) | c;
}

void vga_puts(int x, int y, const char *str, uint8_t color) {
  while (*str) {
    vga_putc(x++, y, *str++, color);
    if (x >= 80) {
      x = 0;
      y++;
    }
  }
}

void vga_print_hex(int x, int y, uint64_t val, uint8_t color) {
  const char *hex = "0123456789ABCDEF";
  for (int i = 0; i < 16; i++) {
    int nibble = (val >> (60 - i * 4)) & 0xF;
    vga_putc(x + i, y, hex[nibble], color);
  }
}

/* Serial I/O */
static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void serial_init() {
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 0x03);
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xC7);
  outb(COM1 + 4, 0x0B);
}

void serial_putchar(char c) {
  while (!(inb(COM1 + 5) & 0x20))
    ;
  outb(COM1, c);
}

void serial_print(const char *s) {
  while (*s)
    serial_putchar(*s++);
}

/* Try to detect Multiboot2 info structure */
struct mb2_header {
  uint32_t total_size;
  uint32_t reserved;
};

void *find_multiboot2_info(uint64_t start_addr, uint64_t end_addr) {
  // Look for the multiboot2 magic pattern
  for (uint64_t addr = start_addr; addr < end_addr; addr += 8) {
    uint32_t *magic_ptr = (uint32_t *)addr;
    if (*magic_ptr == 0x36d76289) {
      // Found magic, check if it looks like a valid structure
      struct mb2_header *header = (struct mb2_header *)(addr + 8);
      if (header->total_size > 16 && header->total_size < 0x10000) {
        return (void *)addr;
      }
    }
  }
  return 0;
}

/* Direct VGA graphics mode (mode 0x13) as fallback */
void init_vga_graphics() {
  // Try to set 320x200 256-color mode through VGA registers
  outb(0x3C2, 0xE3); // Misc Output Register
  outb(0x3D4, 0x11); // Sequencer
  outb(0x3D5, 0x00);

  // More registers would be needed for full mode set...
  // For now, just use text mode
}

/* Main kernel */
void kernel_main(uint64_t mbi_ptr, uint64_t magic) {
  // Initialize VGA text immediately
  vga_clear();
  vga_puts(0, 0, "4DS OS Kernel Started", 0x0F);

  vga_puts(0, 1, "Magic: ", 0x0E);
  vga_print_hex(7, 1, magic, 0x0E);

  vga_puts(0, 2, "MBI Ptr: ", 0x0E);
  vga_print_hex(9, 2, mbi_ptr, 0x0E);

  // Initialize serial for debugging
  serial_init();
  serial_print("\n\n=== 4DS OS ===\n");

  void *mbi = 0;

  // Try different approaches to find the multiboot2 info
  if (magic == 0x36d76289 || magic == 0x2BADB002) {
    // Standard multiboot magic
    mbi = (void *)mbi_ptr;
    vga_puts(0, 3, "Using direct MBI pointer", 0x0A);
  } else {
    vga_puts(0, 3, "Scanning memory for MBI...", 0x0C);

    // Try to find it in memory
    mbi = find_multiboot2_info(0x100000, 0x200000);
    if (!mbi)
      mbi = find_multiboot2_info(0x2000, 0xA0000);

    if (mbi) {
      vga_puts(0, 4, "Found MBI at: ", 0x0A);
      vga_print_hex(14, 4, (uint64_t)mbi, 0x0A);
    } else {
      vga_puts(0, 4, "No MBI found!", 0x0C);
    }
  }

  if (mbi) {
    vga_puts(0, 5, "MBI Magic: ", 0x0E);
    uint32_t actual_magic = *(uint32_t *)mbi;
    vga_print_hex(11, 5, actual_magic, 0x0E);

    if (actual_magic == 0x36d76289) {
      vga_puts(0, 6, "Valid Multiboot2!", 0x0A);

      // Parse tags
      uint8_t *ptr = (uint8_t *)mbi + 8;
      int y = 7;

      while (y < 20) {
        uint32_t *tag = (uint32_t *)ptr;
        if (tag[0] == 0)
          break; // End tag

        vga_puts(0, y, "Tag: ", 0x0E);
        vga_print_hex(5, y, tag[0], 0x0E);

        if (tag[0] == 5) { // Framebuffer tag
          vga_puts(25, y, "FB", 0x0A);
          // We found framebuffer!
        }

        // Move to next tag
        uint32_t size = tag[1];
        ptr += (size + 7) & ~7;
        y++;
      }
    }
  }

  // Direct VGA color test - always works
  vga_puts(0, 22, "VGA Color Test:", 0x0F);

  // Draw color bars in text mode
  for (int x = 0; x < 80; x++) {
    uint8_t color = (x % 16);
    for (int y = 23; y < 25; y++) {
      vga_putc(x, y, 0xDB, color); // Block character
    }
  }

  // Animate colors
  uint32_t frame = 0;
  while (1) {
    // Update frame counter
    vga_puts(70, 0, "Frame:", 0x0F);
    vga_print_hex(76, 0, frame, 0x0F);

    // Animate color bars
    for (int x = 0; x < 80; x++) {
      uint8_t color = ((x + frame) % 16);
      vga_putc(x, 23, 0xDB, color);
      vga_putc(x, 24, 0xDB, (color + 8) % 16);
    }

    frame++;

    // Simple delay
    for (volatile uint32_t i = 0; i < 1000000; i++)
      ;
  }
}
