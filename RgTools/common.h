#ifndef __COMMON_H__
#define __COMMON_H__

#include <algorithm>
#define NOMINMAX
#include <Windows.h>
#pragma warning(disable: 4512 4244 4100)
#include "avisynth.h"
#pragma warning(default: 4512 4244 4100)
#include <smmintrin.h>

typedef unsigned char Byte;

#define RG_FORCEINLINE __forceinline

#define USE_MOVPS

enum InstructionSet {
    SSE2,
    SSE3
};

template<typename T>
static RG_FORCEINLINE Byte clip(T val, T minimum, T maximum) {
    return std::max(std::min(val, maximum), minimum);
}

// avs+
template<typename T>
static RG_FORCEINLINE uint16_t clip_16(T val, T minimum, T maximum) {
  return std::max(std::min(val, maximum), minimum);
}

// avs+
template<typename T>
static RG_FORCEINLINE float clip_32(T val, T minimum, T maximum) {
  return std::max(std::min(val, maximum), minimum);
}

static RG_FORCEINLINE bool is_16byte_aligned(const void *ptr) {
    return (((uintptr_t)ptr) & 15) == 0;
}

static RG_FORCEINLINE __m128i simd_clip(const __m128i &val, const __m128i &minimum, const __m128i &maximum) {
  return _mm_max_epu8(_mm_min_epu8(val, maximum), minimum);
}

// SSE4!
// PF avs+
static RG_FORCEINLINE __m128i simd_clip_16(const __m128i &val, const __m128i &minimum, const __m128i &maximum) {
  return _mm_max_epu16(_mm_min_epu16(val, maximum), minimum); // SSE4
}

// PF avs+
static RG_FORCEINLINE __m128 simd_clip_32(const __m128 &val, const __m128 &minimum, const __m128 &maximum) {
  return _mm_max_ps(_mm_min_ps(val, maximum), minimum); 
}



static RG_FORCEINLINE void sort_pair(__m128i &a1, __m128i &a2)
{
  const __m128i tmp = _mm_min_epu8 (a1, a2);
  a2 = _mm_max_epu8 (a1, a2);
  a1 = tmp;
}

// SSE4
// PF avs+
static RG_FORCEINLINE void sort_pair_16(__m128i &a1, __m128i &a2)
{
  const __m128i tmp = _mm_min_epu16 (a1, a2);
  a2 = _mm_max_epu16 (a1, a2);
  a1 = tmp;
}

// PF avs+
static RG_FORCEINLINE void sort_pair_32(__m128 &a1, __m128 &a2)
{
  const __m128 tmp = _mm_min_ps (a1, a2);
  a2 = _mm_max_ps (a1, a2);
  a1 = tmp;
}

// _mm_lddqu_si128: still faster on i7-3770 vs _mm_loadu_si128
template<InstructionSet optLevel>
static RG_FORCEINLINE __m128i simd_loadu_si128(const Byte* ptr) {
  if (optLevel == SSE2) {
#ifdef USE_MOVPS
        return _mm_castps_si128(_mm_loadu_ps(reinterpret_cast<const float*>(ptr)));
#else
        return _mm_loadu_si128(reinterpret_cast<const __m128i*>(ptr));
#endif
    }
    return _mm_lddqu_si128(reinterpret_cast<const __m128i*>(ptr));
}

template<InstructionSet optLevel>
static RG_FORCEINLINE __m128i simd_loada_si128(const Byte* ptr) {
  if (optLevel == SSE2) {
#ifdef USE_MOVPS
    return _mm_castps_si128(_mm_load_ps(reinterpret_cast<const float*>(ptr)));
#else
    return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
#endif
  }
  return _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
}

//mask ? a : b
static RG_FORCEINLINE __m128i blend(__m128i const &mask, __m128i const &desired, __m128i const &otherwise) {
  //return  _mm_blendv_epi8 (otherwise, desired, mask);
  auto andop = _mm_and_si128(mask , desired);
  auto andnop = _mm_andnot_si128(mask, otherwise);
  return _mm_or_si128(andop, andnop);
}

//avs+
//sse4
static RG_FORCEINLINE __m128i blend_16(__m128i const &mask, __m128i const &desired, __m128i const &otherwise) {
  return  _mm_blendv_epi8 (otherwise, desired, mask); // no need for epi 16 here
  //auto andop = _mm_and_si128(mask , desired);
  //auto andnop = _mm_andnot_si128(mask, otherwise);
  //return _mm_or_si128(andop, andnop);
}

