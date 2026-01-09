// ===== ./kernel/src/ui/home.c (rename to ui.c) =====
#include "home.h"

// system_t sys;

void draw_icon(int x, int y, int size, int icon_type, u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int center_x = x + size / 2;
  int center_y = y + size / 2;

  // Icon background circle
  for (int i = -size / 3; i <= size / 3; i++) {
    for (int j = -size / 3; j <= size / 3; j++) {
      if (i * i + j * j <= (size / 3) * (size / 3)) {
        putpixel(center_x + i, center_y + j, t->icon_bg, pixels);
      }
    }
  }

  // Draw icon based on type
  switch (icon_type) {
  case 0: // App 1
    fill_rect(center_x - 12, center_y - 8, 24, 16, 0xFFFFFF, pixels);
    putpixel(center_x - 6, center_y - 4, 0x000000, pixels);
    putpixel(center_x + 6, center_y - 4, 0x000000, pixels);
    putpixel(center_x, center_y + 4, 0x000000, pixels);
    break;
  case 1: // App 2
    fill_rect(center_x - 2, center_y - 10, 4, 20, 0xFFFFFF, pixels);
    fill_rect(center_x - 10, center_y - 2, 20, 4, 0xFFFFFF, pixels);
    break;
  case 2: // App 3
    for (int i = -size / 4; i <= size / 4; i++) {
      for (int j = -size / 4; j <= size / 4; j++) {
        if (i * i + j * j <= (size / 4) * (size / 4)) {
          putpixel(center_x + i, center_y + j, 0xFFFFFF, pixels);
        }
      }
    }
    fill_rect(center_x - size / 4, center_y, size / 2, 1, 0x000000, pixels);
    fill_rect(center_x, center_y - size / 4, 1, size / 2, 0x000000, pixels);
    break;
  case 3: // App 4
    fill_rect(center_x - 10, center_y - 6, 20, 12, 0xFFFFFF, pixels);
    break;
  case 4: // App 5
    for (int i = 0; i < 12; i++) {
      int start = -i;
      int end = i;
      for (int j = start; j <= end; j++) {
        putpixel(center_x + j, center_y - 6 + i, 0xFFFFFF, pixels);
      }
    }
    break;
  case 5: // App 6
    for (int i = -6; i <= 6; i++) {
      for (int j = -6; j <= 6; j++) {
        if (i * i + j * j <= 36) {
          putpixel(center_x - 8 + i, center_y - 4 + j, 0xFFFFFF, pixels);
          putpixel(center_x + 8 + i, center_y - 4 + j, 0xFFFFFF, pixels);
        }
      }
    }
    break;
  case 6: // System
    fill_rect(center_x - 2, center_y - 10, 4, 20, 0xFFFFFF, pixels);
    fill_rect(center_x - 10, center_y - 2, 20, 4, 0xFFFFFF, pixels);
    break;
  }
}

void draw_notification_badge(int x, int y, u32 *pixels) {
  theme_t *t = &sys.home.theme;

  // Red notification circle
  for (int i = -6; i <= 6; i++) {
    for (int j = -6; j <= 6; j++) {
      if (i * i + j * j <= 36) {
        putpixel(x + i, y + j, t->notification, pixels);
      }
    }
  }

  // White "1" inside
  fill_rect(x - 1, y - 3, 2, 6, 0xFFFFFF, pixels);
  fill_rect(x - 2, y + 3, 4, 2, 0xFFFFFF, pixels);
}

