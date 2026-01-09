#include "string.h"

void *memcpy(void *dest, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  // Simple byte-by-byte copy
  for (size_t i = 0; i < n; i++) {
    d[i] = s[i];
  }

  return dest;
}

void *memset(void *s, int c, size_t n) {
  uint8_t *p = (uint8_t *)s;

  for (size_t i = 0; i < n; i++) {
    p[i] = (uint8_t)c;
  }

  return s;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const uint8_t *p1 = (const uint8_t *)s1;
  const uint8_t *p2 = (const uint8_t *)s2;

  for (size_t i = 0; i < n; i++) {
    if (p1[i] != p2[i]) {
      return p1[i] - p2[i];
    }
  }

  return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dest;
  const uint8_t *s = (const uint8_t *)src;

  if (d == s)
    return dest;

  // If buffers don't overlap, use memcpy
  if (d + n <= s || s + n <= d) {
    return memcpy(dest, src, n);
  }

  // Buffers overlap - copy carefully
  if (d < s) {
    // Copy forward
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else {
    // Copy backward
    for (size_t i = n; i > 0; i--) {
      d[i - 1] = s[i - 1];
    }
  }

  return dest;
}

// ------------------------------------------------------------
// String functions
// ------------------------------------------------------------

size_t strlen(const char *s) {
  size_t len = 0;
  while (s[len])
    len++;
  return len;
}

char *strcpy(char *dest, const char *src) {
  char *d = dest;
  while ((*d++ = *src++))
    ;
  return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
  size_t i;

  for (i = 0; i < n && src[i] != '\0'; i++) {
    dest[i] = src[i];
  }

  for (; i < n; i++) {
    dest[i] = '\0';
  }

  return dest;
}

char *strcat(char *dest, const char *src) {
  char *d = dest;

  // Find end of dest
  while (*d)
    d++;

  // Copy src
  while ((*d++ = *src++))
    ;

  return dest;
}

char *strncat(char *dest, const char *src, size_t n) {
  char *d = dest;

  // Find end of dest
  while (*d)
    d++;

  // Copy at most n chars
  size_t i;
  for (i = 0; i < n && src[i] != '\0'; i++) {
    d[i] = src[i];
  }

  // Null terminate
  d[i] = '\0';

  return dest;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  if (n == 0)
    return 0;

  while (--n && *s1 && (*s1 == *s2)) {
    s1++;
    s2++;
  }

  return *(const unsigned char *)s1 - *(const unsigned char *)s2;
}

char *strchr(const char *s, int c) {
  while (*s != (char)c) {
    if (*s == '\0')
      return NULL;
    s++;
  }

  return (char *)s;
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;

  do {
    if (*s == (char)c) {
      last = s;
    }
  } while (*s++);

  return (char *)last;
}

size_t strspn(const char *s, const char *accept) {
  size_t count = 0;

  while (*s && strchr(accept, *s)) {
    count++;
    s++;
  }

  return count;
}

size_t strcspn(const char *s, const char *reject) {
  size_t count = 0;

  while (*s && !strchr(reject, *s)) {
    count++;
    s++;
  }

  return count;
}

char *strpbrk(const char *s, const char *accept) {
  while (*s) {
    if (strchr(accept, *s)) {
      return (char *)s;
    }
    s++;
  }

  return NULL;
}

// ------------------------------------------------------------
// Additional useful functions
// ------------------------------------------------------------

// Convert integer to string
char *itoa(int value, char *str, int base) {
  static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  char *ptr = str;
  char *ptr1 = str;
  char tmp;
  unsigned int v;

  // Handle negative numbers for base 10
  if (base == 10 && value < 0) {
    *ptr++ = '-';
    ptr1 = ptr;
    v = (unsigned int)(-value);
  } else {
    v = (unsigned int)value;
  }

  // Convert number
  do {
    *ptr++ = digits[v % base];
    v /= base;
  } while (v);

  *ptr-- = '\0';

  // Reverse string
  while (ptr1 < ptr) {
    tmp = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp;
  }

  return str;
}

// Convert unsigned integer to string
char *utoa(unsigned int value, char *str, int base) {
  static char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
  char *ptr = str;
  char *ptr1 = str;
  char tmp;

  // Convert number
  do {
    *ptr++ = digits[value % base];
    value /= base;
  } while (value);

  *ptr-- = '\0';

  // Reverse string
  while (ptr1 < ptr) {
    tmp = *ptr;
    *ptr-- = *ptr1;
    *ptr1++ = tmp;
  }

  return str;
}
