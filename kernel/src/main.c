#include "../limine-protocol/include/limine.h"
#include <stdbool.h>
#include <stdint.h>

#include "string.h"

#include "math.h"

#define COM1 0x3F8

typedef uint32_t u32;
typedef uint64_t u64;
typedef uint8_t u8;

// ------------------------------------------------------------
// Limine framebuffer request
// ------------------------------------------------------------

__attribute__((
    used)) static volatile struct limine_framebuffer_request fb_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST_ID, .revision = 0};

// ------------------------------------------------------------
// Types
// ------------------------------------------------------------

typedef struct {
  int width;
  int height;
  u64 pitch; // in pixels
  bool init;
} screen_t;

typedef struct {
  u32 bg_top;
  u32 bg_bottom;
  u32 tile;
  u32 tile_light;
  u32 tile_dark;
  u32 shadow;
  u32 header;
  u32 header_shadow;
  u32 icon_bg;
  u32 text_color;
  u32 text_shadow;
  u32 highlight;
  u32 notification;
} theme_t;

typedef struct {
  char name[32];
  u32 color;
  int icon_type; // 0: Game, 1: Settings, 2: Browser, 3: Shop, 4: Media, 5:
                 // Friends, 6: System
  bool has_notification;
} app_t;

typedef struct {
  int cols;
  int rows;
  int app_count;
  int selected_tile;
  theme_t theme;
  app_t apps[16];
} home_t;

typedef struct {
  home_t home;
} system_t;

static screen_t screen;
static system_t sys;

// ------------------------------------------------------------
// Drawing primitives
// ------------------------------------------------------------

static inline void putpixel(int x, int y, u32 col, u32 *pixels) {
  if ((unsigned)x >= (unsigned)screen.width ||
      (unsigned)y >= (unsigned)screen.height)
    return;

  pixels[y * screen.pitch + x] = col;
}

static inline u32 lerp_color(u32 a, u32 b, int t, int max) {
  int ar = (a >> 16) & 255;
  int ag = (a >> 8) & 255;
  int ab = (a) & 255;

  int br = (b >> 16) & 255;
  int bg = (b >> 8) & 255;
  int bb = (b) & 255;

  int r = ar + (br - ar) * t / max;
  int g = ag + (bg - ag) * t / max;
  int b2 = ab + (bb - ab) * t / max;

  return (r << 16) | (g << 8) | b2;
}

static void fill_rect(int x, int y, int w, int h, u32 col, u32 *pixels) {
  if (w <= 0 || h <= 0)
    return;

  for (int yy = 0; yy < h; yy++) {
    u32 *row = &pixels[(y + yy) * screen.pitch + x];
    for (int xx = 0; xx < w; xx++)
      row[xx] = col;
  }
}

// Draw anti-aliased circle for better rounded corners
static void draw_circle(int cx, int cy, int radius, u32 color, u32 *pixels) {
  int r2 = radius * radius;
  for (int y = -radius; y <= radius; y++) {
    for (int x = -radius; x <= radius; x++) {
      if (x * x + y * y <= r2) {
        putpixel(cx + x, cy + y, color, pixels);
      }
    }
  }
}

// Rounded rectangle with proper anti-aliasing
static void fill_rounded_rect(int x, int y, int w, int h, int radius, u32 col,
                              u32 *pixels) {
  // Fill center
  fill_rect(x + radius, y, w - 2 * radius, h, col, pixels);
  fill_rect(x, y + radius, w, h - 2 * radius, col, pixels);

  // Draw corners
  for (int i = 0; i <= radius; i++) {
    for (int j = 0; j <= radius; j++) {
      if (i * i + j * j <= radius * radius) {
        putpixel(x + i, y + j, col, pixels);
        putpixel(x + w - 1 - i, y + j, col, pixels);
        putpixel(x + i, y + h - 1 - j, col, pixels);
        putpixel(x + w - 1 - i, y + h - 1 - j, col, pixels);
      }
    }
  }
}