void draw_tile(int x, int y, int w, int h, bool selected, app_t *app,
               u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int radius = 12;

  // Shadow with offset
  fill_rounded_rect(x + 8, y + 8, w, h, radius, t->shadow, pixels);

  // Main tile with gradient
  for (int yy = 0; yy < h; yy++) {
    u32 tile_color = lerp_color(
        app->color, selected ? t->highlight : t->tile_dark, yy * 2, h * 3);

    for (int xx = 0; xx < w; xx++) {
      // Rounded corners check
      bool in_corner = false;
      if (yy < radius && xx < radius) {
        int dx = radius - xx;
        int dy = radius - yy;
        if (dx * dx + dy * dy > radius * radius)
          in_corner = true;
      }
      if (yy < radius && xx >= w - radius) {
        int dx = xx - (w - radius);
        int dy = radius - yy;
        if (dx * dx + dy * dy > radius * radius)
          in_corner = true;
      }
      if (yy >= h - radius && xx < radius) {
        int dx = radius - xx;
        int dy = yy - (h - radius);
        if (dx * dx + dy * dy > radius * radius)
          in_corner = true;
      }
      if (yy >= h - radius && xx >= w - radius) {
        int dx = xx - (w - radius);
        int dy = yy - (h - radius);
        if (dx * dx + dy * dy > radius * radius)
          in_corner = true;
      }

      if (!in_corner) {
        putpixel(x + xx, y + yy, tile_color, pixels);
      }
    }
  }

  // Top glossy highlight
  for (int yy = 0; yy < h / 6; yy++) {
    u32 highlight_color = lerp_color(t->tile_light, 0x00FFFFFF, yy, h / 6);
    for (int xx = radius; xx < w - radius; xx++) {
      putpixel(x + xx, y + yy + 2, highlight_color, pixels);
    }
  }

  // Icon
  draw_icon(x + w / 2, y + h / 3, 48, app->icon_type, pixels);

  // App name
  draw_text(x + w / 2 - (strlen(app->name) * 12), y + h - 40, app->name,
            t->text_color, t->text_shadow, pixels);

  // Notification badge
  if (app->has_notification) {
    draw_notification_badge(x + w - 20, y + 20, pixels);
  }

  // Wii U-style selection highlight (thicker, corner-only)
  if (selected) {
    // Bright blue color for Wii U style
    u32 wiiu_blue = 0x0066CCFF;
    int border_thickness = 6; // Much thicker than before
    int corner_length = 40;   // Length of each corner segment

    // Top-left corner (__ )
    for (int i = 0; i < border_thickness; i++) {
      // Horizontal part (top)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x + j, y - border_thickness + i, wiiu_blue, pixels);
      }
      // Vertical part (left)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x - border_thickness + i, y + j, wiiu_blue, pixels);
      }
    }

    // Top-right corner ( __)
    for (int i = 0; i < border_thickness; i++) {
      // Horizontal part (top)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x + w - 1 - j, y - border_thickness + i, wiiu_blue, pixels);
      }
      // Vertical part (right)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x + w - 1 + border_thickness - i, y + j, wiiu_blue, pixels);
      }
    }

    // Bottom-left corner (|__ )
    for (int i = 0; i < border_thickness; i++) {
      // Horizontal part (bottom)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x + j, y + h - 1 + border_thickness - i, wiiu_blue, pixels);
      }
      // Vertical part (left)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x - border_thickness + i, y + h - 1 - j, wiiu_blue, pixels);
      }
    }

    // Bottom-right corner ( __|)
    for (int i = 0; i < border_thickness; i++) {
      // Horizontal part (bottom)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x + w - 1 - j, y + h - 1 + border_thickness - i, wiiu_blue,
                 pixels);
      }
      // Vertical part (right)
      for (int j = 0; j < corner_length; j++) {
        putpixel(x + w - 1 + border_thickness - i, y + h - 1 - j, wiiu_blue,
                 pixels);
      }
    }

    // Add small diagonal connectors in corners (makes it look more connected)
    for (int i = 0; i < border_thickness; i++) {
      // Top-left diagonal connector
      for (int j = 0; j < border_thickness; j++) {
        putpixel(x + corner_length + j, y - border_thickness + i, wiiu_blue,
                 pixels);
        putpixel(x - border_thickness + i, y + corner_length + j, wiiu_blue,
                 pixels);
      }

      // Top-right diagonal connector
      for (int j = 0; j < border_thickness; j++) {
        putpixel(x + w - 1 - corner_length - j, y - border_thickness + i,
                 wiiu_blue, pixels);
        putpixel(x + w - 1 + border_thickness - i, y + corner_length + j,
                 wiiu_blue, pixels);
      }

      // Bottom-left diagonal connector
      for (int j = 0; j < border_thickness; j++) {
        putpixel(x + corner_length + j, y + h - 1 + border_thickness - i,
                 wiiu_blue, pixels);
        putpixel(x - border_thickness + i, y + h - 1 - corner_length - j,
                 wiiu_blue, pixels);
      }

      // Bottom-right diagonal connector
      for (int j = 0; j < border_thickness; j++) {
        putpixel(x + w - 1 - corner_length - j,
                 y + h - 1 + border_thickness - i, wiiu_blue, pixels);
        putpixel(x + w - 1 + border_thickness - i,
                 y + h - 1 - corner_length - j, wiiu_blue, pixels);
      }
    }
  }
}

void draw_header(u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int h = get_screen().height / 12;

  // Header with subtle gradient
  for (int y = 0; y < h; y++) {
    u32 header_color = lerp_color(t->header, t->header_shadow, y, h);
    u32 *row = &pixels[y * get_screen().pitch];
    for (int x = 0; x < get_screen().width; x++) {
      row[x] = header_color;
    }
  }

  // Time display (simulated)
  draw_text(50, 20, "12:00", 0x333333, 0x888888, pixels);

  // Center "OS LAUNCHER" properly
  int text_width = 11 * 9; // "OS LAUNCHER" has 11 characters
  draw_text(get_screen().width / 2 - text_width / 2, 20, "4DS LAUNCHER",
            0x333333, 0x888888, pixels); // User icon
  int user_x = get_screen().width - 100;
  draw_circle(user_x, h / 2, 20, 0x0077BB, pixels);

  // Bottom shadow line
  fill_rect(0, h, get_screen().width, 2, t->shadow, pixels);
}

