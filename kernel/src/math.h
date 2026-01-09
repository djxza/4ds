// ============================================================================
// math.h implementations
// ============================================================================

#ifndef _MATH_H
#define _MATH_H

#include <stdbool.h>
#include <stdint.h>

// ------------------------------------------------------------
// Constants
// ------------------------------------------------------------

#define M_E 2.71828182845904523536        // e
#define M_LOG2E 1.44269504088896340736    // log2(e)
#define M_LOG10E 0.43429448190325182765   // log10(e)
#define M_LN2 0.69314718055994530942      // ln(2)
#define M_LN10 2.30258509299404568402     // ln(10)
#define M_PI 3.14159265358979323846       // π
#define M_PI_2 1.57079632679489661923     // π/2
#define M_PI_4 0.78539816339744830962     // π/4
#define M_1_PI 0.31830988618379067154     // 1/π
#define M_2_PI 0.63661977236758134308     // 2/π
#define M_2_SQRTPI 1.12837916709551257390 // 2/√π
#define M_SQRT2 1.41421356237309504880    // √2
#define M_SQRT1_2 0.70710678118654752440  // 1/√2
#define M_TAU 6.28318530717958647692      // τ (2π)

// Floating point types
typedef float f32;
typedef double f64;

// ------------------------------------------------------------
// Function prototypes
// ------------------------------------------------------------

// Basic functions
static inline f64 fabs(f64 x);
static inline f32 fabsf(f32 x);
static f64 fmod(f64 x, f64 y);
static inline f64 fmax(f64 a, f64 b);
static inline f64 fmin(f64 a, f64 b);

// Power and exponential
static f64 sqrt(f64 x);
static f32 sqrtf(f32 x);
static f64 powi(f64 base, int exp);
static f64 pow(f64 x, f64 y);
static f64 exp(f64 x);
static f64 log(f64 x);
static f64 log10(f64 x);
static f64 log2(f64 x);

// Trigonometric
static f64 reduce_angle(f64 x);
static f64 sin(f64 x);
static f64 cos(f64 x);
static f64 tan(f64 x);
static f64 asin(f64 x);
static f64 acos(f64 x);
static f64 atan(f64 x);
static f64 atan2(f64 y, f64 x);

// Hyperbolic
static f64 sinh(f64 x);
static f64 cosh(f64 x);
static f64 tanh(f64 x);

// Rounding
static f64 floor(f64 x);
static f64 ceil(f64 x);
static f64 trunc(f64 x);
static f64 round(f64 x);

// Absolute value and sign
static inline int abs(int x);
static inline long int labs(long int x);
static inline long long int llabs(long long int x);
static inline f64 copysign(f64 x, f64 y);
static inline f64 signbit(f64 x);

// Special functions
static f64 hypot(f64 x, f64 y);
static f64 erf(f64 x);
static f64 erfc(f64 x);
static f64 tgamma(f64 x);
static f64 lgamma(f64 x);

// Floating point classification
static inline int fpclassify(f64 x);
static inline int isfinite(f64 x);
static inline int isinf(f64 x);
static inline int isnan(f64 x);
static inline int isnormal(f64 x);

// ------------------------------------------------------------
// Basic functions implementations
// ------------------------------------------------------------

static inline f64 fabs(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  u.i &= 0x7FFFFFFFFFFFFFFFULL;
  return u.f;
}

static inline f32 fabsf(f32 x) {
  union {
    f32 f;
    uint32_t i;
  } u = {x};
  u.i &= 0x7FFFFFFF;
  return u.f;
}

static inline f64 fmax(f64 a, f64 b) {
  if (a != a)
    return b; // a is NaN
  if (b != b)
    return a; // b is NaN
  return (a > b) ? a : b;
}

static inline f64 fmin(f64 a, f64 b) {
  if (a != a)
    return b; // a is NaN
  if (b != b)
    return a; // b is NaN
  return (a < b) ? a : b;
}

// ------------------------------------------------------------
// Power and exponential functions
// ------------------------------------------------------------

