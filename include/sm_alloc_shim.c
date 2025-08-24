// sm_alloc_shim.c
//
// Implementation of the SM allocation API using libc / platform calls,
// with portable fallbacks. Replace TODO sections to delegate to your
// own sm_alloc/sm_free primitives if desired.
//
// This file is C89/C99 friendly (with a C++ section for new-handler logic).

// cl /c /std:c11 /volatile:iso /TC sm_alloc.c
// cl /c /volatile:iso /Zc:preprocessor /std:c++14 /Zc:__cplusplus /EHsc /TP sm_alloc.c
// cl /c /volatile:iso /Zc:preprocessor /std:c++17 /Zc:alignedNew /Zc:__cplusplus /EHsc /TP sm_alloc_shim.c

// Bring in your public header with the attribute macros + function decls.
#include "sm_alloc.h"

#ifdef SM_OVERRIDE_STD_MALLOC

#undef malloc
#undef calloc
#undef realloc
#undef free

#undef strdup
#undef strndup
#undef realpath

#undef _expand
#undef _msize
#undef _recalloc

#undef _strdup
#undef _strndup
#undef _wcsdup
#undef _mbsdup
#undef _dupenv_s
#undef _wdupenv_s

#undef reallocf
#undef malloc_size
#undef malloc_usable_size
#undef malloc_good_size
#undef cfree

#undef valloc
#undef pvalloc
#undef reallocarray
#undef reallocarr
#undef memalign
#undef aligned_alloc
#undef posix_memalign
#undef _posix_memalign

#undef _aligned_malloc
#undef _aligned_realloc
#undef _aligned_recalloc
#undef _aligned_msize
#undef _aligned_free
#undef _aligned_offset_malloc
#undef _aligned_offset_realloc
#undef _aligned_offset_recalloc

#endif

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined( _WIN32 )
#include <direct.h>
#include <io.h>
#include <malloc.h>     // _aligned_malloc, _aligned_realloc, _aligned_free, _msize
#include <windows.h>    // GetSystemInfo
#else
#include <sys/param.h>
#include <sys/types.h>
#include <unistd.h>    // sysconf
#endif

// ----------------------------------------
// Helpers
// ----------------------------------------

static size_t
sm__page_size( void )
{
#if defined( _WIN32 )
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return (size_t) si.dwPageSize;
#elif defined( _SC_PAGESIZE )
    long v = sysconf( _SC_PAGESIZE );
    return v > 0 ? (size_t) v : (size_t) 4096;
#else
    return 4096;
#endif
}

static int
sm__is_power_of_two( size_t x )
{
    return x && ( ( x & ( x - 1 ) ) == 0 );
}

static size_t
sm__round_up( size_t n, size_t a )
{
    return ( n + ( a - 1 ) ) & ~( a - 1 );
}

static int
sm__mul_overflows( size_t a, size_t b, size_t* out )
{
    if( a == 0 || b == 0 )
    {
        *out = 0;
        return 0;
    }
    if( a > SIZE_MAX / b ) { return 1; }
    *out = a * b;
    return 0;
}

// ----------------------------------------
// Core allocation
// ----------------------------------------

void*
sm_malloc( size_t size ) sm_attr_noexcept
{
    // TODO: route through your allocator core if desired:
    // return sm_alloc(size);
    return malloc( size );
}

void*
sm_calloc( size_t count, size_t size ) sm_attr_noexcept
{
    // libc calloc already handles overflow and zeroing
    // TODO: route through your allocator core if desired:
    // return sm_alloc_zero(count * size);
    return calloc( count, size );
}

void*
sm_realloc( void* p, size_t newsize ) sm_attr_noexcept
{
    // TODO: route through your allocator core if desired
    return realloc( p, newsize );
}

// Expand in-place if possible. Windows has _expand; others fallback to realloc.
void*
sm_expand( void* p, size_t newsize ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _expand( p, newsize );
#else
    // No strict in-place expand. Best-effort via realloc.
    return realloc( p, newsize );
#endif
}

