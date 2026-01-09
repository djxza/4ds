#include "io.h"

// Basic port I/O
void outb(u16 port, u8 value) {
  __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

u8 inb(u16 port) {
  uint8_t ret;
  __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void outw(u16 port, u16 value) {
  __asm__ volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

u16 inw(u16 port) {
  uint16_t ret;
  __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

void outl(u16 port, u32 value) {
  __asm__ volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

u32 inl(u16 port) {
  uint32_t ret;
  __asm__ volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

// Small delay for I/O operations
void io_wait(void) { outb(0x80, 0); }

// ========= KEYBOARD IMPLEMENTATION =========

// REMOVE THE 'static' KEYWORD - these variables are now shared
bool shift_pressed = false;
bool ctrl_pressed = false;
bool alt_pressed = false;
bool caps_lock = false;
bool num_lock = false;
bool scroll_lock = false;

// Wait for keyboard input buffer to be ready
static void keyboard_wait_write(void) {
  while (inb(PS2_STATUS_PORT) & PS2_INPUT_FULL)
    io_wait();
}

// Wait for keyboard output buffer to have data
static void keyboard_wait_read(void) {
  while (!(inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL))
    io_wait();
}

// Send command to keyboard controller
void keyboard_send_command(u8 cmd) {
  keyboard_wait_write();
  outb(PS2_COMMAND_PORT, cmd);
}

// Send command to keyboard (not controller)
static void keyboard_send_data(u8 data) {
  keyboard_wait_write();
  outb(PS2_DATA_PORT, data);
}

// In io.c, update keyboard_init:/*
/*void keyboard_init(void) {
  printb_str("Starting keyboard initialization...\n");

  // Disable keyboard
  keyboard_send_command(KEYBOARD_DISABLE);
  io_wait();

  // Clear any pending data
  while (inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) {
    u8 data = inb(PS2_DATA_PORT);
    printb_str("Clearing garbage: 0x");
    printb_hex(data);
    printb_str("\n");
    io_wait();
  }

  // Enable keyboard
  keyboard_send_command(KEYBOARD_ENABLE);
  io_wait();

  // Send reset command
  keyboard_send_data(0xFF); // Reset command
  io_wait();

  // Wait for response
  int timeout = 100000;
  while (!(inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) && timeout-- > 0) {
    io_wait();
  }

  if (timeout > 0) {
    u8 response = inb(PS2_DATA_PORT);
    printb_str("Reset response: 0x");
    printb_hex(response);
    printb_str("\n");

    if (response == 0xFA) {
      // Wait for success code
      timeout = 100000;
      while (!(inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) && timeout-- > 0) {
        io_wait();
      }
      if (timeout > 0) {
        u8 success = inb(PS2_DATA_PORT);
        printb_str("Reset success: 0x");
        printb_hex(success);
        printb_str("\n");
      }
    }
  }*

// Enable scanning
keyboard_send_data(0xF4);
io_wait();

timeout = 100000;
while (!(inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) && timeout-- > 0) {
  io_wait();
}

if (timeout > 0) {
  u8 ack = inb(PS2_DATA_PORT);
  printb_str("Keyboard ACK: 0x");
  printb_hex(ack);
  printb_str("\n");
} else {
  printb_str("Keyboard timeout - assuming it's working\n");
}

printb_str("Keyboard initialization complete\n");
}*/

// Read a scancode from keyboard
u8 keyboard_read_scancode(void) {
  keyboard_wait_read();
  return inb(PS2_DATA_PORT);
}

// Set keyboard LED states
void keyboard_set_leds(bool scroll, bool num, bool caps) {
  keyboard_send_data(0xED); // Command to set LEDs
  keyboard_wait_read();
  inb(PS2_DATA_PORT); // Read ACK

  u8 led_state = 0;
  if (scroll)
    led_state |= 0x01;
  if (num)
    led_state |= 0x02;
  if (caps)
    led_state |= 0x04;

  keyboard_send_data(led_state);
  keyboard_wait_read();
  inb(PS2_DATA_PORT); // Read ACK

  scroll_lock = scroll;
  num_lock = num;
  caps_lock = caps;
}

// Check if scancode is for a printable character
bool keyboard_is_printable(u8 scancode) {
  // Printable scancodes range from 0x02 to 0x53 (excluding special keys)
  if (scancode >= 0x02 && scancode <= 0x0D)
    return true; // 1-0 and symbols
  if (scancode >= 0x10 && scancode <= 0x1B)
    return true; // Q-P and symbols
  if (scancode >= 0x1E && scancode <= 0x26)
    return true; // A-L and symbols
  if (scancode >= 0x2C && scancode <= 0x32)
    return true; // Z-M and symbols
  if (scancode >= 0x39 && scancode <= 0x3A)
    return true; // Space and Caps
  if (scancode == 0x0E)
    return true; // Backspace (we treat as printable for editing)
  if (scancode == 0x1C)
    return true; // Enter
  if (scancode == 0x0F)
    return true; // Tab

  return false;
}

// Convert scancode to character (ASCII)
char keyboard_scancode_to_char(u8 scancode, bool shift_pressed) {
  // US QWERTY keyboard layout
  static const char normal_map[] = {
      0,   0,   '1',  '2', '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', 0x08, 0,   'q',  'w', 'e', 'r',  't', 'y', 'u', 'i',
      'o', 'p', '[',  ']', '\n', 0,   'a', 's',  'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ';', '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v',
      'b', 'n', 'm',  ',', '.',  '/', 0,   '*',  0,   ' '};

  static const char shift_map[] = {
      0,   0,   '!',  '@', '#',  '$', '%', '^', '&', '*', '(', ')',
      '_', '+', 0x08, 0,   'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
      'O', 'P', '{',  '}', '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
      'J', 'K', 'L',  ':', '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
      'B', 'N', 'M',  '<', '>',  '?', 0,   '*', 0,   ' '};

  static const char caps_map[] = {
      0,   0,   '1',  '2', '3',  '4', '5', '6',  '7', '8', '9', '0',
      '-', '=', 0x08, 0,   'Q',  'W', 'E', 'R',  'T', 'Y', 'U', 'I',
      'O', 'P', '[',  ']', '\n', 0,   'A', 'S',  'D', 'F', 'G', 'H',
      'J', 'K', 'L',  ';', '\'', '`', 0,   '\\', 'Z', 'X', 'C', 'V',
      'B', 'N', 'M',  ',', '.',  '/', 0,   '*',  0,   ' '};

  static const char caps_shift_map[] = {
      0,   0,   '!',  '@', '#',  '$', '%', '^', '&', '*', '(', ')',
      '_', '+', 0x08, 0,   'q',  'w', 'e', 'r', 't', 'y', 'u', 'i',
      'o', 'p', '{',  '}', '\n', 0,   'a', 's', 'd', 'f', 'g', 'h',
      'j', 'k', 'l',  ':', '"',  '~', 0,   '|', 'z', 'x', 'c', 'v',
      'B', 'N', 'M',  '<', '>',  '?', 0,   '*', 0,   ' '};

  if (scancode >= sizeof(normal_map))
    return 0;

  if (caps_lock) {
    if (shift_pressed) {
      return caps_shift_map[scancode];
    } else {
      return caps_map[scancode];
    }
  } else {
    if (shift_pressed) {
      return shift_map[scancode];
    } else {
      return normal_map[scancode];
    }
  }
}

// ========= SERIAL OUTPUT =========

void printb_str(const char *str) {
  for (; *str; ++str) {
    outb(COM1, *str);
  }
}

void printb_char(char c) { outb(COM1, c); }

void printb_hex(u32 n) {
  char buf[9];
  const char *hex = "0123456789ABCDEF";

  for (int i = 7; i >= 0; i--) {
    buf[i] = hex[n & 0xF];
    n >>= 4;
  }
  buf[8] = '\0';

  printb_str("0x");
  printb_str(buf);
}

void printb_dec(u32 n) {
  char buf[11];
  int i = 0;

  if (n == 0) {
    buf[i++] = '0';
  } else {
    while (n > 0) {
      buf[i++] = '0' + (n % 10);
      n /= 10;
    }
  }
  buf[i] = '\0';

  // Reverse the string
  for (int j = 0; j < i / 2; j++) {
    char temp = buf[j];
    buf[j] = buf[i - j - 1];
    buf[i - j - 1] = temp;
  }

  printb_str(buf);
}
