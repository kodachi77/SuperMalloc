#ifndef CONFIG_H
#define CONFIG_H

//#include <inttypes.h>

#if defined( _MSC_VER )

#ifndef _WIN64
#error "This allocator only supports 64-bit platforms."
#endif

#include <intrin.h>
#include <nmmintrin.h>
#include <stdint.h>
#include <time.h>

#define WINVER       0x0A00
#define _WIN32_WINNT 0x0A00

#ifndef UNICODE
#define UNICODE
#endif

#define WIN32_LEAN_AND_MEAN
#define STRICT
#define NOGDICAPMASKS
#define NOMENUS
#define NOICONS
#define NORASTEROPS
#define NOOEMRESOURCE
#define NOATOM
#define NOCLIPBOARD
#define NOCOLOR
#define NOCTLMGR
#define NONLS
#define NOMETAFILE
#define NOMINMAX
#define NOSCROLL
#define NOSERVICE
#define NOSOUND
#define NOWH
#define NOCOMM
#define NOKANJI
#define NOHELP
#define NOPROFILER
#define NODEFERWINDOWPOS
#define NOMCX

#include <windows.h>

#define ALIGNED( num )   __declspec( align( ( num ) ) )
#define ATTRIBUTE_THREAD __declspec( thread )
#define ATTRIBUTE_UNROLL

#define LOCK_INITIALIZER                                                                                                         \
    {                                                                                                                            \
    }

#define __THROW

int inline sched_yield()
{
    ::Sleep( 0 );
    return 0;
}

unsigned int inline sleep( unsigned int seconds )
{
    ::Sleep( seconds * 1000 );
    return 0;
}

#define MADV_WILLNEED 3 /* will need these pages */
#define MADV_DONTNEED 4 /* dont need these pages */

int inline madvise( void* addr, size_t len, int advice )
{
    int ret = 0;
    if( advice < MADV_WILLNEED || advice > MADV_DONTNEED || !len )
    {
        ret = EINVAL;
        goto out;
    }

    MEMORY_BASIC_INFORMATION m;
    char *                   p, *endp;

    for( p = (char*) addr, endp = p + len; p < endp; p = (char*) m.BaseAddress + m.RegionSize )
    {
        if( !VirtualQuery( p, &m, sizeof m ) || m.State == MEM_FREE )
        {
            ret = ENOMEM;
            break;
        }
    }
#define rounddown( x, y )            ( ( ( x ) / ( y ) ) * ( y ) )
#define roundup( x, y )              ( ( ( ( x ) + ( (y) -1 ) ) / ( y ) ) * ( y ) ) /* to any y */
#define roundup2( x, y )             ( ( ( x ) + ( (y) -1 ) ) & ( ~( (y) -1 ) ) )   /* if y is powers of two */
#define geterrno_from_win_error( x ) ( 13 )

    if( ret ) goto out;
    switch( advice )
    {
        case MADV_WILLNEED:
        {
            /* Align address and length values to page size. */
            const size_t             pagesize = 4096;    // TODO: safe enough???
            PVOID                    base     = (PVOID) rounddown( (uintptr_t) addr, pagesize );
            SIZE_T                   size     = roundup2( ( (uintptr_t) addr - (uintptr_t) base ) + len, pagesize );
            WIN32_MEMORY_RANGE_ENTRY me       = { base, size };
            if( !PrefetchVirtualMemory( GetCurrentProcess(), 1, &me, 0 ) && GetLastError() != ERROR_PROC_NOT_FOUND )
            {
                if( GetLastError() != ERROR_INVALID_PARAMETER ) ret = EINVAL;
            }
        }
        break;
        case MADV_DONTNEED:
        {
            // Align address and length values to page size
            const size_t pagesize = 4096;    // TODO: safe enough???
            PVOID        base     = (PVOID) rounddown( (uintptr_t) addr, pagesize );
            SIZE_T       size     = roundup2( ( (uintptr_t) addr - (uintptr_t) base ) + len, pagesize );
            DWORD        err      = DiscardVirtualMemory( base, size );
            /* DiscardVirtualMemory is unfortunately pretty crippled:
		       On copy-on-write pages it returns ERROR_INVALID_PARAMETER, on
		       any file-backed memory map it returns ERROR_USER_MAPPED_FILE.
		       Since POSIX_MADV_DONTNEED is advisory only anyway, let them
		       slip through. */
            switch( err )
            {
                case ERROR_PROC_NOT_FOUND:
                case ERROR_USER_MAPPED_FILE:
                case 0: break;
                case ERROR_INVALID_PARAMETER:
                {
                    ret = EINVAL;
                    /* Check if the region contains copy-on-write pages.*/
                    for( p = (char*) addr, endp = p + len; p < endp; p = (char*) m.BaseAddress + m.RegionSize )
                    {
                        if( VirtualQuery( p, &m, sizeof m ) && m.State == MEM_COMMIT
                            && m.Protect & ( PAGE_EXECUTE_WRITECOPY | PAGE_WRITECOPY ) )
                        {
                            /* Yes, let this slip. */
                            ret = 0;
                            break;
                        }
                    }
                }
                break;
                // 0x000001e7 - Attempt to access invalid address.
                default: ret = geterrno_from_win_error( err ); break;
            }
        }
        break;
        default: break;
    }
out:
    //if (0) fprintf( stderr, "%d = madvise(%p, %lu, %d)", ret, addr, len, advice );
    return ret;
}