// sse4
static RG_FORCEINLINE __m128 blend_32(__m128 const &mask, __m128 const &desired, __m128 const &otherwise) {
  return  _mm_blendv_ps (otherwise, desired, mask);
  //auto andop = _mm_and_si128(mask , desired);
  //auto andnop = _mm_andnot_si128(mask, otherwise);
  //return _mm_or_si128(andop, andnop);
}


static RG_FORCEINLINE __m128i abs_diff(__m128i a, __m128i b) {
  //return  _mm_blendv_epi8 (otherwise, desired, mask); // SSE4
  auto positive = _mm_subs_epu8(a, b);
  auto negative = _mm_subs_epu8(b, a);
  return _mm_or_si128(positive, negative);
}

// avs+ todo for sse4
static RG_FORCEINLINE __m128i abs_diff_16(__m128i a, __m128i b) {
  //return  _mm_blendv_epi16 (otherwise, desired, mask); // SSE4
  auto positive = _mm_subs_epu16(a, b);
  auto negative = _mm_subs_epu16(b, a);
  return _mm_or_si128(positive, negative);
}

// avs+
static RG_FORCEINLINE __m128 abs_diff_32(__m128 a, __m128 b) {
  // maybe not optimal, mask may be generated 
  const __m128 absmask = _mm_castsi128_ps(_mm_set1_epi32(~(1<<31))); // 0x7FFFFFFF
  return _mm_and_ps(_mm_sub_ps(a, b), absmask);
}

// PF until I find out better
static RG_FORCEINLINE __m128 _mm_subs_ps(__m128 a, __m128 b) {
#if 0
const __m128 zero = _mm_setzero_ps();
return _mm_max_ps(_mm_sub_ps(a, b), zero);
#else
  // no float clamp
  return _mm_sub_ps(a, b);
#endif
}

// PF until I find out better
static RG_FORCEINLINE __m128 _mm_adds_ps(__m128 a, __m128 b) {
#if 0
  const __m128 one = _mm_set1_ps(1.0f);
  return _mm_min_ps(_mm_add_ps(a, b), one);
#else
  // no float clamp
  return _mm_add_ps(a, b);
#endif
}

// PF until I find out better
static RG_FORCEINLINE __m128 _mm_avg_ps(__m128 a, __m128 b) {
  const __m128 div2 = _mm_set1_ps(0.5f);
  return _mm_mul_ps(_mm_add_ps(a, b), div2);
}

static RG_FORCEINLINE __m128i select_on_equal(const __m128i &cmp1, const __m128i &cmp2, const __m128i &current, const __m128i &desired) {
  auto eq = _mm_cmpeq_epi8(cmp1, cmp2);
  return blend(eq, desired, current);
}

static RG_FORCEINLINE __m128i select_on_equal_16(const __m128i &cmp1, const __m128i &cmp2, const __m128i &current, const __m128i &desired) {
  auto eq = _mm_cmpeq_epi16(cmp1, cmp2);
  return blend_16(eq, desired, current);
}

static RG_FORCEINLINE __m128 select_on_equal_32(const __m128 &cmp1, const __m128 &cmp2, const __m128 &current, const __m128 &desired) {
  auto eq = _mm_cmpeq_ps(cmp1, cmp2);
  return blend_32(eq, desired, current);
}

#define LOAD_SQUARE_SSE_0_18(optLevel, ptr, pitch, pixelsize, aligned) \
__m128i a1, a8; \
if(!aligned) {\
a1 = simd_loadu_si128<optLevel>((ptr) - (pitch) - (pixelsize)); \
a8 = simd_loadu_si128<optLevel>((ptr) + (pitch) + (pixelsize)); \
} else {\
a1 = simd_loadu_si128<optLevel>((ptr) - (pitch) - (pixelsize)); \
a8 = simd_loadu_si128<optLevel>((ptr) + (pitch) + (pixelsize)); \
}

#define LOAD_SQUARE_SSE_0_27(optLevel, ptr, pitch, pixelsize, aligned) \
__m128i a2, a7; \
if(!aligned) {\
a2 = simd_loadu_si128<optLevel>((ptr) - (pitch)); \
a7 = simd_loadu_si128<optLevel>((ptr) + (pitch)); \
} else {\
a2 = simd_loada_si128<optLevel>((ptr) - (pitch)); \
a7 = simd_loada_si128<optLevel>((ptr) + (pitch)); \
}