void draw_bottom_bar(u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int bar_height = get_screen().height / 14;
  int y = get_screen().height - bar_height;

  // Bar with gradient
  for (int yy = 0; yy < bar_height; yy++) {
    u32 bar_color = lerp_color(0x00F0F0F0, 0x00E0E0E0, yy, bar_height);
    u32 *row = &pixels[(y + yy) * get_screen().pitch];
    for (int x = 0; x < get_screen().width; x++) {
      row[x] = bar_color;
    }
  }

  // Top shadow line
  fill_rect(0, y, get_screen().width, 2, t->shadow, pixels);

  // Quick launch icons
  const char *quick_icons[] = {"HOME", "APPS", "SETTINGS", "HELP"};
  for (int i = 0; i < 4; i++) {
    int icon_width = strlen(quick_icons[i]) * 9;
    int icon_x = 100 + i * 200 - icon_width / 2;
    draw_text(icon_x, y + 15, quick_icons[i], 0x666666, 0xBBBBBB, pixels);
  }
}

void draw_background(u32 *pixels) {
  theme_t *t = &sys.home.theme;

  // Gradient background
  for (int y = 0; y < get_screen().height; y++) {
    u32 c1 = lerp_color(t->bg_top, t->bg_bottom, y, get_screen().height);
    u32 c2 = lerp_color(0x00E8F2FF, 0x00D0E0F0, y, get_screen().height);
    u32 c = lerp_color(c1, c2, y, get_screen().height);

    u32 *row = &pixels[y * get_screen().pitch];
    for (int x = 0; x < get_screen().width; x++) {
      row[x] = c;
    }
  }

  // Subtle grid pattern
  for (int y = 0; y < get_screen().height; y += 4) {
    for (int x = 0; x < get_screen().width; x += 4) {
      u32 *pixel = &pixels[y * get_screen().pitch + x];
      *pixel = t->icon_bg;
    }
  }
}

void draw_home_screen(u32 *pixels) {
  draw_background(pixels);
  draw_header(pixels);
  draw_bottom_bar(pixels);

  int cols = sys.home.cols;
  int rows = sys.home.rows;

  int gap = 40;
  int top_offset = get_screen().height / 7;

  int tile_w = 180;
  int tile_h = 140;

  int grid_w = cols * tile_w + (cols - 1) * gap;
  int start_x = (get_screen().width - grid_w) / 2;
  int start_y = top_offset;

  int index = 0;

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (index >= sys.home.app_count)
        return;

      int x = start_x + c * (tile_w + gap);
      int y = start_y + r * (tile_h + gap);

      draw_tile(x, y, tile_w, tile_h, (index == sys.home.selected_tile),
                &sys.home.apps[index], pixels);

      index++;
    }
  }
}

void init_apps() {
  // Initialize all 8 apps
  strcpy(sys.home.apps[0].name, "Terminal");
  sys.home.apps[0].color = 0x0099CCFF;
  sys.home.apps[0].icon_type = 0;
  sys.home.apps[0].has_notification = true;

  strcpy(sys.home.apps[1].name, "Settings");
  sys.home.apps[1].color = 0x00FF9966;
  sys.home.apps[1].icon_type = 1;
  sys.home.apps[1].has_notification = false;

  strcpy(sys.home.apps[2].name, "Browser");
  sys.home.apps[2].color = 0x0066CC99;
  sys.home.apps[2].icon_type = 2;
  sys.home.apps[2].has_notification = false;

  strcpy(sys.home.apps[3].name, "Editor");
  sys.home.apps[3].color = 0x00CC99FF;
  sys.home.apps[3].icon_type = 3;
  sys.home.apps[3].has_notification = true;

  strcpy(sys.home.apps[4].name, "Files");
  sys.home.apps[4].color = 0x00FFCC66;
  sys.home.apps[4].icon_type = 4;
  sys.home.apps[4].has_notification = false;

  strcpy(sys.home.apps[5].name, "Music");
  sys.home.apps[5].color = 0x0066FFCC;
  sys.home.apps[5].icon_type = 5;
  sys.home.apps[5].has_notification = true;

  strcpy(sys.home.apps[6].name, "Camera");
  sys.home.apps[6].color = 0x00FF6666;
  sys.home.apps[6].icon_type = 6;
  sys.home.apps[6].has_notification = false;

  strcpy(sys.home.apps[7].name, "Calculator");
  sys.home.apps[7].color = 0x009966FF;
  sys.home.apps[7].icon_type = 0;
  sys.home.apps[7].has_notification = false;
}
