#pragma once

#include "sm_config.h"

//> TODO: use generics

static inline uint32_t
__builtin_ffs( uint64_t v )
{
    unsigned long index;
    return (uint32_t) _BitScanForward64( &index, (unsigned long long) v ) ? index + 1 : 0;
}

#define __builtin_popcount  __popcnt
#define __builtin_popcountl _mm_popcnt_u64

static inline uint32_t
__builtin_ctzl( uint64_t value )
{
    unsigned long index;
    return (uint32_t) _BitScanForward64( &index, value ) ? index : 64;
}

static inline uint32_t
__builtin_clzl( uint64_t value )
{
    unsigned long index;
    return (uint32_t) _BitScanReverse64( &index, value ) ? 63 - index : 64;
}