// Square root using Newton's method
static f64 sqrt(f64 x) {
  if (x < 0.0)
    return 0.0 / 0.0; // NaN
  if (x == 0.0)
    return 0.0;

  // Initial guess
  f64 y = x;
  f64 z = (y + x / y) / 2.0;

  // Iterate to improve accuracy
  while (fabs(y - z) >= 1e-15) {
    y = z;
    z = (y + x / y) / 2.0;
  }

  return z;
}

static f32 sqrtf(f32 x) { return (f32)sqrt((f64)x); }

// Fast integer power
static f64 powi(f64 base, int exp) {
  if (exp == 0)
    return 1.0;
  if (exp == 1)
    return base;
  if (exp < 0)
    return 1.0 / powi(base, -exp);

  f64 result = 1.0;
  while (exp > 0) {
    if (exp & 1)
      result *= base;
    base *= base;
    exp >>= 1;
  }
  return result;
}

// Exponential function using Taylor series
static f64 exp(f64 x) {
  // Handle negative inputs
  bool neg = x < 0;
  if (neg)
    x = -x;

  // Scale down
  int n = 0;
  while (x > 1.0) {
    x /= M_E;
    n++;
  }

  // Taylor series expansion
  f64 term = 1.0;
  f64 sum = 1.0;

  for (int i = 1; i < 20; i++) {
    term *= x / i;
    sum += term;
    if (term < 1e-15)
      break;
  }

  // Scale back up
  while (n > 0) {
    sum *= M_E;
    n--;
  }

  return neg ? 1.0 / sum : sum;
}

// Natural logarithm using Newton's method
static f64 log(f64 x) {
  if (x <= 0.0)
    return 0.0 / 0.0; // NaN

  // Reduce range
  f64 y = 0.0;
  while (x >= 2.0) {
    x /= M_E;
    y += 1.0;
  }
  while (x < 1.0) {
    x *= M_E;
    y -= 1.0;
  }

  // Newton's method: solve exp(z) = x
  x -= 1.0; // Now x is in [0, 1)
  f64 z = x;
  f64 term = x;

  for (int i = 2; i < 30; i++) {
    term *= -x;
    z += term / i;
    if (fabs(term) < 1e-15)
      break;
  }

  return y + z;
}

// General power function using exp(y * log(x))
static f64 pow(f64 x, f64 y) {
  // Handle special cases
  if (x == 1.0 || y == 0.0)
    return 1.0;
  if (y == 1.0)
    return x;
  if (x == 0.0) {
    if (y > 0.0)
      return 0.0;
    if (y < 0.0)
      return 1.0 / 0.0; // Infinity
    return 1.0;         // 0^0
  }

  // Use identity: x^y = exp(y * log(x))
  return exp(y * log(x));
}

// Base-10 logarithm
static f64 log10(f64 x) { return log(x) * M_LOG10E; }

// Base-2 logarithm
static f64 log2(f64 x) { return log(x) * M_LOG2E; }

// ------------------------------------------------------------
// Rounding functions (need to be defined before fmod)
// ------------------------------------------------------------

static f64 floor(f64 x) {
  if (x >= 0.0)
    return (f64)((int64_t)x);
  f64 i = (f64)((int64_t)x);
  return (i == x) ? i : i - 1.0;
}

static f64 ceil(f64 x) {
  if (x <= 0.0)
    return (f64)((int64_t)x);
  f64 i = (f64)((int64_t)x);
  return (i == x) ? i : i + 1.0;
}

static f64 trunc(f64 x) { return (x >= 0.0) ? floor(x) : ceil(x); }

static f64 round(f64 x) { return (x >= 0.0) ? floor(x + 0.5) : ceil(x - 0.5); }

// Now define fmod after trunc is defined
static f64 fmod(f64 x, f64 y) {
  if (y == 0.0)
    return 0.0 / 0.0; // NaN

  f64 n = trunc(x / y);
  return x - n * y;
}

// ------------------------------------------------------------
// Trigonometric functions
// ------------------------------------------------------------

// Reduce angle to [-π, π]
static f64 reduce_angle(f64 x) {
  x = fmod(x, M_TAU);
  if (x > M_PI)
    x -= M_TAU;
  if (x < -M_PI)
    x += M_TAU;
  return x;
}

