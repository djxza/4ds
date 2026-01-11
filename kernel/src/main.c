#include "../limine-protocol/include/limine.h"
#include <stdbool.h>
#include <stdint.h>

#include "fs/ahci.h"
#include "fs/fat32.h"
#include "fs/ide.h"
#include "fs/qemu_disk.h"
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

// #include "icon.h"

void draw_img(u32 *img, u32 width, u32 height, int x, int y, u32 *pixels) {
  for (int i = 0; i < width; ++i) {
    for (int j = 0; j < height; ++j) {
      putpixel(x + i, y + j, img[j * width + i], pixels);
    }
  }
}

// Kernel entry
void _start(void) {
  // 1. FIRST: Initialize serial port (for debugging)
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 0x03);
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xC7);
  outb(COM1 + 4, 0x0B);

  printb_str("=== 4DS Launcher ===\n");

  // 2. Initialize graphics
  initscr();
  screen_t screen = get_screen();

  printb_str("Screen: ");
  printb_dec(screen.width);
  printb_str("x");
  printb_dec(screen.height);
  printb_str("\n");

  // 3. Initialize system
  memset(&sys, 0, sizeof(sys));

  // 4. Initialize keyboard
  keyboard_init();
  keyboard_set_leds(false, true, false);

  // 5. Initialize disk using IDE (simpler than AHCI)
  printb_str("Initializing disk...\n");
  ide_init();

  // Set up disk operations for IDE
  disk_ops_t disk_ops = {.read_sectors = ide_read_sectors,
                         .write_sectors = ide_write_sectors};

  // Test disk read
  u8 sector_buffer[512];
  memset(sector_buffer, 0, sizeof(sector_buffer));

  if (ide_read_sectors(0, 1, sector_buffer) == 0) {
    printb_str("IDE disk read successful!\n");

    // Check for boot sector signature
    if (sector_buffer[510] == 0x55 && sector_buffer[511] == 0xAA) {
      printb_str("Valid boot sector found\n");

      // Initialize FAT32
      if (fat32_init(&disk_ops) == 0) {
        printb_str("FAT32 initialized successfully\n");

        // List directory
        fat32_list_dir("/");

        // Try to read TEST.TXT
        char file_buffer[1024];
        int file_size =
            fat32_read_file("TEST.TXT", file_buffer, sizeof(file_buffer));

        if (file_size > 0) {
          printb_str("Successfully read TEST.TXT: ");
          printb_dec(file_size);
          printb_str(" bytes\n");

          // Print first part
          file_buffer[64] = '\0';
          printb_str("Content: ");
          printb_str(file_buffer);
          printb_str("\n");
        } else {
          printb_str("TEST.TXT not found or empty\n");
        }
      } else {
        printb_str("FAT32 init failed\n");
      }
    } else {
      printb_str("No boot sector signature\n");
    }
  } else {
    printb_str("IDE disk read failed\n");
  }

  // Check specific FAT32 fields
  fat32_bpb_t *bpb = (fat32_bpb_t *)sector_buffer;
  printb_str("Bytes per sector: ");
  printb_dec(bpb->bytes_per_sector);
  printb_str("\n");
  printb_str("Sectors per cluster: ");
  printb_dec(bpb->sectors_per_cluster);
  printb_str("\n");
  printb_str("Root cluster: ");
  printb_dec(bpb->root_cluster);
  printb_str("\n");

  // TEMPORARILY DISABLE DISK INITIALIZATION - COMMENT THIS OUT
  /*
  // 5. Initialize disk and filesystem
  printb_str("Initializing disk...\n");
  ahci_init();

  // Set up disk operations
  disk_ops_t disk_ops = {.read_sectors = ahci_read_sectors,
                         .write_sectors = ahci_write_sectors};

  // Initialize FAT32
  if (fat32_init(&disk_ops) == 0) {
    printb_str("FAT32 initialized successfully\n");

    // Test: List directory
    fat32_list_dir("/");

    // Test: Try to read TEST.TXT
    char file_buffer[1024];
    int file_size =
        fat32_read_file("TEST.TXT", file_buffer, sizeof(file_buffer));

    if (file_size > 0) {
      printb_str("Successfully read TEST.TXT: ");
      printb_dec(file_size);
      printb_str(" bytes\n");

      // Print first 64 bytes
      file_buffer[64] = '\0'; // Ensure null termination
      printb_str("First 64 chars: ");
      printb_str(file_buffer);
      printb_str("\n");
    } else {
      printb_str("Failed to read TEST.TXT\n");
    }
  } else {
    printb_str("Failed to initialize FAT32\n");
  }
  */

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

  // Initialize apps
  init_apps();

  // Draw something to the screen to verify graphics work
  // Fill screen with gradient
  for (int y = 0; y < screen.height; y++) {
    u32 color = 0xFF0000 + (y * 255 / screen.height) * 0x100; // Red gradient
    for (int x = 0; x < screen.width; x++) {
      putpixel(x, y, color, screen.pixels);
    }
  }

  // Draw text to verify fonts work
  draw_text(100, 100, "4DS Launcher Running!", 0xFFFFFF, 0x000000,
            screen.pixels);

  printb_str("Graphics test complete - UI should be visible\n");

  // MAIN LOOP
  while (1) {
    // Handle keyboard
    keyboard_poll();

    // Check for ESC to exit
    if (keyboard_just_pressed(KEY_ESC)) {
      printb_str("ESC pressed - exiting\n");
      break;
    }

    // Update keyboard state
    keyboard_update();

    // Small delay
    for (volatile int i = 0; i < 1000000; i++)
      ;
  }

  printb_str("Shutting down...\n");
  hang();
}
