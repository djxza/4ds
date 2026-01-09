#ifndef _IO_H
#define _IO_H

#include "stdlib.h"

#define COM1 0x3F8

// PS/2 Keyboard Controller Ports
#define PS2_DATA_PORT 0x60
#define PS2_STATUS_PORT 0x64
#define PS2_COMMAND_PORT 0x64

// Status Register Bits
#define PS2_OUTPUT_FULL 0x01  // Output buffer is full (data available)
#define PS2_INPUT_FULL 0x02   // Input buffer is full (don't write yet)
#define PS2_SYSTEM_FLAG 0x04  // System flag
#define PS2_COMMAND_DATA 0x08 // Command/data (0=data, 1=command)
#define PS2_TIMEOUT_ERR 0x40  // Timeout error
#define PS2_PARITY_ERR 0x80   // Parity error

// Keyboard Commands
#define KEYBOARD_ENABLE 0xAE  // Enable keyboard
#define KEYBOARD_DISABLE 0xAD // Disable keyboard
#define KEYBOARD_RESET 0xFF   // Reset keyboard

// Special Key Codes
#define KEY_ENTER 0x1C
#define KEY_BACKSPACE 0x0E
#define KEY_TAB 0x0F
#define KEY_ESC 0x01
#define KEY_LSHIFT 0x2A
#define KEY_RSHIFT 0x36
#define KEY_LCTRL 0x1D
#define KEY_LALT 0x38
#define KEY_CAPSLOCK 0x3A
#define KEY_F1 0x3B
#define KEY_F2 0x3C
#define KEY_F3 0x3D
#define KEY_F4 0x3E
#define KEY_F5 0x3F
#define KEY_F6 0x40
#define KEY_F7 0x41
#define KEY_F8 0x42
#define KEY_F9 0x43
#define KEY_F10 0x44
#define KEY_F11 0x57
#define KEY_F12 0x58
#define KEY_NUMLOCK 0x45
#define KEY_SCROLLLOCK 0x46
#define KEY_HOME 0x47
#define KEY_UP 0x48
#define KEY_PGUP 0x49
#define KEY_LEFT 0x4B
#define KEY_RIGHT 0x4D
#define KEY_END 0x4F
#define KEY_DOWN 0x50
#define KEY_PGDN 0x51
#define KEY_INS 0x52
#define KEY_DEL 0x53

// Basic I/O functions
void outb(u16 port, u8 value);
u8 inb(u16 port);
void outw(u16 port, u16 value);
u16 inw(u16 port);
void outl(u16 port, u32 value);
u32 inl(u16 port);
void io_wait(void);

void keyboard_send_command(u8 cmd);

// Keyboard functions
void keyboard_init(void);
u8 keyboard_read_scancode(void);
char keyboard_scancode_to_char(u8 scancode, bool shift_pressed);
bool keyboard_is_printable(u8 scancode);
void keyboard_set_leds(bool scroll, bool num, bool caps);

// Serial functions
void printb_str(const char *str);
void printb_char(char c);
void printb_hex(u32 n);
void printb_dec(u32 n);

// Keyboard state variables (now exported)
extern bool shift_pressed;
extern bool ctrl_pressed;
extern bool alt_pressed;
extern bool caps_lock;
extern bool num_lock;
extern bool scroll_lock;

// Helper functions to check keyboard state
static inline bool keyboard_is_shift_pressed(void) { return shift_pressed; }
static inline bool keyboard_is_ctrl_pressed(void) { return ctrl_pressed; }
static inline bool keyboard_is_alt_pressed(void) { return alt_pressed; }
static inline bool keyboard_is_capslock_on(void) { return caps_lock; }

#endif // _IO_H
