#include "../limine-protocol/include/limine.h"
#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"
#include "io.h"
#include "keyboard.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "ui/home.h"

system_t sys;

// Simple keyboard handling
void handle_keyboard(void) {
  // Poll keyboard
  keyboard_poll();

  // Check for arrow keys
  if (keyboard_just_pressed(KEY_UP)) {
    sys.home.selected_tile = (sys.home.selected_tile - sys.home.cols);
    if (sys.home.selected_tile < 0) {
      sys.home.selected_tile += sys.home.app_count;
    }
    printb_str("Up pressed - Selected: ");
    printb_dec(sys.home.selected_tile);
    printb_str("\n");
  }

  if (keyboard_just_pressed(KEY_DOWN)) {
    sys.home.selected_tile =
        (sys.home.selected_tile + sys.home.cols) % sys.home.app_count;
    printb_str("Down pressed - Selected: ");
    printb_dec(sys.home.selected_tile);
    printb_str("\n");
  }

  if (keyboard_just_pressed(KEY_LEFT)) {
    sys.home.selected_tile = (sys.home.selected_tile - 1);
    if (sys.home.selected_tile < 0) {
      sys.home.selected_tile += sys.home.app_count;
    }
    printb_str("Left pressed - Selected: ");
    printb_dec(sys.home.selected_tile);
    printb_str("\n");
  }

  if (keyboard_just_pressed(KEY_RIGHT)) {
    sys.home.selected_tile = (sys.home.selected_tile + 1) % sys.home.app_count;
    printb_str("Right pressed - Selected: ");
    printb_dec(sys.home.selected_tile);
    printb_str("\n");
  }

  if (keyboard_just_pressed(KEY_ENTER)) {
    printb_str("Enter pressed - Launching: ");
    printb_str(sys.home.apps[sys.home.selected_tile].name);
    printb_str("\n");
  }

  if (keyboard_just_pressed(KEY_ESC)) {
    printb_str("Escape pressed\n");
  }

  // Test regular keys
  for (u8 key = 'A'; key <= 'Z'; key++) {
    if (keyboard_just_pressed(key)) {
      printb_str("Key pressed: ");
      printb_char(key);
      printb_str("\n");
    }
  }

  for (u8 key = '1'; key <= '9'; key++) {
    if (keyboard_just_pressed(key)) {
      printb_str("Key pressed: ");
      printb_char(key);
      printb_str("\n");
    }
  }
}

// Kernel entry
void _start(void) {
  // Initialize serial
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 0x03);
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xC7);
  outb(COM1 + 4, 0x0B);

  // Initialize graphics
  initscr();
  screen_t screen = get_screen();

  printb_str("=== 4DS Launcher ===\n");
  printb_str("Screen: ");
  printb_dec(screen.width);
  printb_str("x");
  printb_dec(screen.height);
  printb_str("\n");

  // Initialize system
  memset(&sys, 0, sizeof(sys));

  // Initialize keyboard
  keyboard_init();
  keyboard_set_leds(false, true, false);

  // UI Configuration
  sys.home.cols = 4;
  sys.home.rows = 2;
  sys.home.app_count = 8;
  sys.home.selected_tile = 0;

  // Theme
  sys.home.theme.bg_top = 0x00F8FCFF;
  sys.home.theme.bg_bottom = 0x00E0E8F0;
  sys.home.theme.tile = 0x0099CCFF;
  sys.home.theme.tile_light = 0x00CCEEFF;
  sys.home.theme.tile_dark = 0x006699CC;
  sys.home.theme.shadow = 0x00404040;
  sys.home.theme.header = 0x00FFFFFF;
  sys.home.theme.header_shadow = 0x00E0E0E0;
  sys.home.theme.icon_bg = 0x00FFFFFF;
  sys.home.theme.text_color = 0x00333333;
  sys.home.theme.text_shadow = 0x00AAAAAA;
  sys.home.theme.highlight = 0x0066CCFF;
  sys.home.theme.notification = 0x00FF3333;

  // Initialize apps (you need to complete init_apps in home.c)
  init_apps();

  // Draw initial screen
  draw_home_screen(screen.pixels);

  printb_str("Ready. Press arrow keys to navigate, Enter to select.\n");

  int last_selected = -1;

  while (1) {
    // Handle keyboard input
    handle_keyboard();

    // Update keyboard state for next frame
    keyboard_update();

    if (keyboard_read_scancode() == KEY_LSHIFT) {
      printb_str("AA");
    }

    // Redraw if selection changed
    if (sys.home.selected_tile != last_selected) {
      draw_home_screen(screen.pixels);
      last_selected = sys.home.selected_tile;
    }

    // Small delay
    for (volatile int i = 0; i < 10000; i++)
      ;
  }
}