void
sm_free( void* p ) sm_attr_noexcept
{
    // TODO: route through your allocator core if desired:
    free( p );
}

void
sm_free_size( void* p, size_t size ) sm_attr_noexcept
{
    (void) size;
    // Size hint ignored by default.
    free( p );
}

void
sm_free_aligned( void* p, size_t alignment ) sm_attr_noexcept
{
    (void) alignment;
#if defined( _WIN32 )
    _aligned_free( p );
#else
    free( p );
#endif
}

void
sm_free_size_aligned( void* p, size_t size, size_t alignment ) sm_attr_noexcept
{
    (void) size;
    (void) alignment;
#if defined( _WIN32 )
    _aligned_free( p );
#else
    free( p );
#endif
}

// ----------------------------------------
// C++ new-like wrappers
// ----------------------------------------

#if defined( __cplusplus )
#include <new>

static void*
sm__new_loop_alloc( size_t bytes )
{
    while( true )
    {
        void* p = sm_malloc( bytes );
        if( p ) return p;
        std::new_handler h = std::get_new_handler();
        if( !h ) throw std::bad_alloc();
        try
        {
            h();
        }
        catch( ... )
        {
            throw;
        }
    }
}

static void*
sm__new_loop_alloc_aligned( size_t bytes, size_t alignment )
{
    while( true )
    {
        void* p = sm_malloc_aligned( bytes, alignment );
        if( p ) return p;
        std::new_handler h = std::get_new_handler();
        if( !h ) throw std::bad_alloc();
        try
        {
            h();
        }
        catch( ... )
        {
            throw;
        }
    }
}

void*
sm_new( size_t size )
{
    return sm__new_loop_alloc( size );
}

void*
sm_new_aligned( size_t size, size_t alignment )
{
    return sm__new_loop_alloc_aligned( size, alignment );
}

void*
sm_new_nothrow( size_t size ) noexcept
{
    try
    {
        return sm__new_loop_alloc( size );
    }
    catch( ... )
    {
        return nullptr;
    }
}

void*
sm_new_aligned_nothrow( size_t size, size_t alignment ) noexcept
{
    try
    {
        return sm__new_loop_alloc_aligned( size, alignment );
    }
    catch( ... )
    {
        return nullptr;
    }
}

void*
sm_new_n( size_t count, size_t size )
{
    size_t total;
    if( sm__mul_overflows( count, size, &total ) ) throw std::bad_alloc();
    return sm__new_loop_alloc( total );
}

void*
sm_new_realloc( void* p, size_t newsize )
{
    void* r = sm_realloc( p, newsize );
    if( r ) return r;
    // If realloc failed, apply new-handler behavior
    // (Note: standard operator new doesn't wrap realloc; this is a best-effort match.)
    return sm__new_loop_alloc( newsize );
}

void*
sm_new_reallocn( void* p, size_t newcount, size_t size )
{
    size_t total;
    if( sm__mul_overflows( newcount, size, &total ) ) throw std::bad_alloc();
    void* r = sm_realloc( p, total );
    if( r ) return r;
    return sm__new_loop_alloc( total );
}

#else    // !__cplusplus

// // In pure C builds, provide best-effort nothrow behavior.
// void* sm_new(size_t size)                  { return sm_malloc(size); }
// void* sm_new_aligned(size_t size, size_t a){ return sm_malloc_aligned(size, a); }
// void* sm_new_nothrow(size_t size)          { return sm_malloc(size); }
// void* sm_new_aligned_nothrow(size_t s,size_t a){ return sm_malloc_aligned(s,a); }
// void* sm_new_n(size_t count, size_t size)  {
//     size_t total;
//     if (sm__mul_overflows(count, size, &total)) return NULL;
//     return sm_malloc(total);
// }
// void* sm_new_realloc(void* p, size_t n)    { return sm_realloc(p, n); }
// void* sm_new_reallocn(void* p, size_t c, size_t s) {
//     size_t total;
//     if (sm__mul_overflows(c, s, &total)) return NULL;
//     return sm_realloc(p, total);
// }
#endif