// Sine using Taylor series
static f64 sin(f64 x) {
  x = reduce_angle(x);

  f64 term = x;
  f64 sum = x;
  f64 x2 = x * x;

  for (int i = 1; i < 15; i++) {
    term *= -x2 / ((2 * i) * (2 * i + 1));
    sum += term;
    if (fabs(term) < 1e-15)
      break;
  }

  return sum;
}

// Cosine
static f64 cos(f64 x) { return sin(M_PI_2 - x); }

// Tangent
static f64 tan(f64 x) {
  f64 c = cos(x);
  if (c == 0.0)
    return (x > 0) ? 1.0 / 0.0 : -1.0 / 0.0;
  return sin(x) / c;
}

// Arc sine using Taylor series
static f64 asin(f64 x) {
  if (x < -1.0 || x > 1.0)
    return 0.0 / 0.0; // NaN

  f64 term = x;
  f64 sum = x;
  f64 x2 = x * x;

  for (int i = 1; i < 30; i++) {
    term *= x2 * (2 * i - 1) * (2 * i - 1) / ((2 * i) * (2 * i + 1));
    sum += term;
    if (fabs(term) < 1e-15)
      break;
  }

  return sum;
}

// Arc cosine
static f64 acos(f64 x) { return M_PI_2 - asin(x); }

// Arc tangent
static f64 atan(f64 x) {
  // Use identity: atan(x) = π/2 - atan(1/x) for x > 0
  if (x > 1.0)
    return M_PI_2 - atan(1.0 / x);
  if (x < -1.0)
    return -M_PI_2 - atan(1.0 / x);

  // Taylor series for |x| ≤ 1
  f64 term = x;
  f64 sum = x;
  f64 x2 = x * x;

  for (int i = 1; i < 30; i++) {
    term *= -x2;
    sum += term / (2 * i + 1);
    if (fabs(term) < 1e-15)
      break;
  }

  return sum;
}

// Arc tangent of y/x
static f64 atan2(f64 y, f64 x) {
  if (x > 0.0)
    return atan(y / x);
  if (x < 0.0) {
    if (y >= 0.0)
      return atan(y / x) + M_PI;
    return atan(y / x) - M_PI;
  }
  // x == 0
  if (y > 0.0)
    return M_PI_2;
  if (y < 0.0)
    return -M_PI_2;
  return 0.0; // atan2(0,0) is defined as 0
}

// ------------------------------------------------------------
// Hyperbolic functions
// ------------------------------------------------------------

static f64 sinh(f64 x) {
  f64 ex = exp(x);
  f64 emx = 1.0 / ex;
  return (ex - emx) / 2.0;
}

static f64 cosh(f64 x) {
  f64 ex = exp(x);
  f64 emx = 1.0 / ex;
  return (ex + emx) / 2.0;
}

static f64 tanh(f64 x) {
  f64 ex = exp(x);
  f64 emx = 1.0 / ex;
  return (ex - emx) / (ex + emx);
}

// ------------------------------------------------------------
// Absolute value and sign functions
// ------------------------------------------------------------

static inline int abs(int x) {
  int mask = x >> (sizeof(int) * 8 - 1);
  return (x + mask) ^ mask;
}

static inline long int labs(long int x) {
  long int mask = x >> (sizeof(long int) * 8 - 1);
  return (x + mask) ^ mask;
}

static inline long long int llabs(long long int x) {
  long long int mask = x >> (sizeof(long long int) * 8 - 1);
  return (x + mask) ^ mask;
}

static inline f64 copysign(f64 x, f64 y) {
  union {
    f64 f;
    uint64_t i;
  } ux = {x};
  union {
    f64 f;
    uint64_t i;
  } uy = {y};
  ux.i = (ux.i & 0x7FFFFFFFFFFFFFFFULL) | (uy.i & 0x8000000000000000ULL);
  return ux.f;
}

static inline f64 signbit(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  return (u.i >> 63) & 1;
}

// ------------------------------------------------------------
// Special functions
// ------------------------------------------------------------

// Hypothenuse: sqrt(x² + y²) without overflow
static f64 hypot(f64 x, f64 y) {
  x = fabs(x);
  y = fabs(y);

  if (x < y) {
    f64 temp = x;
    x = y;
    y = temp;
  }

  if (x == 0.0)
    return 0.0;

  y /= x;
  return x * sqrt(1.0 + y * y);
}

