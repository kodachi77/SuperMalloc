#include "sm_platform.h"

#if defined( __linux__ )

#if defined( HAVE_CPUCORES_SYSCONF )
#include <unistd.h>

#elif defined( HAVE_CPUCORES_SYSCTL )
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#endif

#elif defined( _WIN64 )

#include "sm_config.h"

#endif

#ifdef __cplusplus
extern "C"
{
#endif

    unsigned long cpucores( void )
    {
#if defined( __linux__ )
        unsigned long ret;
#if defined( HAVE_CPUCORES_SYSCONF )
        const long cpus = sysconf( _SC_NPROCESSORS_ONLN );
        if( cpus > 0 ) ret = static_cast<uint32_t>( cpus );

#elif defined( HAVE_CPUCORES_SYSCTL )
        int    name[2] = { CTL_HW, HW_NCPU };
        int    cpus;
        size_t cpus_size = sizeof( cpus );
        if( !sysctl( name, &cpus, &cpus_size, NULL, NULL ) && cpus_size == sizeof( cpus ) && cpus > 0 )
            ret = static_cast<uint32_t>( cpus );
#endif

        return ret;
#elif defined( _WIN64 )
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return si.dwNumberOfProcessors;
#endif
    }

    // This is copied out of Dave Dice's web log
    //  https://blogs.oracle.com/dave/entry/a_simple_prng_idiom

    static const uint64_t MIX = 0xdaba0b6eb09322e3ull;

    static uint64_t Mix64( uint64_t z )
    {
        z = ( z ^ ( z >> 32 ) ) * MIX;
        z = ( z ^ ( z >> 32 ) ) * MIX;
        return z ^ ( z >> 32 );
    }

    uint64_t prandnum()
    {
        // Instead of incrementing by 1 we could also increment by a large
        // prime or by the MIX constant.

        // One clever idea from Dave Dice: We use the address of the
        // __thread variable as part of the hashed input, so that our random
        // number generator doesn't need to be initialized.

        // Make this aligned(64) so that we don't get any false sharing.
        // Perhaps __thread variables already avoid false sharing, but I
        // don't want to verify that.
        static SM_ALIGNED( 64 ) uint64_t SM_ATTRIBUTE_THREAD rv = 0;
        return Mix64( (int64_t) ( &rv ) + ( ++rv ) );
    }

    int sched_yield()
    {
        Sleep( 0 );
        return 0;
    }

    unsigned int sleep( unsigned int seconds )
    {
        Sleep( seconds * 1000 );
        return 0;
    }

    int madvise( void* addr, size_t len, int advice )
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

    static inline LARGE_INTEGER getFILETIMEoffset()
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

    /* static inline*/ int clock_gettime( int X, struct timespec* tv )
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
    int sched_getcpu() { return GetCurrentProcessorNumber(); }    //> TODO: this API has limitation of 64 processors
#ifdef __cplusplus
}
#endif