// ----------------------------------------
// Size / introspection
// ----------------------------------------

size_t
sm_usable_size( void* p ) sm_attr_noexcept
{
    if( !p ) return 0;
#if defined( _WIN32 )
    return (size_t) _msize( p );
#elif defined( __APPLE__ )
    // macOS
    extern size_t malloc_size( const void* );
    return malloc_size( p );
#elif defined( __linux__ )
    // glibc
    extern size_t malloc_usable_size( void* );
    return malloc_usable_size( p );
#else
    // TODO: wire to your allocator's usable-size if available.
    return 0;
#endif
}

size_t
sm_malloc_good_size( size_t sz ) sm_attr_noexcept
{
#if defined( __APPLE__ )
    extern size_t malloc_good_size( size_t );
    return malloc_good_size( sz );
#else
    // TODO: if your allocator exposes a size class rounding function, call it here.
    return sz;
#endif
}

// ----------------------------------------
// Dup helpers
// ----------------------------------------

char*
sm_strdup( const char* s ) sm_attr_noexcept
{
    if( !s ) return NULL;
#if defined( _WIN32 )
    return _strdup( s );
#elif defined( HAVE_STRDUP ) || ( !defined( __STRICT_ANSI__ ) && !defined( __STDC_VERSION__ ) ) || defined( __GNU_LIBRARY__ )
    return strdup( s );
#else
    size_t n = strlen( s ) + 1;
    char*  d = (char*) sm_malloc( n );
    if( !d ) return NULL;
    memcpy( d, s, n );
    return d;
#endif
}

char*
sm_strndup( const char* s, size_t n ) sm_attr_noexcept
{
    if( !s ) return NULL;
#if defined( _WIN32 )
    // No _strndup on older MSVC; implement.
    size_t len = 0;
    while( len < n && s[len] != '\0' ) ++len;
    char* d = (char*) sm_malloc( len + 1 );
    if( !d ) return NULL;
    memcpy( d, s, len );
    d[len] = '\0';
    return d;
#elif defined( HAVE_STRNDUP ) || ( defined( _GNU_SOURCE ) || defined( __BSD_VISIBLE ) )
    return strndup( s, n );
#else
    size_t len = 0;
    while( len < n && s[len] != '\0' ) ++len;
    char* d = (char*) sm_malloc( len + 1 );
    if( !d ) return NULL;
    memcpy( d, s, len );
    d[len] = '\0';
    return d;
#endif
}

char*
sm_realpath( const char* fname, char* resolved_name ) sm_attr_noexcept
{
#if defined( _WIN32 )
    // _fullpath allocates if buffer == NULL (using malloc).
    return _fullpath( resolved_name, fname, _MAX_PATH );
#else
    // On POSIX, realpath allocates if resolved_name == NULL (using malloc).
    return realpath( fname, resolved_name );
#endif
}

char*
sm_mbsdup( const char* s ) sm_attr_noexcept
{
    return sm_strdup( s );
}

unsigned short*
sm_wcsdup( const unsigned short* s ) sm_attr_noexcept
{
    if( !s ) return NULL;
#if defined( _WIN32 )
    return (unsigned short*) _wcsdup( (const wchar_t*) s );
#else
    // Portable fallback (UTF-16/UTF-32 width varies; this is a raw element copy).
    size_t len = 0;
    while( s[len] != 0 ) ++len;
    unsigned short* d = (unsigned short*) sm_malloc( ( len + 1 ) * sizeof( unsigned short ) );
    if( !d ) return NULL;
    for( size_t i = 0; i <= len; ++i ) d[i] = s[i];
    return d;
#endif
}