// Error function using approximation
static f64 erf(f64 x) {
  // Constants
  const f64 a1 = 0.254829592;
  const f64 a2 = -0.284496736;
  const f64 a3 = 1.421413741;
  const f64 a4 = -1.453152027;
  const f64 a5 = 1.061405429;
  const f64 p = 0.3275911;

  // Save sign
  int sign = (x < 0) ? -1 : 1;
  x = fabs(x);

  // A&S formula 7.1.26
  f64 t = 1.0 / (1.0 + p * x);
  f64 y =
      1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-x * x);

  return sign * y;
}

// Complementary error function
static f64 erfc(f64 x) { return 1.0 - erf(x); }

// Gamma function approximation (Lanczos)
static f64 tgamma(f64 x) {
  // Lanczos coefficients
  const f64 g = 7.0;
  const f64 p[] = {
      0.99999999999980993,  676.5203681218851,     -1259.1392167224028,
      771.32342877765313,   -176.61502916214059,   12.507343278686905,
      -0.13857109526572012, 9.9843695780195716e-6, 1.5056327351493116e-7};

  // Reflection formula
  if (x < 0.5) {
    return M_PI / (sin(M_PI * x) * tgamma(1.0 - x));
  }

  x -= 1.0;
  f64 a = p[0];
  f64 t = x + g + 0.5;

  for (int i = 1; i < 9; i++) {
    a += p[i] / (x + i);
  }

  return sqrt(2.0 * M_PI) * pow(t, x + 0.5) * exp(-t) * a;
}

// Natural log of gamma function
static f64 lgamma(f64 x) { return log(tgamma(x)); }

// ------------------------------------------------------------
// Floating point classification
// ------------------------------------------------------------

static inline int fpclassify(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  int exp = (u.i >> 52) & 0x7FF;
  uint64_t frac = u.i & 0xFFFFFFFFFFFFFULL;

  if (exp == 0) {
    return (frac == 0) ? 0 /*FP_ZERO*/ : 2 /*FP_SUBNORMAL*/;
  } else if (exp == 0x7FF) {
    return (frac == 0) ? 1 /*FP_INFINITE*/ : 3 /*FP_NAN*/;
  }
  return 4; /*FP_NORMAL*/
}

static inline int isfinite(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  return ((u.i >> 52) & 0x7FF) != 0x7FF;
}

static inline int isinf(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  return (((u.i >> 52) & 0x7FF) == 0x7FF) && ((u.i & 0xFFFFFFFFFFFFFULL) == 0);
}

static inline int isnan(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  return (((u.i >> 52) & 0x7FF) == 0x7FF) && ((u.i & 0xFFFFFFFFFFFFFULL) != 0);
}

static inline int isnormal(f64 x) {
  union {
    f64 f;
    uint64_t i;
  } u = {x};
  int exp = (u.i >> 52) & 0x7FF;
  return (exp != 0) && (exp != 0x7FF);
}

// ------------------------------------------------------------
// Random number generation
// ------------------------------------------------------------

static uint64_t rand_seed = 1;

static void srand(uint64_t seed) { rand_seed = seed; }

static int rand(void) {
  // Simple LCG
  rand_seed = rand_seed * 1103515245 + 12345;
  return (int)((rand_seed >> 16) & 0x7FFF);
}

// Generate random double in [0, 1)
static f64 drand(void) { return (f64)rand() / (f64)(0x7FFF + 1); }

// ------------------------------------------------------------
// Vector math (useful for graphics)
// ------------------------------------------------------------

typedef struct {
  f64 x, y;
} vec2;
typedef struct {
  f64 x, y, z;
} vec3;
typedef struct {
  f64 x, y, z, w;
} vec4;

static inline vec2 vec2_add(vec2 a, vec2 b) {
  return (vec2){a.x + b.x, a.y + b.y};
}
static inline vec2 vec2_sub(vec2 a, vec2 b) {
  return (vec2){a.x - b.x, a.y - b.y};
}
static inline vec2 vec2_mul(vec2 a, f64 s) { return (vec2){a.x * s, a.y * s}; }
static inline f64 vec2_dot(vec2 a, vec2 b) { return a.x * b.x + a.y * b.y; }
static inline f64 vec2_len(vec2 a) { return sqrt(a.x * a.x + a.y * a.y); }
static inline vec2 vec2_norm(vec2 a) {
  f64 l = vec2_len(a);
  return l > 0 ? vec2_mul(a, 1.0 / l) : a;
}