// Draw text (simple bitmap font for demo)
static void draw_text(int x, int y, const char *text, u32 color,
                      u32 shadow_color, u32 *pixels) {
  // Simple 8x8 font simulation
  while (*text) {
    for (int yy = 0; yy < 8; yy++) {
      for (int xx = 0; xx < 8; xx++) {
        // Draw shadow offset
        putpixel(x + xx * 8 + 1, y + yy * 8 + 1, shadow_color, pixels);
        // Draw character (simplified - always draw for demo)
        putpixel(x + xx * 8, y + yy * 8, color, pixels);
      }
    }
    x += 48;
    text++;
  }
}

// ------------------------------------------------------------
// Wii U specific elements
// ------------------------------------------------------------

static void draw_wiiu_icon(int x, int y, int size, int icon_type, u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int center_x = x + size / 2;
  int center_y = y + size / 2;

  // Icon background circle (simplified - just draw a filled circle)
  for (int i = -size / 3; i <= size / 3; i++) {
    for (int j = -size / 3; j <= size / 3; j++) {
      if (i * i + j * j <= (size / 3) * (size / 3)) {
        putpixel(center_x + i, center_y + j, t->icon_bg, pixels);
      }
    }
  }

  // Draw icon based on type (using simpler integer math)
  switch (icon_type) {
  case 0: // Game - controller icon
    // Square body
    fill_rect(center_x - 12, center_y - 8, 24, 16, 0xFFFFFF, pixels);
    // Buttons
    putpixel(center_x - 6, center_y - 4, 0x000000, pixels);
    putpixel(center_x + 6, center_y - 4, 0x000000, pixels);
    putpixel(center_x, center_y + 4, 0x000000, pixels);
    break;
  case 1: // Settings - simplified gear
    // Draw a simple cross instead of gear
    fill_rect(center_x - 2, center_y - 10, 4, 20, 0xFFFFFF, pixels);
    fill_rect(center_x - 10, center_y - 2, 20, 4, 0xFFFFFF, pixels);
    // Add small circles at ends
    for (int i = -2; i <= 2; i++) {
      for (int j = -2; j <= 2; j++) {
        if (i * i + j * j <= 4) {
          putpixel(center_x - 10 + i, center_y - 2 + j, 0xFFFFFF, pixels);
          putpixel(center_x + 10 + i, center_y - 2 + j, 0xFFFFFF, pixels);
          putpixel(center_x + i, center_y - 10 + j, 0xFFFFFF, pixels);
          putpixel(center_x + i, center_y + 10 + j, 0xFFFFFF, pixels);
        }
      }
    }
    break;
  case 2: // Browser - simplified globe
    for (int i = -size / 4; i <= size / 4; i++) {
      for (int j = -size / 4; j <= size / 4; j++) {
        if (i * i + j * j <= (size / 4) * (size / 4)) {
          putpixel(center_x + i, center_y + j, 0xFFFFFF, pixels);
        }
      }
    }
    // Simple cross lines
    fill_rect(center_x - size / 4, center_y, size / 2, 1, 0x000000, pixels);
    fill_rect(center_x, center_y - size / 4, 1, size / 2, 0x000000, pixels);
    break;
  case 3: // Shop - shopping bag
    fill_rect(center_x - 10, center_y - 6, 20, 12, 0xFFFFFF, pixels);
    fill_rect(center_x - 6, center_y - 10, 12, 4, 0xFFFFFF, pixels);
    // Handle
    for (int i = -4; i <= 4; i++) {
      putpixel(center_x + i, center_y - 12, 0xFFFFFF, pixels);
    }
    break;
  case 4: // Media - play triangle
    for (int i = 0; i < 12; i++) {
      int start = -i;
      int end = i;
      for (int j = start; j <= end; j++) {
        putpixel(center_x + j, center_y - 6 + i, 0xFFFFFF, pixels);
      }
    }
    break;
  case 5: // Friends - two people
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

static void draw_notification_badge(int x, int y, u32 *pixels) {
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

static void draw_wiiu_tile(int x, int y, int w, int h, bool selected,
                           app_t *app, u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int radius = 12;

  // Shadow with offset (more prominent)
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

  // Top glossy highlight (Wii U style)
  for (int yy = 0; yy < h / 6; yy++) {
    u32 highlight_color = lerp_color(t->tile_light, 0x00FFFFFF, yy, h / 6);
    for (int xx = radius; xx < w - radius; xx++) {
      putpixel(x + xx, y + yy + 2, highlight_color, pixels);
    }
  }

  // Icon
  draw_wiiu_icon(x + w / 2, y + h / 3, 48, app->icon_type, pixels);

  // App name
  draw_text(x + w / 2 - (strlen(app->name) * 12), y + h - 40, app->name,
            t->text_color, t->text_shadow, pixels);

  // Notification badge
  if (app->has_notification) {
    draw_notification_badge(x + w - 20, y + 20, pixels);
  }

  // Selection highlight
  if (selected) {
    // Blue glow around tile
    for (int i = -2; i <= w + 1; i++) {
      putpixel(x + i, y - 2, 0x0066CCFF, pixels);
      putpixel(x + i, y + h + 1, 0x0066CCFF, pixels);
    }
    for (int i = -2; i <= h + 1; i++) {
      putpixel(x - 2, y + i, 0x0066CCFF, pixels);
      putpixel(x + w + 1, y + i, 0x0066CCFF, pixels);
    }
  }
}

static void draw_wiiu_header(u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int h = screen.height / 12;

  // Header with subtle gradient
  for (int y = 0; y < h; y++) {
    u32 header_color = lerp_color(t->header, t->header_shadow, y, h);
    u32 *row = &pixels[y * screen.pitch];
    for (int x = 0; x < screen.width; x++) {
      row[x] = header_color;
    }
  }

  // Time display (simulated)
  draw_text(50, 20, "2:45 PM", 0x333333, 0x888888, pixels);

  // Console name
  draw_text(screen.width / 2 - 100, 20, "NOVA CONSOLE", 0x333333, 0x888888,
            pixels);

  // User icon
  int user_x = screen.width - 100;
  draw_circle(user_x, h / 2, 20, 0x0077BB, pixels);

  // Bottom shadow line
  fill_rect(0, h, screen.width, 2, t->shadow, pixels);
}

static void draw_wiiu_bottom_bar(u32 *pixels) {
  theme_t *t = &sys.home.theme;
  int bar_height = screen.height / 14;
  int y = screen.height - bar_height;

  // Bar with gradient
  for (int yy = 0; yy < bar_height; yy++) {
    u32 bar_color = lerp_color(0x00F0F0F0, 0x00E0E0E0, yy, bar_height);
    u32 *row = &pixels[(y + yy) * screen.pitch];
    for (int x = 0; x < screen.width; x++) {
      row[x] = bar_color;
    }
  }

  // Top shadow line
  fill_rect(0, y, screen.width, 2, t->shadow, pixels);

  // Quick launch icons
  const char *quick_icons[] = {"HOME", "WEB", "FRIENDS", "SHOP"};
  for (int i = 0; i < 4; i++) {
    int icon_x = 100 + i * 200;
    draw_text(icon_x, y + 15, quick_icons[i], 0x666666, 0xBBBBBB, pixels);
  }
}

static void draw_wiiu_background(u32 *pixels) {
  theme_t *t = &sys.home.theme;

  // Wii U style gradient background
  for (int y = 0; y < screen.height; y++) {
    // Double gradient for more depth
    u32 c1 = lerp_color(t->bg_top, t->bg_bottom, y, screen.height);
    u32 c2 = lerp_color(0x00E8F2FF, 0x00D0E0F0, y, screen.height);
    u32 c = lerp_color(c1, c2, y, screen.height);

    u32 *row = &pixels[y * screen.pitch];
    for (int x = 0; x < screen.width; x++) {
      row[x] = c;
    }
  }

  // Subtle grid pattern
  for (int y = 0; y < screen.height; y += 4) {
    for (int x = 0; x < screen.width; x += 4) {
      u32 *pixel = &pixels[y * screen.pitch + x];
      *pixel = sys.home.theme.icon_bg;
    }
  }
}

// ------------------------------------------------------------
// Home screen layout
// ------------------------------------------------------------

static void draw_wiiu_home_screen(u32 *pixels) {
  draw_wiiu_background(pixels);
  draw_wiiu_header(pixels);
  draw_wiiu_bottom_bar(pixels);

  int cols = sys.home.cols;
  int rows = sys.home.rows;

  int gap = 40;
  int top_offset = screen.height / 7;

  int tile_w = 180;
  int tile_h = 140;

  int grid_w = cols * tile_w + (cols - 1) * gap;
  int start_x = (screen.width - grid_w) / 2;
  int start_y = top_offset;

  int index = 0;

  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (index >= sys.home.app_count)
        return;

      int x = start_x + c * (tile_w + gap);
      int y = start_y + r * (tile_h + gap);

      draw_wiiu_tile(x, y, tile_w, tile_h, (index == sys.home.selected_tile),
                     &sys.home.apps[index], pixels);
      index++;
    }
  }
}

// Initialize Wii U style apps
static void init_wiiu_apps() {
  // Game apps with Wii U style colors
  strcpy(sys.home.apps[0].name, "Super Nova");
  sys.home.apps[0].color = 0x00FF3366;
  sys.home.apps[0].icon_type = 0;
  sys.home.apps[0].has_notification = true;

  strcpy(sys.home.apps[1].name, "Galaxy Run");
  sys.home.apps[1].color = 0x006633FF;
  sys.home.apps[1].icon_type = 0;
  sys.home.apps[1].has_notification = false;

  strcpy(sys.home.apps[2].name, "Settings");
  sys.home.apps[2].color = 0x00888888;
  sys.home.apps[2].icon_type = 1;
  sys.home.apps[2].has_notification = false;

  strcpy(sys.home.apps[3].name, "Browser");
  sys.home.apps[3].color = 0x0033AA33;
  sys.home.apps[3].icon_type = 2;
  sys.home.apps[3].has_notification = true;

  strcpy(sys.home.apps[4].name, "eShop");
  sys.home.apps[4].color = 0x00FFAA00;
  sys.home.apps[4].icon_type = 3;
  sys.home.apps[4].has_notification = false;

  strcpy(sys.home.apps[5].name, "TV & Video");
  sys.home.apps[5].color = 0x00CC33CC;
  sys.home.apps[5].icon_type = 4;
  sys.home.apps[5].has_notification = false;

  strcpy(sys.home.apps[6].name, "Friends");
  sys.home.apps[6].color = 0x0033CCCC;
  sys.home.apps[6].icon_type = 5;
  sys.home.apps[6].has_notification = false;

  strcpy(sys.home.apps[7].name, "System");
  sys.home.apps[7].color = 0x00AA6666;
  sys.home.apps[7].icon_type = 6;
  sys.home.apps[7].has_notification = false;
}

// ------------------------------------------------------------
// Kernel entry
// ------------------------------------------------------------

void _start(void) {
  if (!fb_request.response || fb_request.response->framebuffer_count < 1) {
    for (;;)
      __asm__ volatile("hlt");
  }

  struct limine_framebuffer *fb = fb_request.response->framebuffers[0];

  u32 *pixels = (u32 *)fb->address;

  screen.width = fb->width;
  screen.height = fb->height;
  screen.pitch = fb->pitch / 4;
  screen.init = true;

  // ---------------- Wii U Theme ----------------
  sys.home.cols = 4;
  sys.home.rows = 2;
  sys.home.app_count = 8;
  sys.home.selected_tile = 0; // First tile selected

  // Authentic Wii U color palette
  sys.home.theme.bg_top = 0x00F8FCFF;        // Bright sky white
  sys.home.theme.bg_bottom = 0x00E0E8F0;     // Soft blue-gray
  sys.home.theme.tile = 0x0099CCFF;          // Wii U blue base
  sys.home.theme.tile_light = 0x00CCEEFF;    // Gloss highlight
  sys.home.theme.tile_dark = 0x006699CC;     // Depth shadow
  sys.home.theme.shadow = 0x00404040;        // Subtle shadow
  sys.home.theme.header = 0x00FFFFFF;        // Clean white header
  sys.home.theme.header_shadow = 0x00E0E0E0; // Header shadow
  sys.home.theme.icon_bg = 0x00FFFFFF;       // Icon background
  sys.home.theme.text_color = 0x00333333;    // Dark gray text
  sys.home.theme.text_shadow = 0x00AAAAAA;   // Text shadow
  sys.home.theme.highlight = 0x0066CCFF;     // Selection highlight
  sys.home.theme.notification = 0x00FF3333;  // Red notification

  init_wiiu_apps();
  draw_wiiu_home_screen(pixels);

  for (;;)
    __asm__ volatile("hlt");
}
