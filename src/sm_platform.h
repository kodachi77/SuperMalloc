#pragma once

#include <stdint.h>

#if defined( _MSC_VER )

//#include <intrin.h>
//#include <nmmintrin.h>

#include <time.h>
#include "_windows.h"

#endif

#ifdef __cplusplus
extern "C"
{
#endif

unsigned long cpucores();
int           sched_getcpu();

uint64_t prandnum( void );    // a threaded pseudo-random number generator.
int inline sched_yield();
unsigned int sleep( unsigned int seconds );

#define MADV_WILLNEED 3 /* will need these pages */
#define MADV_DONTNEED 4 /* dont need these pages */

int madvise( void* addr, size_t len, int advice );
#define CLOCK_MONOTONIC 0
int clock_gettime( int X, struct timespec* tv );

#ifdef __cplusplus
}
#endif
