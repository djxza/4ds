#include "keyboard.h"
#include "io.h"
#include "stdlib.h"
#include "string.h"

// Key states for all possible keys (0-127)
static key_state_t key_states[128];
static key_state_t prev_key_states[128];

// Initialize keyboard
void keyboard_init(void) {
  printb_str("Initializing keyboard...\n");

  // Clear all key states
  memset(key_states, 0, sizeof(key_states));
  memset(prev_key_states, 0, sizeof(prev_key_states));

  // Simple PS/2 initialization
  keyboard_send_command(KEYBOARD_ENABLE);

  // Clear any pending data
  while (inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) {
    inb(PS2_DATA_PORT);
  }

  printb_str("Keyboard ready\n");
}

// Poll keyboard for input
void keyboard_poll(void) {
  if (inb(PS2_STATUS_PORT) & PS2_OUTPUT_FULL) {
    u8 scancode = inb(PS2_DATA_PORT);

    // Debug: print scancode
    printb_str("Scancode: 0x");
    printb_hex(scancode);
    printb_str("\n");

    // Check for key release (bit 7 set)
    bool released = scancode & 0x80;
    u8 keycode = scancode & 0x7F;

    if (released) {
      // Key released
      key_states[keycode].pressed = false;
      key_states[keycode].just_released = true;
      printb_str("Key released: 0x");
      printb_hex(keycode);
      printb_str("\n");
    } else {
      // Key pressed
      key_states[keycode].pressed = true;
      key_states[keycode].just_pressed = true;
      printb_str("Key pressed: 0x");
      printb_hex(keycode);
      printb_str("\n");

      // Update modifier keys
      switch (keycode) {
      case KEY_LSHIFT:
      case KEY_RSHIFT:
        shift_pressed = true;
        break;
      case KEY_LCTRL:
        ctrl_pressed = true;
        break;
      case KEY_LALT:
        alt_pressed = true;
        break;
      case KEY_CAPSLOCK:
        caps_lock = !caps_lock;
        keyboard_set_leds(scroll_lock, num_lock, caps_lock);
        break;
      }
    }
  }
}

// Check if a key is currently pressed
bool keyboard_is_pressed(u8 keycode) {
  if (keycode >= 128)
    return false;
  return key_states[keycode].pressed;
}

// Check if a key was just pressed
bool keyboard_just_pressed(u8 keycode) {
  if (keycode >= 128)
    return false;
  return key_states[keycode].just_pressed;
}

// Check if a key was just released
bool keyboard_just_released(u8 keycode) {
  if (keycode >= 128)
    return false;
  return key_states[keycode].just_released;
}

// Update keyboard state (call at end of frame)
void keyboard_update(void) {
  // Copy current states to previous states
  memcpy(prev_key_states, key_states, sizeof(key_states));

  // Clear the "just" flags for next frame
  for (int i = 0; i < 128; i++) {
    key_states[i].just_pressed = false;
    key_states[i].just_released = false;
  }
}
