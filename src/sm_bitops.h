/* sm_builtins.h
 * Cross-platform wrappers for bit builtins:
 *   ffs, ctz, clz, clrsb, popcount
 * C11, MSVC          /GCC/Clang, x64 & ARM64
 */
#ifndef SM_BUILTINS_H
#define SM_BUILTINS_H

#include <stdint.h>

#if __STDC_VERSION__ < 201112L
#error "sm_builtins.h requires C11 (_Generic used)"
#endif

/* ---- 32/64-bit core implementations ---------------------------------- */

#if defined( _MSC_VER ) && !defined( __clang__ )
/* ================================ MSVC ================================= */
#include <intrin.h>

/* ctz: undefined if x == 0 (matches GCC/Clang builtins) */
static inline int
sm_ctz32( uint32_t x )
{
    unsigned long idx; /* NOLINT(misc-unused-parameters) */
    _BitScanForward( &idx, x );
    return (int) idx;
}
static inline int
sm_ctz64( uint64_t x )
{
    unsigned long idx;
    _BitScanForward64( &idx, x );
    return (int) idx;
}

/* clz: undefined if x == 0 */
static inline int
sm_clz32( uint32_t x )
{
    unsigned long idx;
    _BitScanReverse( &idx, x ); /* idx in [0..31], MSB position */
    return 31 - (int) idx;
}
static inline int
sm_clz64( uint64_t x )
{
    unsigned long idx;
    _BitScanReverse64( &idx, x );
    return 63 - (int) idx;
}

/* ffs: return 0 if x == 0; else 1 + index of least-significant 1 bit */
static inline int
sm_ffs32( uint32_t x )
{
    return x ? ( sm_ctz32( x ) + 1 ) : 0;
}
static inline int
sm_ffs64( uint64_t x )
{
    return x ? ( sm_ctz64( x ) + 1 ) : 0;
}

/* popcount: use intrinsics where available; otherwise software fallback */
static inline int
sm_popcount32( uint32_t x )
{
#if defined( _M_X64 )
    return (int) __popcnt( x );
#else /* ARM64 or others: software popcount */
    // https://nimrod.blog/posts/algorithms-behind-popcount/
    x -= ( x >> 1 ) & 0x55555555u;
    x = ( x & 0x33333333u ) + ( ( x >> 2 ) & 0x33333333u );
    x = ( x + ( x >> 4 ) ) & 0x0F0F0F0Fu;
    return (int) ( ( x * 0x01010101u ) >> 24 );
#endif
}
static inline int
sm_popcount64( uint64_t x )
{
#if defined( _M_X64 )
    return (int) __popcnt64( x );
#else
    /* ARM64 fallback */
    return sm_popcount32( (uint32_t) x ) + sm_popcount32( (uint32_t) ( x >> 32 ) );
#endif
}

/* clrsb (count leading redundant sign bits, excluding the sign bit) */
static inline int
sm_clrsb32( int32_t v )
{
    /* For v>=0: clrsb = clz(v) - 1 ; For v<0: clrsb = clz(~v) - 1 */
    return ( v >= 0 ) ? ( sm_clz32( (uint32_t) v ) - 1 ) : ( sm_clz32( ~(uint32_t) v ) - 1 );
}
static inline int
sm_clrsb64( int64_t v )
{
    return ( v >= 0 ) ? ( sm_clz64( (uint64_t) v ) - 1 ) : ( sm_clz64( ~(uint64_t) v ) - 1 );
}

#elif defined( __clang__ )
/* =============================== Clang ================================ */
static inline int
sm_ctz32( uint32_t x )
{
    return __builtin_ctz( x );
}
static inline int
sm_ctz64( uint64_t x )
{
    return __builtin_ctzll( x );
}
static inline int
sm_clz32( uint32_t x )
{
    return __builtin_clz( x );
}
static inline int
sm_clz64( uint64_t x )
{
    return __builtin_clzll( x );
}
static inline int
sm_ffs32( uint32_t x )
{
    return __builtin_ffs( (int) x );
}
static inline int
sm_ffs64( uint64_t x )
{
    return __builtin_ffsll( (long long) x );
}
static inline int
sm_popcount32( uint32_t x )
{
    return __builtin_popcount( x );
}
static inline int
sm_popcount64( uint64_t x )
{
    return __builtin_popcountll( x );
}
static inline int
sm_clrsb32( int32_t x )
{
    return __builtin_clrsb( x );
}
static inline int
sm_clrsb64( int64_t x )
{
    return __builtin_clrsbll( x );
}

#elif defined( __GNUC__ )
/* =============================== GCC ================================= */
static inline int
sm_ctz32( uint32_t x )
{
    return __builtin_ctz( x );
}
static inline int
sm_ctz64( uint64_t x )
{
    return __builtin_ctzll( x );
}
static inline int
sm_clz32( uint32_t x )
{
    return __builtin_clz( x );
}
static inline int
sm_clz64( uint64_t x )
{
    return __builtin_clzll( x );
}
static inline int
sm_ffs32( uint32_t x )
{
    return __builtin_ffs( (int) x );
}
static inline int
sm_ffs64( uint64_t x )
{
    return __builtin_ffsll( (long long) x );
}
static inline int
sm_popcount32( uint32_t x )
{
    return __builtin_popcount( x );
}
static inline int
sm_popcount64( uint64_t x )
{
    return __builtin_popcountll( x );
}
static inline int
sm_clrsb32( int32_t x )
{
    return __builtin_clrsb( x );
}
static inline int
sm_clrsb64( int64_t x )
{
    return __builtin_clrsbll( x );
}

