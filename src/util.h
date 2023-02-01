#ifndef UTIL_H
#define UTIL_H

#ifdef HAVE_ARM_ENDIAN_H
  #include <arm/endian.h>
#elif defined(HAVE_ENDIAN_H)
  #include <endian.h>
  #define BYTE_ORDER    __BYTE_ORDER
  #define LITTLE_ENDIAN __LITTLE_ENDIAN
  #define BIG_ENDIAN    __BIG_ENDIAN
#else
  #define LITTLE_ENDIAN 1234
  #define BIG_ENDIAN    4321
  #if defined(__APPLE__) || defined(_WIN32)
    #define BYTE_ORDER LITTLE_ENDIAN
  #else
    #error Not implemented
  #endif
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
  #define FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
  #define FORCE_INLINE __forceinline
#else
  #define FORCE_INLINE
#endif

#if defined(__GNUC__) || defined(__clang__)
  #define STRUCT_PACK(NAME) struct __attribute__((__packed__)) NAME
#elif defined(_MSC_VER)
  #define STRUCT_PACK(NAME) __pragma(pack(push, 1)) struct NAME __pragma(pack(pop))
#else
  #define STRUCT_PACK(NAME) struct NAME
#endif

#include <stdint.h>
#include <math.h>

static inline uint32_t FORCE_INLINE swap32(uint32_t v)
{ return (v << 24) | ((v << 8) & 0x00FF0000) | ((v >> 8) & 0x0000FF00) | (v >> 24); }

static inline uint16_t FORCE_INLINE swap16(uint16_t v)
{ return (v << 8) | (v >> 8); }

#if BYTE_ORDER == LITTLE_ENDIAN
  #define SWAP_BE32(V) swap32(V)
  #define SWAP_BE16(V) swap16(V)
#else
  #define SWAP_BE32(V) (V)
  #define SWAP_BE16(V) (V)
#endif

#if BYTE_ORDER == BIG_ENDIAN
  #define FOURCC(A, B, C, D) \
  (((uint32_t)(D)) | ((uint32_t)(C) << 8) | ((uint32_t)(B) << 16) | ((uint32_t)(A) << 24))
#else
  #define FOURCC(A, B, C, D) \
  (((uint32_t)(A)) | ((uint32_t)(B) << 8) | ((uint32_t)(C) << 16) | ((uint32_t)(D) << 24))
#endif

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))

static inline int FORCE_INLINE emod(int i, int d) { int r = i % d; return r < 0 ? r + d : r; }
static inline double FORCE_INLINE efmod(double x, double d) { double r = fmod(x, d); return r < 0.0 ? r + d : r; }
static inline float FORCE_INLINE efmodf(float x, float d) { float r = fmodf(x, d); return r < 0.0f ? r + d : r; }
#define DEG_SHORTESTDE(S, E) (efmod((S) - (E) + 180.0, 360.0) - 180.0)

#define LERP(A, B, X) ((A) * (1 - (X)) + (B) * (X))
#define DEGLERP(A, B, X) efmod(LERP((A), (A) - DEG_SHORTESTDE((A), (B)), (X)), 360.0)

#define STR(X) #X

#endif//UTIL_H