#define LOAD_SQUARE_SSE_0_36(optLevel, ptr, pitch, pixelsize, aligned) \
__m128i a3, a6; \
if(!aligned) {\
a3 = simd_loadu_si128<optLevel>((ptr) - (pitch) + (pixelsize)); \
a6 = simd_loadu_si128<optLevel>((ptr) + (pitch) - (pixelsize)); \
} else {\
a3 = simd_loadu_si128<optLevel>((ptr) - (pitch) + (pixelsize)); \
a6 = simd_loadu_si128<optLevel>((ptr) + (pitch) - (pixelsize)); \
}

#define LOAD_SQUARE_SSE_0_45(optLevel, ptr, pitch, pixelsize, aligned) \
__m128i a4, a5; \
if(!aligned) {\
a4 = simd_loadu_si128<optLevel>((ptr) - (pixelsize)); \
a5 = simd_loadu_si128<optLevel>((ptr) + (pixelsize)); \
} else {\
a4 = simd_loadu_si128<optLevel>((ptr) - (pixelsize)); \
a5 = simd_loadu_si128<optLevel>((ptr) + (pixelsize)); \
}

#define LOAD_SQUARE_SSE_0_Cent(optLevel, ptr, pitch, pixelsize, aligned) \
__m128i c; \
if(!aligned) {\
c  = simd_loadu_si128<optLevel>((ptr) ); \
} else {\
c  = simd_loada_si128<optLevel>((ptr) ); \
}

#define LOAD_SQUARE_SSE_0(optLevel, ptr, pitch, pixelsize, aligned) \
__m128i a1, a2, a3, a4, a5, a6, a7, a8, c; \
if(!aligned) {\
a1 = simd_loadu_si128<optLevel>((ptr) - (pitch) - (pixelsize)); \
a2 = simd_loadu_si128<optLevel>((ptr) - (pitch)); \
a3 = simd_loadu_si128<optLevel>((ptr) - (pitch) + (pixelsize)); \
a4 = simd_loadu_si128<optLevel>((ptr) - (pixelsize)); \
c  = simd_loadu_si128<optLevel>((ptr) ); \
a5 = simd_loadu_si128<optLevel>((ptr) + (pixelsize)); \
a6 = simd_loadu_si128<optLevel>((ptr) + (pitch) - (pixelsize)); \
a7 = simd_loadu_si128<optLevel>((ptr) + (pitch)); \
a8 = simd_loadu_si128<optLevel>((ptr) + (pitch) + (pixelsize)); \
} else {\
a1 = simd_loadu_si128<optLevel>((ptr) - (pitch) - (pixelsize)); \
a2 = simd_loada_si128<optLevel>((ptr) - (pitch)); \
a3 = simd_loadu_si128<optLevel>((ptr) - (pitch) + (pixelsize)); \
a4 = simd_loadu_si128<optLevel>((ptr) - (pixelsize)); \
c  = simd_loada_si128<optLevel>((ptr) ); \
a5 = simd_loadu_si128<optLevel>((ptr) + (pixelsize)); \
a6 = simd_loadu_si128<optLevel>((ptr) + (pitch) - (pixelsize)); \
a7 = simd_loada_si128<optLevel>((ptr) + (pitch)); \
a8 = simd_loadu_si128<optLevel>((ptr) + (pitch) + (pixelsize)); \
}

// 8 bit loads
// unaligned
#define LOAD_SQUARE_SSE(optLevel, ptr, pitch) LOAD_SQUARE_SSE_0(optLevel, ptr, pitch, 1, false)
// unaligned or aligned
#define LOAD_SQUARE_SSE_UA(optLevel, ptr, pitch, aligned) LOAD_SQUARE_SSE_0(optLevel, ptr, pitch, 1, aligned)
#define LOAD_SQUARE_SSE_UA_18(optLevel, ptr, pitch, aligned) LOAD_SQUARE_SSE_0_18(optLevel, ptr, pitch, 1, aligned)
#define LOAD_SQUARE_SSE_UA_27(optLevel, ptr, pitch, aligned) LOAD_SQUARE_SSE_0_27(optLevel, ptr, pitch, 1, aligned)
#define LOAD_SQUARE_SSE_UA_36(optLevel, ptr, pitch, aligned) LOAD_SQUARE_SSE_0_36(optLevel, ptr, pitch, 1, aligned)
#define LOAD_SQUARE_SSE_UA_45(optLevel, ptr, pitch, aligned) LOAD_SQUARE_SSE_0_45(optLevel, ptr, pitch, 1, aligned)
#define LOAD_SQUARE_SSE_UA_Cent(optLevel, ptr, pitch, aligned) LOAD_SQUARE_SSE_0_Cent(optLevel, ptr, pitch, 1, aligned)