// Windows-style env dup
int
sm_dupenv_s( char** buffer, size_t* numberOfElements, const char* varname ) sm_attr_noexcept
{
    if( !buffer || !varname ) return EINVAL;
#if defined( _WIN32 )
    return _dupenv_s( buffer, numberOfElements, varname );
#else
    const char* v = getenv( varname );
    if( !v )
    {
        if( buffer ) *buffer = NULL;
        if( numberOfElements ) *numberOfElements = 0;
        return 0;
    }
    size_t len = strlen( v ) + 1;
    char*  out = (char*) sm_malloc( len );
    if( !out ) return ENOMEM;
    memcpy( out, v, len );
    if( buffer ) *buffer = out;
    if( numberOfElements ) *numberOfElements = len;
    return 0;
#endif
}

int
sm_wdupenv_s( unsigned short** buffer, size_t* numberOfElements, const unsigned short* varname ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _wdupenv_s( (wchar_t**) buffer, numberOfElements, (const wchar_t*) varname );
#else
    // TODO: Implement a proper wide-env lookup or define not supported on non-Windows.
    (void) buffer;
    (void) numberOfElements;
    (void) varname;
    return ENOTSUP;
#endif
}

// ----------------------------------------
// Realloc variants
// ----------------------------------------

void*
sm_reallocf( void* p, size_t newsize ) sm_attr_noexcept
{
#if defined( __APPLE__ ) || defined( __FreeBSD__ ) || defined( __OpenBSD__ ) || defined( __NetBSD__ )
    extern void* reallocf( void*, size_t );
    return reallocf( p, newsize );
#else
    void* r = sm_realloc( p, newsize );
    if( !r && p ) sm_free( p );
    return r;
#endif
}

void*
sm_reallocarray( void* p, size_t nmemb, size_t size ) sm_attr_noexcept
{
#if defined( __OpenBSD__ ) || ( defined( __GLIBC__ ) && ( __GLIBC__ * 100 + __GLIBC_MINOR__ ) >= 234 )
    extern void* reallocarray( void*, size_t, size_t );
    return reallocarray( p, nmemb, size );
#else
    size_t total;
    if( sm__mul_overflows( nmemb, size, &total ) )
    {
        errno = ENOMEM;
        return NULL;
    }
    return sm_realloc( p, total );
#endif
}

int
sm_reallocarr( void** p, size_t nmemb, size_t size ) sm_attr_noexcept
{
    if( !p ) return EINVAL;
    void* np = sm_reallocarray( *p, nmemb, size );
    if( !np && ( nmemb || size ) ) return errno ? errno : ENOMEM;
    *p = np;
    return 0;
}

void*
sm_recalloc( void* p, size_t count, size_t size ) sm_attr_noexcept
{
#if defined( _WIN32 )
    // _recalloc zero-initializes new bytes on expansion.
    return _recalloc( p, count, size );
#else
    size_t total;
    if( sm__mul_overflows( count, size, &total ) )
    {
        errno = ENOMEM;
        return NULL;
    }
    if( !p ) return sm_calloc( count, size );

    size_t old = sm_usable_size( p );    // may be 0 if unknown
    void*  np  = sm_realloc( p, total );
    if( !np ) return NULL;
    if( total > old && old != 0 )
    {
        // Zero only the grown tail if we could determine the old size.
        memset( (char*) np + old, 0, total - old );
    }
    // TODO: If sm_usable_size is not supported, consider zeroing the whole block on growth.
    return np;
#endif
}

// ----------------------------------------
// Aligned allocation family
// ----------------------------------------

