#include <stdint.h>

#define MULTIBOOT2_TAG_TYPE_END 0
#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER 8

typedef struct {
  uint32_t type;
  uint32_t size;
} mb2_tag_t;

typedef struct {
  uint32_t type;
  uint32_t size;
  uint64_t framebuffer_addr;
  uint32_t framebuffer_pitch;
  uint32_t framebuffer_width;
  uint32_t framebuffer_height;
  uint8_t framebuffer_bpp;
  uint8_t framebuffer_type;
  uint16_t reserved;
} mb2_framebuffer_tag_t;

static inline uint64_t align8(uint64_t x) { return (x + 7) & ~7; }

void kernel_main64(void *multiboot_info) {
  uint8_t *ptr = (uint8_t *)multiboot_info + 8;
  mb2_framebuffer_tag_t *fb = 0;

  while (1) {
    mb2_tag_t *tag = (mb2_tag_t *)ptr;

    if (tag->type == MULTIBOOT2_TAG_TYPE_END)
      break;

    if (tag->type == MULTIBOOT2_TAG_TYPE_FRAMEBUFFER) {
      fb = (mb2_framebuffer_tag_t *)tag;
      break;
    }

    ptr += align8(tag->size);
  }

  if (!fb)
    for (;;)
      __asm__ volatile("hlt");

  volatile uint32_t *pixels = (uint32_t *)(uint64_t)fb->framebuffer_addr;
  uint32_t pitch_pixels = fb->framebuffer_pitch / 4;

  // Paint the screen bright red ❤️
  for (uint32_t y = 0; y < fb->framebuffer_height; y++) {
    for (uint32_t x = 0; x < fb->framebuffer_width; x++) {
      pixels[y * pitch_pixels + x] = 0x00FF0000;
    }
  }

  for (;;)
    __asm__ volatile("hlt");
}
