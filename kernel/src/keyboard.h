#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "stdlib.h"

// Key state tracking structure
typedef struct {
  bool pressed;       // Is the key currently pressed?
  bool just_pressed;  // Was the key just pressed this frame?
  bool just_released; // Was the key just released this frame?
} key_state_t;

// Initialize keyboard
void keyboard_init(void);

// Poll keyboard for input (call this regularly)
void keyboard_poll(void);

// Check if a specific key is currently pressed
bool keyboard_is_pressed(u8 keycode);

// Check if a key was just pressed (one-time check)
bool keyboard_just_pressed(u8 keycode);

// Check if a key was just released (one-time check)
bool keyboard_just_released(u8 keycode);

// Update keyboard state (call this at the end of each frame)
void keyboard_update(void);

#endif // _KEYBOARD_H