#else
#error "Unsupported compiler for sm_builtins.h"
#endif

/* ---- Public SM_BUILTIN_* dispatchers (work with common integer types) -- */

//#define SM__RET_I int
//#define SM__U32   uint32_t
//#define SM__U64   uint64_t
//#define SM__I32   int32_t
//#define SM__I64   int64_t

/* Map common integer types to 32/64-bit helpers.
   Note: if you pass 'unsigned long', this chooses 32- or 64-bit
   based on the platform’s 'long' width. */
#define SM__DISPATCH_U32_U64( expr32, expr64, x )                                                                                \
    _Generic( ( x ),                                                                                                             \
        unsigned int: expr32,                                                                                                    \
        int: expr32,                                                                                                             \
        /*SM__U32: expr32,*/                                                                                                         \
        unsigned long: sizeof( unsigned long ) == 8 ? expr64 : expr32,                                                           \
        long: sizeof( long ) == 8 ? expr64 : expr32,                                                                             \
        unsigned long long: expr64,                                                                                              \
        long long: expr64/*,                                                                                                       \
        SM__U64: expr64*/ )

#define SM__DISPATCH_I32_I64( expr32, expr64, x )                                                                                \
    _Generic( ( x ),                                                                                                             \
        int: expr32,                                                                                                             \
        unsigned int: expr32,                                                                                                    \
        /*SM__I32: expr32,*/                                                                                                         \
        long: sizeof( long ) == 8 ? expr64 : expr32,                                                                             \
        unsigned long: sizeof( unsigned long ) == 8 ? expr64 : expr32,                                                           \
        long long: expr64,                                                                                                       \
        unsigned long long: expr64/*,                                                                                              \
        SM__I64: expr64*/ )

/* Public macros (evaluate argument once via inline wrappers) */
#define SM_BUILTIN_CTZ( x ) SM__DISPATCH_U32_U64( sm_ctz32( (uint32_t) ( x ) ), sm_ctz64( (uint64_t) ( x ) ), ( x ) )
#define SM_BUILTIN_CLZ( x ) SM__DISPATCH_U32_U64( sm_clz32( (uint32_t) ( x ) ), sm_clz64( (uint64_t) ( x ) ), ( x ) )
#define SM_BUILTIN_FFS( x ) SM__DISPATCH_U32_U64( sm_ffs32( (uint32_t) ( x ) ), sm_ffs64( (uint64_t) ( x ) ), ( x ) )
#define SM_BUILTIN_POPCOUNT( x )                                                                                                 \
    SM__DISPATCH_U32_U64( sm_popcount32( (uint32_t) ( x ) ), sm_popcount64( (uint64_t) ( x ) ), ( x ) )
#define SM_BUILTIN_CLRSB( x ) SM__DISPATCH_I32_I64( sm_clrsb32( (int32_t) ( x ) ), sm_clrsb64( (int64_t) ( x ) ), ( x ) )

/* Optional: explicit-width versions if you prefer no _Generic */
#define SM_BUILTIN_CTZ32( x )      sm_ctz32( (uint32_t) ( x ) )
#define SM_BUILTIN_CTZ64( x )      sm_ctz64( (uint64_t) ( x ) )
#define SM_BUILTIN_CLZ32( x )      sm_clz32( (uint32_t) ( x ) )
#define SM_BUILTIN_CLZ64( x )      sm_clz64( (uint64_t) ( x ) )
#define SM_BUILTIN_FFS32( x )      sm_ffs32( (uint32_t) ( x ) )
#define SM_BUILTIN_FFS64( x )      sm_ffs64( (uint64_t) ( x ) )
#define SM_BUILTIN_POPCOUNT32( x ) sm_popcount32( (uint32_t) ( x ) )
#define SM_BUILTIN_POPCOUNT64( x ) sm_popcount64( (uint64_t) ( x ) )
#define SM_BUILTIN_CLRSB32( x )    sm_clrsb32( (int32_t) ( x ) )
#define SM_BUILTIN_CLRSB64( x )    sm_clrsb64( (int64_t) ( x ) )

/* Notes / preconditions (match GCC/Clang builtins):
   - SM_BUILTIN_CTZ(x), SM_BUILTIN_CLZ(x) are undefined if x == 0.
   - SM_BUILTIN_CLRSB(x) is undefined for x == 0? (GCC defines it for any x, but
     the count is only meaningful for 2’s-complement; our implementation assumes that.)
   - SM_BUILTIN_FFS(x) returns 0 when x == 0.
*/

#endif /* SM_BUILTINS_H */
