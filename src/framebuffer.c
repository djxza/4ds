#include "framebuffer.h"
#include <stddef.h>
#include <stdint.h>

#define MB2_TAG_FRAMEBUFFER 8
#define MB2_TAG_END 0

// ----------------------------
// Multiboot2 tag structures
// ----------------------------
struct mb2_tag {
  uint32_t type;
  uint32_t size;
};

struct mb2_tag_framebuffer {
  uint32_t type;
  uint32_t size;
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type;
  uint16_t reserved;
};

// ----------------------------
// Framebuffer state
// ----------------------------
static volatile uint8_t *fb_base = 0;
static uint32_t fb_width = 0;
static uint32_t fb_height = 0;
static uint32_t fb_pitch = 0;
static uint32_t fb_bpp = 0;

// ----------------------------
// Public API
// ----------------------------
int framebuffer_init(void *multiboot_info) {
  uint8_t *ptr = (uint8_t *)multiboot_info;

  // Skip total_size + reserved (8 bytes)
  ptr += 8;

  while (1) {
    struct mb2_tag *tag = (struct mb2_tag *)ptr;

    if (tag->type == MB2_TAG_END)
      break;

    if (tag->type == MB2_TAG_FRAMEBUFFER) {
      struct mb2_tag_framebuffer *fb = (struct mb2_tag_framebuffer *)tag;

      fb_base = (volatile uint8_t *)(uintptr_t)fb->framebuffer_addr;
      fb_width = fb->framebuffer_width;
      fb_height = fb->framebuffer_height;
      fb_pitch = fb->framebuffer_pitch;
      fb_bpp = fb->framebuffer_bpp;

      return 1; // success
    }

    // Advance to next tag (8-byte aligned)
    ptr += (tag->size + 7) & ~7;
  }

  return 0; // framebuffer not found
}

void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color) {
  if (!fb_base)
    return;
  if (x >= fb_width || y >= fb_height)
    return;

  uint32_t bytes_per_pixel = fb_bpp / 8;
  volatile uint8_t *pixel = fb_base + y * fb_pitch + x * bytes_per_pixel;

  // Assume 32bpp XRGB (most GRUB setups)
  pixel[0] = (color >> 0) & 0xFF;  // B
  pixel[1] = (color >> 8) & 0xFF;  // G
  pixel[2] = (color >> 16) & 0xFF; // R
  pixel[3] = 0x00;
}

void framebuffer_clear(uint32_t color) {
  for (uint32_t y = 0; y < fb_height; y++) {
    for (uint32_t x = 0; x < fb_width; x++) {
      framebuffer_put_pixel(x, y, color);
    }
  }
}

uint32_t get_fb_width() { return fb_width; }
uint32_t get_fb_height() { return fb_height; }