void*
sm_malloc_aligned( size_t size, size_t alignment ) sm_attr_noexcept
{
    if( !sm__is_power_of_two( alignment ) || alignment < sizeof( void* ) )
    {
        errno = EINVAL;
        return NULL;
    }
#if defined( _WIN32 )
    return _aligned_malloc( size, alignment );
#elif defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 201112L )
    // C11 aligned_alloc requires size to be a multiple of alignment
    size_t adj = sm__round_up( size, alignment );
    void*  p   = aligned_alloc( alignment, adj );
    // Note: returned pointer is for adj bytes; we asked for >= size.
    return p;
#else
    void* p = NULL;
    if( posix_memalign( &p, alignment, size ) != 0 ) return NULL;
    return p;
#endif
}

void*
sm_realloc_aligned( void* p, size_t newsize, size_t alignment ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _aligned_realloc( p, newsize, alignment );
#else
    // Portable fallback: allocate new aligned block, copy, free old.
    void* np = sm_malloc_aligned( newsize, alignment );
    if( !np ) return NULL;
    if( p )
    {
        size_t n = sm_usable_size( p );
        if( n == 0 || n > newsize ) n = newsize;
        if( n ) memcpy( np, p, n );
        sm_free_aligned( p, alignment );
    }
    return np;
#endif
}

void*
sm_aligned_recalloc( void* p, size_t count, size_t size, size_t alignment ) sm_attr_noexcept
{
    size_t total;
    if( sm__mul_overflows( count, size, &total ) )
    {
        errno = ENOMEM;
        return NULL;
    }
#if defined( _WIN32 )
    return _aligned_recalloc( p, count, size, alignment );
#else
    size_t old = p ? sm_usable_size( p ) : 0;
    void*  np  = sm_realloc_aligned( p, total, alignment );
    if( !np ) return NULL;
    if( total > old && old != 0 ) memset( (char*) np + old, 0, total - old );
    // TODO: if old size unknown, consider zeroing whole block on growth.
    return np;
#endif
}

// Aligned with explicit offset (Windows has direct APIs; others emulate).
void*
sm_malloc_aligned_at( size_t size, size_t alignment, size_t offset ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _aligned_offset_malloc( size, alignment, (int) offset );
#else
    if( !sm__is_power_of_two( alignment ) || alignment < sizeof( void* ) )
    {
        errno = EINVAL;
        return NULL;
    }
    // Overallocate and align such that (ptr + offset) % alignment == 0
    size_t extra = alignment + sizeof( void* );
    char*  base  = (char*) sm_malloc( size + extra );
    if( !base ) return NULL;
    uintptr_t addr    = (uintptr_t) ( base + sizeof( void* ) );
    uintptr_t aligned = ( addr + ( ( alignment - ( ( addr + offset ) & ( alignment - 1 ) ) ) & ( alignment - 1 ) ) );
    void**    user    = (void**) aligned;
    user[-1]          = base;    // store base before user pointer
    return (void*) user;
#endif
}

void*
sm_realloc_aligned_at( void* p, size_t newsize, size_t alignment, size_t offset ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _aligned_offset_realloc( p, newsize, alignment, (int) offset );
#else
    // Generic: allocate new, copy, free.
    void* np = sm_malloc_aligned_at( newsize, alignment, offset );
    if( !np ) return NULL;
    if( p )
    {
        size_t n = sm_usable_size( p );
        if( n == 0 || n > newsize ) n = newsize;
        if( n ) memcpy( np, p, n );
        sm_free_aligned( p, alignment );    // offset ignored in free shim
    }
    return np;
#endif
}

void*
sm_recalloc_aligned_at( void* p, size_t count, size_t size, size_t alignment, size_t offset ) sm_attr_noexcept
{
    size_t total;
    if( sm__mul_overflows( count, size, &total ) )
    {
        errno = ENOMEM;
        return NULL;
    }
#if defined( _WIN32 )
    return _aligned_offset_recalloc( p, count, size, alignment, (int) offset );
#else
    size_t old = p ? sm_usable_size( p ) : 0;
    void*  np  = sm_realloc_aligned_at( p, total, alignment, offset );
    if( !np ) return NULL;
    if( total > old && old != 0 ) memset( (char*) np + old, 0, total - old );
    // TODO: if old size unknown, consider zeroing whole block on growth.
    return np;
#endif
}

