#include <stdint.h>

#define MULTIBOOT2_TAG_TYPE_FRAMEBUFFER 8

struct multiboot_tag {
  uint32_t type;
  uint32_t size;
};

struct multiboot_tag_framebuffer {
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

void kmain(void *mbi) {
  uint8_t *ptr = (uint8_t *)mbi + 8;

  while (1) {
    struct multiboot_tag *tag = (struct multiboot_tag *)ptr;
    if (tag->type == 0)
      break;

    if (tag->type == MULTIBOOT2_TAG_TYPE_FRAMEBUFFER) {
      struct multiboot_tag_framebuffer *fb =
          (struct multiboot_tag_framebuffer *)tag;

      uint32_t *pixels = (uint32_t *)(uint64_t)fb->framebuffer_addr;
      uint32_t pitch = fb->framebuffer_pitch / 4;

      for (uint32_t y = 0; y < fb->framebuffer_height; y++) {
        for (uint32_t x = 0; x < fb->framebuffer_width; x++) {
          pixels[y * pitch + x] = 0xFA3030FF; // visible blue
        }
      }
      break;
    }

    ptr += (tag->size + 7) & ~7;
  }

  for (;;)
    __asm__("hlt");
}