#pragma intrinsic( _InterlockedExchange )
#pragma intrinsic( _InterlockedExchangeAdd )
#pragma intrinsic( _InterlockedCompareExchange )
#pragma intrinsic( _InterlockedIncrement )
#pragma intrinsic( _InterlockedDecrement )
#pragma intrinsic( _InterlockedExchange64 )
#pragma intrinsic( _InterlockedExchangeAdd64 )
#pragma intrinsic( _InterlockedCompareExchange64 )
#pragma intrinsic( _InterlockedIncrement64 )
#pragma intrinsic( _InterlockedDecrement64 )
#pragma intrinsic( _InterlockedOr64 )
#pragma intrinsic( _InterlockedOr )
#pragma intrinsic( _InterlockedExchangePointer )

template <typename T>
inline T
atomic_load( T* ptr )
{
    return T( _InterlockedOr64( (volatile int64_t*) ptr, 0 ) );
}

static inline void
atomic_store( void* ptr, uintptr_t val )
{
    _InterlockedExchangePointer( (void* volatile*) ptr, (void*) val );
}
#define __sync_fetch_and_or( a, b )                                                                                              \
    sizeof( *( a ) ) == sizeof( int64_t ) ? InterlockedOr64( (volatile int64_t*) ( a ), int64_t( b ) ) :                         \
                                            InterlockedOr( (volatile long*) ( a ), long( b ) )

#define __sync_fetch_and_add( a, b )                                                                                             \
    sizeof( *( a ) ) == sizeof( int64_t ) ? InterlockedExchangeAdd64( (volatile int64_t*) ( a ), int64_t( b ) ) :                \
                                            InterlockedExchangeAdd( (volatile long*) ( a ), (long) ( b ) )

static inline bool
__sync_bool_compare_and_swap( uint64_t volatile* dst, uint64_t oldl, uint64_t newl )
{
    int64_t original = InterlockedCompareExchange64( (volatile int64_t*) dst, (int64_t) newl, (int64_t) oldl );
    return ( original == (int64_t) oldl );
}

static inline bool
__sync_bool_compare_and_swap( uint32_t volatile* dst, uint32_t oldi, uint32_t newi )
{
    int original = InterlockedCompareExchange( (volatile long*) dst, (long) newi, (long) oldi );
    return ( original == oldi );
}