static inline vec3 vec3_add(vec3 a, vec3 b) {
  return (vec3){a.x + b.x, a.y + b.y, a.z + b.z};
}
static inline vec3 vec3_sub(vec3 a, vec3 b) {
  return (vec3){a.x - b.x, a.y - b.y, a.z - b.z};
}
static inline vec3 vec3_mul(vec3 a, f64 s) {
  return (vec3){a.x * s, a.y * s, a.z * s};
}
static inline f64 vec3_dot(vec3 a, vec3 b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}
static inline vec3 vec3_cross(vec3 a, vec3 b) {
  return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x};
}
static inline f64 vec3_len(vec3 a) {
  return sqrt(a.x * a.x + a.y * a.y + a.z * a.z);
}
static inline vec3 vec3_norm(vec3 a) {
  f64 l = vec3_len(a);
  return l > 0 ? vec3_mul(a, 1.0 / l) : a;
}

// ------------------------------------------------------------
// Matrix math (4x4 for 3D transformations)
// ------------------------------------------------------------

typedef struct {
  f64 m[4][4];
} mat4;

static mat4 mat4_identity(void) {
  mat4 m = {0};
  m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0;
  return m;
}

static mat4 mat4_mul(mat4 a, mat4 b) {
  mat4 r = {0};
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      for (int k = 0; k < 4; k++) {
        r.m[i][j] += a.m[i][k] * b.m[k][j];
      }
    }
  }
  return r;
}

static vec4 mat4_mul_vec4(mat4 m, vec4 v) {
  return (vec4){
      m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w,
      m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w,
      m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w,
      m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w};
}

// Create translation matrix
static mat4 mat4_translate(f64 x, f64 y, f64 z) {
  mat4 m = mat4_identity();
  m.m[0][3] = x;
  m.m[1][3] = y;
  m.m[2][3] = z;
  return m;
}

// Create scale matrix
static mat4 mat4_scale(f64 x, f64 y, f64 z) {
  mat4 m = mat4_identity();
  m.m[0][0] = x;
  m.m[1][1] = y;
  m.m[2][2] = z;
  return m;
}

// Create rotation matrix around X axis
static mat4 mat4_rotate_x(f64 angle) {
  mat4 m = mat4_identity();
  f64 c = cos(angle);
  f64 s = sin(angle);
  m.m[1][1] = c;
  m.m[1][2] = -s;
  m.m[2][1] = s;
  m.m[2][2] = c;
  return m;
}

// Create rotation matrix around Y axis
static mat4 mat4_rotate_y(f64 angle) {
  mat4 m = mat4_identity();
  f64 c = cos(angle);
  f64 s = sin(angle);
  m.m[0][0] = c;
  m.m[0][2] = s;
  m.m[2][0] = -s;
  m.m[2][2] = c;
  return m;
}

// Create rotation matrix around Z axis
static mat4 mat4_rotate_z(f64 angle) {
  mat4 m = mat4_identity();
  f64 c = cos(angle);
  f64 s = sin(angle);
  m.m[0][0] = c;
  m.m[0][1] = -s;
  m.m[1][0] = s;
  m.m[1][1] = c;
  return m;
}

// ------------------------------------------------------------
// Interpolation functions
// ------------------------------------------------------------

// Linear interpolation
static f64 lerp(f64 a, f64 b, f64 t) { return a + t * (b - a); }

// Smoothstep
static f64 smoothstep(f64 edge0, f64 edge1, f64 x) {
  x = (x - edge0) / (edge1 - edge0);
  if (x < 0.0)
    x = 0.0;
  if (x > 1.0)
    x = 1.0;
  return x * x * (3.0 - 2.0 * x);
}

// Smootherstep
static f64 smootherstep(f64 edge0, f64 edge1, f64 x) {
  x = (x - edge0) / (edge1 - edge0);
  if (x < 0.0)
    x = 0.0;
  if (x > 1.0)
    x = 1.0;
  return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

#endif // _MATH_H
