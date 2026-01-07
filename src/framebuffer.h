#pragma once

#include <stdint.h>

int framebuffer_init(void *multiboot_info);
void framebuffer_put_pixel(uint32_t x, uint32_t y, uint32_t color);
void framebuffer_clear(uint32_t color);

uint32_t get_fb_width();
uint32_t get_fb_height();