// 16 bit loads
// unaligned
#define LOAD_SQUARE_SSE_16(ptr, pitch) LOAD_SQUARE_SSE_0(SSE3, ptr, pitch, 2, false)
// unaligned or aligned
#define LOAD_SQUARE_SSE_16_UA(ptr, pitch, aligned) LOAD_SQUARE_SSE_0(SSE3, ptr, pitch, 2, aligned)

// 32 bit float loads
#define LOAD_SQUARE_SSE_0_32(ptr, pitch, aligned) \
__m128 a1, a2, a3, a4, a5, a6, a7, a8, c; \
if(!aligned) {\
a1 = _mm_loadu_ps((const float *)((ptr) - (pitch) - 4)); \
a2 = _mm_loadu_ps((const float *)((ptr) - (pitch))); \
a3 = _mm_loadu_ps((const float *)((ptr) - (pitch) + (4))); \
a4 = _mm_loadu_ps((const float *)((ptr) - (4))); \
c  = _mm_loadu_ps((const float *)((ptr) )); \
a5 = _mm_loadu_ps((const float *)((ptr) + (4))); \
a6 = _mm_loadu_ps((const float *)((ptr) + (pitch) - (4))); \
a7 = _mm_loadu_ps((const float *)((ptr) + (pitch))); \
a8 = _mm_loadu_ps((const float *)((ptr) + (pitch) + (4))); \
} else { \
a1 = _mm_loadu_ps((const float *)((ptr) - (pitch) - 4)); \
a2 = _mm_load_ps((const float *)((ptr) - (pitch))); \
a3 = _mm_loadu_ps((const float *)((ptr) - (pitch) + (4))); \
a4 = _mm_loadu_ps((const float *)((ptr) - (4))); \
c  = _mm_load_ps((const float *)((ptr) )); \
a5 = _mm_loadu_ps((const float *)((ptr) + (4))); \
a6 = _mm_loadu_ps((const float *)((ptr) + (pitch) - (4))); \
a7 = _mm_load_ps((const float *)((ptr) + (pitch))); \
a8 = _mm_loadu_ps((const float *)((ptr) + (pitch) + (4))); \
}

// unaligned
#define LOAD_SQUARE_SSE_32(ptr, pitch) LOAD_SQUARE_SSE_0_32(ptr, pitch, false) 
// unaligned or aligned
#define LOAD_SQUARE_SSE_32_UA(ptr, pitch, aligned) LOAD_SQUARE_SSE_0_32(ptr, pitch, aligned)

// loaders for C routines
// pointers and pitch are byte-based
#define LOAD_SQUARE_CPP_0(pixel_t, ptr, pitch) \
    pixel_t a1 = *(pixel_t *)((ptr) - (pitch) - sizeof(pixel_t)); \
    pixel_t a2 = *(pixel_t *)((ptr) - (pitch)); \
    pixel_t a3 = *(pixel_t *)((ptr) - (pitch) + sizeof(pixel_t)); \
    pixel_t a4 = *(pixel_t *)((ptr) - sizeof(pixel_t)); \
    pixel_t c  = *(pixel_t *)((ptr) ); \
    pixel_t a5 = *(pixel_t *)((ptr) + sizeof(pixel_t)); \
    pixel_t a6 = *(pixel_t *)((ptr) + (pitch) - sizeof(pixel_t)); \
    pixel_t a7 = *(pixel_t *)((ptr) + (pitch)); \
    pixel_t a8 = *(pixel_t *)((ptr) + (pitch) + sizeof(pixel_t));

#define LOAD_SQUARE_CPP(ptr, pitch) LOAD_SQUARE_CPP_0(Byte, ptr, pitch);
#define LOAD_SQUARE_CPP_16(ptr, pitch) LOAD_SQUARE_CPP_0(uint16_t, ptr, pitch);
#define LOAD_SQUARE_CPP_32(ptr, pitch) LOAD_SQUARE_CPP_0(float, ptr, pitch);

/*
#define LOAD_SQUARE_CPP(ptr, pitch) \
    Byte a1 = *((ptr) - (pitch) - 1); \
    Byte a2 = *((ptr) - (pitch)); \
    Byte a3 = *((ptr) - (pitch) + 1); \
    Byte a4 = *((ptr) - 1); \
    Byte c  = *((ptr) ); \
    Byte a5 = *((ptr) + 1); \
    Byte a6 = *((ptr) + (pitch) - 1); \
    Byte a7 = *((ptr) + (pitch)); \
    Byte a8 = *((ptr) + (pitch) + 1);
*/
#endif
