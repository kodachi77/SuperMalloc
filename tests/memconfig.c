// cl /W4 /O2 memconfig.c
#include <stdint.h>
#include <stdio.h>

#if defined( _WIN32 )
#include <malloc.h>
#include <windows.h>
#else
#include <errno.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <unistd.h>
#endif

// ----------------------
// Page size
// ----------------------
static size_t
detect_page_size( void )
{
#if defined( _WIN32 )
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return (size_t) si.dwPageSize;
#else
    return (size_t) sysconf( _SC_PAGESIZE );
#endif
}

// ----------------------
// Cacheline size
// ----------------------
#if defined( __x86_64__ ) || defined( _M_X64 )

#if defined( _MSC_VER )
#include <intrin.h>
#else
#include <cpuid.h>
#endif

static int
detect_cacheline_size( void )
{
    unsigned int eax, ebx, ecx, edx;

#if defined( _MSC_VER )
    int regs[4];
    __cpuid( regs, 1 );
    ebx = (unsigned) regs[1];
#else
    __cpuid( 1, eax, ebx, ecx, edx );
#endif
    int line_size = ( ebx >> 8 ) & 0xff;      // CLFLUSH line size in 8-byte units
    return line_size ? line_size * 8 : 64;    // Convert to bytes, fallback 64
}
#else
static int
detect_cacheline_size( void )
{
#if defined( _SC_LEVEL1_DCACHE_LINESIZE )
    long sz = sysconf( _SC_LEVEL1_DCACHE_LINESIZE );
    if( sz > 0 ) return (int) sz;
#endif
    return 0;    //64;
}
#endif

// ----------------------
// Huge page size
// ----------------------
static size_t
detect_hugepage_size( void )
{
#if defined( _WIN32 )
    // Windows large page minimum size
    SIZE_T sz = GetLargePageMinimum();
    return sz ? (size_t) sz : 0 /*2 * 1024 * 1024*/;    // fallback 2 MiB
#else
#ifdef _SC_HUGE_PAGESIZE
    long sz = sysconf( _SC_HUGE_PAGESIZE );
    if( sz > 0 ) return (size_t) sz;
#endif
    FILE* f = fopen( "/sys/kernel/mm/transparent_hugepage/hpage_pmd_size", "r" );
    if( f )
    {
        unsigned long long thp = 0;
        if( fscanf( f, "%llu", &thp ) == 1 )
        {
            //printf("THP PMD size: %llu bytes\n", thp);
        }
        fclose( f );
        return thp;
    }

    // fallback typical 2 MiB
    return 0;    //2 * 1024 * 1024;
#endif
}

// ----------------------
// Virtual address width
// ----------------------
static int
detect_virtual_address_bits( void )
{
#if defined( __x86_64__ ) || defined( _M_X64 )
    unsigned int eax, ebx, ecx, edx;
#if defined( _MSC_VER )
    int regs[4];
    __cpuid( regs, 0x80000008 );
    eax = (unsigned) regs[0];
#else
    __cpuid( 0x80000008, eax, ebx, ecx, edx );
#endif
    //int phys_bits = eax & 0xff;
    int virt_bits = ( eax >> 8 ) & 0xff;
    return virt_bits;    // typically 48, sometimes 52 on new CPUs
#elif defined( __aarch64__ )
    // On ARM64, VA bits are given by kernel config, often 48
    return 48;    // safe default
#else
    return 0;    //64;
#endif
}

// ----------------------
// Main
// ----------------------
int
main( void )
{
    size_t page_size = detect_page_size();
    int    cacheline = detect_cacheline_size();
    size_t hugepage  = detect_hugepage_size();
    int    va_bits   = detect_virtual_address_bits();

    printf( "Regular page size : %zu bytes\n", page_size );
    printf( "Cache line size   : %d bytes\n", cacheline );
    printf( "Huge page size    : %zu bytes\n", hugepage );
    printf( "Virtual address   : %d bits\n", va_bits );

    return 0;
}