/* Simple 64-bit kernel with VGA text output */
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;

typedef uint64_t size_t;

/* VGA text buffer */
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

static uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
static uint8_t cursor_x = 0;
static uint8_t cursor_y = 0;

/* Terminal colors */
enum vga_color {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15,
};

/* Create color byte from foreground and background */
static inline uint8_t vga_entry_color(enum vga_color fg, enum vga_color bg) {
  return fg | (bg << 4);
}

/* Create VGA entry from character and color */
static inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | ((uint16_t)color << 8);
}

/* Clear the screen */
void clear_screen(uint8_t color) {
  for (size_t y = 0; y < VGA_HEIGHT; y++) {
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      const size_t index = y * VGA_WIDTH + x;
      vga_buffer[index] = vga_entry(' ', color);
    }
  }
  cursor_x = 0;
  cursor_y = 0;
}

/* Put a character at specific position */
void put_char_at(char c, uint8_t color, size_t x, size_t y) {
  const size_t index = y * VGA_WIDTH + x;
  vga_buffer[index] = vga_entry(c, color);
}

/* Write a character to the terminal */
void put_char(char c, uint8_t color) {
  if (c == '\n') {
    cursor_x = 0;
    cursor_y++;
  } else if (c == '\r') {
    cursor_x = 0;
  } else if (c == '\t') {
    cursor_x = (cursor_x + 4) & ~(4 - 1);
  } else {
    put_char_at(c, color, cursor_x, cursor_y);
    cursor_x++;
  }

  if (cursor_x >= VGA_WIDTH) {
    cursor_x = 0;
    cursor_y++;
  }

  /* Scroll if needed */
  if (cursor_y >= VGA_HEIGHT) {
    for (size_t y = 1; y < VGA_HEIGHT; y++) {
      for (size_t x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(y - 1) * VGA_WIDTH + x] = vga_buffer[y * VGA_WIDTH + x];
      }
    }

    /* Clear last line */
    for (size_t x = 0; x < VGA_WIDTH; x++) {
      put_char_at(' ', color, x, VGA_HEIGHT - 1);
    }

    cursor_y = VGA_HEIGHT - 1;
  }
}

/* Write a string */
void write_string(const char *str, uint8_t color) {
  while (*str) {
    put_char(*str++, color);
  }
}

/* Kernel entry point */
void _start(void) {
  /* Clear screen with blue background */
  clear_screen(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));

  /* Write welcome message */
  write_string("64-bit Kernel Booted Successfully!\n",
               vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLUE));
  write_string("===================================\n",
               vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLUE));
  write_string("System is running in 64-bit mode\n",
               vga_entry_color(VGA_COLOR_GREEN, VGA_COLOR_BLUE));
  write_string("VGA text mode: 80x25\n",
               vga_entry_color(VGA_COLOR_CYAN, VGA_COLOR_BLUE));
  write_string("Kernel loaded at: 0x100000\n",
               vga_entry_color(VGA_COLOR_MAGENTA, VGA_COLOR_BLUE));
  write_string("\nHalted. System is stable.\n",
               vga_entry_color(VGA_COLOR_RED, VGA_COLOR_BLUE));

  /* Halt the CPU */
  asm volatile("cli");
  while (1) {
    asm volatile("hlt");
  }
}
