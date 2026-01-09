#ifndef _STDLIB_H
#define _STDLIB_H

#define NULL ((void *)0)
#define ASSERT(_e) (_assert(_e, #_e))

#include <stdbool.h>
#include <stdint.h>

typedef unsigned long size_t;
typedef signed long ssize_t;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef size_t usize;
typedef ssize_t isize;

void hang();
void _assert(bool expr, const char *msg);

#endif // _STDLIB_H
