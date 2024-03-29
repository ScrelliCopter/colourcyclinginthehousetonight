#ifndef UTIL_H
#define UTIL_H

// Detect endianess

#ifdef HAVE_ARM_ENDIAN_H
# include <arm/endian.h>
#elif defined(HAVE_ENDIAN_H)
# include <endian.h>
# define BYTE_ORDER    __BYTE_ORDER
# define LITTLE_ENDIAN __LITTLE_ENDIAN
# define BIG_ENDIAN    __BIG_ENDIAN
#else
# define LITTLE_ENDIAN 1234
# define BIG_ENDIAN    4321
# if defined(__APPLE__) || defined(_WIN32)
#  define BYTE_ORDER LITTLE_ENDIAN
# else
#  error Not implemented
# endif
#endif

// Inline control

#if (defined(__GNUC__) && (__GNUC__ >= 4)) || defined(__clang__)
# define FORCE_INLINE __attribute__((always_inline))
#elif defined(_MSC_VER)
# define FORCE_INLINE __forceinline
#else
# define FORCE_INLINE
#endif

// Struct packing

#if defined(__GNUC__) || defined(__clang__)
# define STRUCT_PACK(NAME) struct __attribute__((__packed__)) NAME
#elif defined(_MSC_VER)
# define STRUCT_PACK(NAME) __pragma(pack(push, 1)) struct NAME __pragma(pack(pop))
#else
# define STRUCT_PACK(NAME) struct NAME
#endif

// Endian handling

#include <stdint.h>

#ifdef _MSC_VER
# define swap32(X) _byteswap_ulong((X))
# define swap16(X) _byteswap_ushort((X))
#elif (__GNUC__ == 4 && __GNUC_MINOR__ >= 8) || (__GNUC__ > 4)
# define swap32(X) __builtin_bswap32((X))
# define swap16(X) __builtin_bswap16((X))
#elif defined(__has_builtin) && __has_builtin(__builtin_bswap32) && __has_builtin(__builtin_bswap16)
# define swap32(X) __builtin_bswap32((X))
# define swap16(X) __builtin_bswap16((X))
#else
static inline uint32_t FORCE_INLINE swap32(uint32_t v) { return v << 24 | (v & 0xFF00) << 8 | (v & 0xFF0000) >> 8 | v >> 24; }
static inline uint16_t FORCE_INLINE swap16(uint16_t v) { return v << 8 | v >> 8; }
#endif

#if BYTE_ORDER == LITTLE_ENDIAN
# define SWAP_BE32(V) swap32(V)
# define SWAP_BE16(V) swap16(V)
#else
# define SWAP_BE32(V) (V)
# define SWAP_BE16(V) (V)
#endif

#if BYTE_ORDER == BIG_ENDIAN
# define FOURCC(A, B, C, D) ((uint32_t)(D) | (uint32_t)(C) << 8 | (uint32_t)(B) << 16 | (uint32_t)(A) << 24)
#else
# define FOURCC(A, B, C, D) ((uint32_t)(A) | (uint32_t)(B) << 8 | (uint32_t)(C) << 16 | (uint32_t)(D) << 24)
#endif

// Maths utilities

#include <math.h>

#define MIN(A, B) (((A) < (B)) ? (A) : (B))
#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define CLAMP(X, A, B) (MIN((B), MAX((A), (X))))
#define SATURATE(X) (CLAMP((X), 0, 1))

static inline int FORCE_INLINE emod(int i, int d) { int r = i % d; return r < 0 ? r + d : r; }
static inline double FORCE_INLINE efmod(double x, double d) { double r = fmod(x, d); return r < 0.0 ? r + d : r; }
static inline float FORCE_INLINE efmodf(float x, float d) { float r = fmodf(x, d); return r < 0.0f ? r + d : r; }
#define DEG_SHORTEST(S, E) (efmod((S) - (E) + 180.0, 360.0) - 180.0)

#define LERP(A, B, X) ((A) * (1 - (X)) + (B) * (X))
#define DEGLERP(A, B, X) efmod(LERP((A), (A) - DEG_SHORTEST((A), (B)), (X)), 360.0)

// Sized buffers

#include <stddef.h>

typedef struct { void* ptr; size_t len; } SizedBuf;

#define BUF_SIZED(P, S) (SizedBuf){ (void*)(P), (S) }
#define BUF_CLEAR() { NULL, 0U }
#define BUF_EMPTY(B) (!(B).ptr)
#define BUF_FREE(B) ((B) = !(B).ptr ? free((B).ptr), (B) : (SizedBuf)BUF_CLEAR())

typedef struct { char* ptr; size_t len; } SizedStr;

#define STR_SIZED(S, L) (SizedStr){ (S), (L) }
#define STR_ALLOC(L) STR_SIZED(malloc((L) + 1), (L))
#define STR_CLEAR() { NULL, 0U }
#define STR_EMPTY(B) (!(B).ptr || !(B).len)
#define STR_FREE(B) ((B) = !(B).ptr ? free((B).ptr), (B) : (SizedStr)STR_CLEAR())

// Disable warnings

#define WRAP_PRAGMA(X) _Pragma(#X)
#if defined(__GNUC__) || defined(__clang__)
# define GCC_NOWARN(X) WRAP_PRAGMA(GCC diagnostic push) WRAP_PRAGMA(GCC diagnostic ignored #X)
# define GCC_ENDNOWARN() WRAP_PRAGMA(GCC diagnostic pop)
# define MSVC_NOWARN(X)
# define MSVC_ENDNOWARN()
#elif defined(_MSC_VER)
# define GCC_NOWARN(X)
# define GCC_ENDNOWARN()
# define MSVC_NOWARN(X) WRAP_PRAGMA(warning(push)) WRAP_PRAGMA(warning(disable:##X##))
# define MSVC_ENDNOWARN() WRAP_PRAGMA(warning(pop))
#else
# define GCC_NOWARN(X)
# define GCC_ENDNOWARN()
# define MSVC_NOWARN(X)
# define MSVC_ENDNOWARN()
#endif

#endif//UTIL_H