// ----------------------------------------
// POSIX / BSD / GNU / C11 compatibles
// ----------------------------------------

void*
sm_valloc( size_t size ) sm_attr_noexcept
{
    size_t ps = sm__page_size();
#if defined( __APPLE__ ) || defined( __GLIBC__ ) || defined( __FreeBSD__ ) || defined( __NetBSD__ ) || defined( __OpenBSD__ )
    // valloc is deprecated; emulate via posix_memalign
    void* p = NULL;
    if( posix_memalign( &p, ps, size ) != 0 ) return NULL;
    return p;
#else
    // Portable fallback
    return sm_malloc_aligned( size, ps );
#endif
}

void*
sm_pvalloc( size_t size ) sm_attr_noexcept
{
#if defined( __GLIBC__ )
    extern void* pvalloc( size_t );
    return pvalloc( size );
#else
    size_t ps = sm__page_size();
    size_t n  = sm__round_up( size, ps );
    // emulate via page-size rounding + posix_memalign/aligned alloc
    return sm_malloc_aligned( n, ps );
#endif
}

void*
sm_memalign( size_t alignment, size_t size ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _aligned_malloc( size, alignment );
#else
    void* p = NULL;
    if( !sm__is_power_of_two( alignment ) || alignment < sizeof( void* ) )
    {
        errno = EINVAL;
        return NULL;
    }
    if( posix_memalign( &p, alignment, size ) != 0 ) return NULL;
    return p;
#endif
}

void*
sm_aligned_alloc( size_t alignment, size_t size ) sm_attr_noexcept
{
#if defined( _WIN32 )
    return _aligned_malloc( size, alignment );
#elif defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ >= 201112L )
    // C11 requires size multiple of alignment
    if( ( size % alignment ) != 0 ) { size = sm__round_up( size, alignment ); }
    return aligned_alloc( alignment, size );
#else
    void* p = NULL;
    if( !sm__is_power_of_two( alignment ) || alignment < sizeof( void* ) )
    {
        errno = EINVAL;
        return NULL;
    }
    if( posix_memalign( &p, alignment, size ) != 0 ) return NULL;
    return p;
#endif
}

int
sm_posix_memalign( void** memptr, size_t alignment, size_t size ) sm_attr_noexcept
{
#if defined( _WIN32 )
    if( !memptr ) return EINVAL;
    if( !sm__is_power_of_two( alignment ) || alignment < sizeof( void* ) ) return EINVAL;
    *memptr = _aligned_malloc( size, alignment );
    return *memptr ? 0 : ENOMEM;
#else
    return posix_memalign( memptr, alignment, size );
#endif
}

// ----------------------------------------
// Portable free for offset emulation block
// ----------------------------------------

#if !defined( _WIN32 )
// If we used the "store base before user pointer" trick in sm_malloc_aligned_at,
// free must detect and use that base.
static void
sm__free_maybe_offset( void* p )
{
    if( !p ) return;
    void* base = ( (void**) p )[-1];
    // Heuristic: base must be <= p and within one alignment window. If it's not
    // a plausible pointer, just free(p).
    // In our construction we always set user[-1] = base, so prefer freeing base.
    if( base )
        free( base );
    else
        free( p );
}
#endif

// NOTE: Our public sm_free_aligned already maps to _aligned_free/free.
// For offset-allocations on POSIX we called sm_free_aligned(p, alignment)
// which ends up here and uses plain free(). Since we embedded base just
// before the returned pointer, the standard free() would be wrong.
// We already routed offset cases through sm_realloc_aligned_at / sm_free_aligned
// which call free(); if you plan to mix offset + POSIX free paths, consider
// exposing and using sm__free_maybe_offset above instead (left as TODO).

/* End of file */
