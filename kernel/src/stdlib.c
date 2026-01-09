#include "stdlib.h"
#include "io.h"

void hang() {
  for (;;)
    __asm__ volatile("hlt");
}

void _assert(bool expr, const char *msg) {
  if (!expr) {
    printb_str(msg);
    outb(COM1, '\n'); // newline
    hang();
  }
}
