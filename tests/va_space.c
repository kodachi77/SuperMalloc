#define _GNU_SOURCE
#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

static size_t
page_size( void )
{
    long ps = sysconf( _SC_PAGESIZE );
    return ps > 0 ? (size_t) ps : 4096;
}

static int
try_map_at( uint64_t addr, size_t len )
{
    void* p = mmap( (void*) addr, len, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS
#ifdef MAP_FIXED_NOREPLACE
                        | MAP_FIXED_NOREPLACE
#endif
                    ,
                    -1, 0 );
    if( p == MAP_FAILED ) return -1;
    // Only succeed if the kernel gave us exactly that address (NOREPLACE path)
    if( p != (void*) addr )
    {
        munmap( p, len );
        return -1;
    }
    munmap( p, len );
    return 0;
}

int
main( void )
{
    size_t ps = page_size();
    printf( "Page size: %zu bytes\n", ps );

    // Search up to an optimistic ceiling (1 << 48) for user space;
    // user VA is often 39/42/47/48 depending on kernel config.
    uint64_t low  = 0x10000;                // skip NULL page
    uint64_t high = ( 1ULL << 48 ) - ps;    // cap at 48-bit for probe
    uint64_t best = 0;

#ifdef MAP_FIXED_NOREPLACE
    // Binary search for highest mappable aligned address.
    while( low <= high )
    {
        uint64_t mid = ( low + high ) / 2;
        mid -= ( mid % ps );    // align
        if( try_map_at( mid, ps ) == 0 )
        {
            best = mid;
            low  = mid + ps;
        }
        else { high = mid - ps; }
    }
#else
    // Fallback: linear downwards probe with hints (slower, but safe).
    for( uint64_t a = high; a > ( 1ULL << 32 ); a -= ( 64ULL << 20 ) )
    {    // step down by 64 MiB
        uint64_t addr = a - ( a % ps );
        if( try_map_at( addr, ps ) == 0 )
        {
            best = addr;
            break;
        }
    }
#endif

    if( best )
    {
        // Estimate user VA bits as ceil(log2(best+1))
        int      bits = 0;
        uint64_t x    = best;
        while( x )
        {
            bits++;
            x >>= 1;
        }
        // if best is exactly (2^n - 1), bits is n; otherwise +1 for ceiling
        // but since best is an address, bits is good enough as ">= n"
        printf( "Estimated user VA max address: 0x%016" PRIx64 "\n", best );
        printf( "Estimated user VA width: %d bits (approx)\n", bits );
    }
    else { printf( "Could not probe user VA high address (permissions or kernel policy)\n" ); }

    // Huge page hints (THP PMD size if available)
    FILE* f = fopen( "/sys/kernel/mm/transparent_hugepage/hpage_pmd_size", "r" );
    if( f )
    {
        unsigned long long thp = 0;
        if( fscanf( f, "%llu", &thp ) == 1 ) { printf( "THP PMD size: %llu bytes\n", thp ); }
        fclose( f );
    }

    // Default hugepage size:
    FILE* fm = fopen( "/proc/meminfo", "r" );
    if( fm )
    {
        char          key[64];
        unsigned long val;
        char          unit[32];
        while( fscanf( fm, "%63s %lu %31s", key, &val, unit ) == 3 )
        {
            if( strcmp( key, "Hugepagesize:" ) == 0 )
            {
                printf( "Hugepagesize: %lu %s\n", val, unit );    // usually kB
                break;
            }
        }
        fclose( fm );
    }

    return 0;
}
