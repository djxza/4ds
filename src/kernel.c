#include <stddef.h>
#include <stdint.h>

#define MULTIBOOT2_BOOTLOADER_MAGIC 0x36d76289
#define MULTIBOOT_TAG_TYPE_END 0
#define MULTIBOOT_TAG_TYPE_FRAMEBUFFER 8

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

void kmain(uint32_t magic, uint32_t addr) {
  if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
    for (;;)
      ;

  struct multiboot_tag *tag = (struct multiboot_tag *)(addr + 8);

  struct multiboot_tag_framebuffer *fb = 0;

  while (tag->type != MULTIBOOT_TAG_TYPE_END) {
    if (tag->type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
      fb = (struct multiboot_tag_framebuffer *)tag;
      break;
    }
    tag = (struct multiboot_tag *)((uint8_t *)tag + ((tag->size + 7) & ~7));
  }

  if (!fb)
    for (;;)
      ;

  uint32_t *buffer = (uint32_t *)(uint32_t)fb->framebuffer_addr;

  uint32_t pitch = fb->framebuffer_pitch / 4;
  uint32_t width = fb->framebuffer_width;
  uint32_t height = fb->framebuffer_height;

  for (uint32_t y = 0; y < height; y++) {
    for (uint32_t x = 0; x < width; x++) {
      buffer[y * pitch + x] = 0x00FF00; // GREEN
    }
  }

  for (;;)
    __asm__ volatile("hlt");
}
