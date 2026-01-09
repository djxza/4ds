#ifndef _GFX_H
#define _GFX_H

#include "../limine-protocol/include/limine.h"

#include "io.h"
#include "stdlib.h"

typedef struct {
  int width;
  int height;
  u32 *pixels;
  u64 pitch; // in pixels
} screen_t;

void initscr();
screen_t get_screen();

// ------------------------------------------------------------
// Drawing primitives
// ------------------------------------------------------------
void putpixel(int x, int y, u32 col, u32 *pixels);

u32 lerp_color(u32 a, u32 b, int t, int max);

void fill_rect(int x, int y, int w, int h, u32 col, u32 *pixels);

// Draw anti-aliased circle for better rounded corners
void draw_circle(int cx, int cy, int radius, u32 color, u32 *pixels);

// Rounded rectangle with proper anti-aliasing
void fill_rounded_rect(int x, int y, int w, int h, int radius, u32 col,
                       u32 *pixels);
// Font rendering
void draw_char(int x, int y, char c, u32 color, u32 *pixels);
void draw_text(int x, int y, const char *text, u32 color, u32 shadow_color,
               u32 *pixels);

#endif // _GFX_H