static inline bool
__sync_bool_compare_and_swap( int volatile* dst, int oldi, int newi )
{
    int original = InterlockedCompareExchange( (volatile long*) dst, (long) newi, (long) oldi );
    return ( original == oldi );
}

#define __sync_lock_test_and_set( a, b )                                                                                         \
    sizeof( *a ) == sizeof( int64_t ) ? InterlockedExchange64( (volatile int64_t*) ( a ), int64_t( b ) ) :                       \
                                        InterlockedExchange( ( a ), ( b ) )

// This built-in function releases the lock acquired by __sync_lock_test_and_set. Normally this means writing the constant 0 to *ptr.
// This built - in function is not a full barrier, but rather a release barrier.
#define __sync_lock_release( a )                                                                                                 \
    sizeof( *a ) == sizeof( int64_t ) ? InterlockedExchange64( (volatile int64_t*) ( a ), 0 ) : InterlockedExchange( ( a ), 0 )

static inline uint32_t
__builtin_ffs( uint64_t v )
{
    unsigned long index;
    return uint32_t( _BitScanForward64( &index, (unsigned long long) v ) ? index + 1 : 0 );
}

#define __builtin_popcount  __popcnt
#define __builtin_popcountl _mm_popcnt_u64

static inline uint32_t
__builtin_ctzl( uint64_t value )
{
    unsigned long index;
    return uint32_t( _BitScanForward64( &index, value ) ? index : 64 );
}

static inline uint32_t
__builtin_clzl( uint64_t value )
{
    unsigned long index;
    return uint32_t( _BitScanReverse64( &index, value ) ? 63 - index : 64 );
}

static inline int
sched_getcpu()
{
    return ::GetCurrentProcessorNumber();
}

static inline LARGE_INTEGER
getFILETIMEoffset()
{
    SYSTEMTIME    s;
    FILETIME      f;
    LARGE_INTEGER t;

    s.wYear         = 1970;
    s.wMonth        = 1;
    s.wDay          = 1;
    s.wHour         = 0;
    s.wMinute       = 0;
    s.wSecond       = 0;
    s.wMilliseconds = 0;
    SystemTimeToFileTime( &s, &f );
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
    return ( t );
}
#define CLOCK_MONOTONIC 0

static inline int
clock_gettime( int X, struct timespec* tv )
{
    LARGE_INTEGER        t;
    FILETIME             f;
    double               microseconds;
    static LARGE_INTEGER offset;
    static double        frequencyToNanoseconds;
    static int           initialized           = 0;
    static BOOL          usePerformanceCounter = 0;

    if( !initialized )
    {
        LARGE_INTEGER performanceFrequency;
        initialized           = 1;
        usePerformanceCounter = QueryPerformanceFrequency( &performanceFrequency );
        if( usePerformanceCounter )
        {
            QueryPerformanceCounter( &offset );
            frequencyToNanoseconds = (double) performanceFrequency.QuadPart / 1000000000.0;
        }
        else
        {
            offset                 = getFILETIMEoffset();
            frequencyToNanoseconds = 10.;
        }
    }
    if( usePerformanceCounter )
        QueryPerformanceCounter( &t );
    else
    {
        GetSystemTimeAsFileTime( &f );
        t.QuadPart = f.dwHighDateTime;
        t.QuadPart <<= 32;
        t.QuadPart |= f.dwLowDateTime;
    }

    t.QuadPart -= offset.QuadPart;
    microseconds = (double) t.QuadPart / frequencyToNanoseconds;
    t.QuadPart   = (long long) microseconds;
    tv->tv_sec   = t.QuadPart / 1000000000;
    tv->tv_nsec  = t.QuadPart % 1000000000;
    return ( 0 );
}

#elif defined( __GNUC__ )

#define LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define ALIGNED( num )   __attribute__( ( aligned( ( num ) ) ) )
#define ATTRIBUTE_THREAD __thread
#define ATTRIBUTE_UNROLL __attribute__( ( optimize( "unroll-loops" ) ) )

#endif

#endif
