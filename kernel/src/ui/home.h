// ===== ./kernel/src/ui/home.h (rename to ui.h) =====
#ifndef _UI_H
#define _UI_H

#include "../gfx.h"
#include "../stdlib.h"
#include "../string.h"

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
  int icon_type;
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

extern system_t sys;

// ------------------------------------------------------------
// UI elements
// ------------------------------------------------------------
void draw_icon(int x, int y, int size, int icon_type, u32 *pixels);
void draw_notification_badge(int x, int y, u32 *pixels);
void draw_tile(int x, int y, int w, int h, bool selected, app_t *app,
               u32 *pixels);

void draw_header(u32 *pixels);
void draw_bottom_bar(u32 *pixels);

void draw_background(u32 *pixels);
void draw_home_screen(u32 *pixels);

void init_apps();

#endif // _UI_H
