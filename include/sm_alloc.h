#ifndef SM_ALLOC_H_
#define SM_ALLOC_H_

#if defined( _DEBUG ) || defined( SM_TEST )
#define SM_ASSERTS_ENABLED 1
#endif

#if defined( _M_X64 ) || defined( _M_AMD64 ) || defined( __x86_64__ ) || defined( __amd64__ )
#define SM_ARCH_X64 1
#elif defined( _M_ARM64 ) || defined( __arm64__ ) || defined( __arm64 ) || defined( __aarch64__ )
#define SM_ARCH_ARM64 1
#else
#error "SM_ALLOC: Unsupported architecture, only x86_64 and ARM64 are supported."
#endif

#if defined( _WIN32 ) || defined( _WIN64 )
#define SM_PLATFORM_WINDOWS 1
#elif defined( __linux__ )
#define SM_PLATFORM_LINUX 1
#else
#error "SM_ALLOC: Unsupported platform, only Windows and Linux are supported."
#endif

#if defined( _MSC_VER )

#if ( _MSC_VER < 1938 )
#error "SM_ALLOC: Unsupported toolchain, version 14.38 (Visual Studio 2022 17.8+) is required."
#endif

#if !defined( _WIN64 )
#error "SM_ALLOC: Unsupported platform, only 64-bit Windows is supported."
#endif

#if !defined( _ISO_VOLATILE ) || !_ISO_VOLATILE
#error "SM_ALLOC: Set /volatile:iso option to enable C11 standard compliance."
#endif

#if !defined( _MSVC_TRADITIONAL ) || _MSVC_TRADITIONAL
#error "SM_ALLOC: Set /Zc:preprocessor to enable C11 standard compliance."
#endif

#endif

#if defined( __GNUC__ ) && ( __GNUC__ < 4 ) && ( __GNUC_MINOR__ < 9 )
#error "SM_ALLOC: Unsupported toolchain, GCC 4.9+ is required."
#endif

#if defined( __clang__ ) && ( __clang_major__ < 3 ) && ( __clang_minor__ < 6 )
#error "SM_ALLOC: Unsupported toolchain, Clang 3.6+ is required."
#endif

#if defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ < 201112L )
#error "SM_ALLOC: Set /std:c11 to enable C11 standard."
#endif

#if defined( __cplusplus ) && ( __cplusplus < 201103L )
#error "SM_ALLOC: Set /std:c++11 to enable C++11 standard."
#endif

#if !defined( _MSC_VER ) && !defined( __STDC_NO_ATOMICS__ )
#error "SM_ALLOC: C11 <stdatomic.h> not available"
#endif

#if !defined( _MSC_VER ) && !defined( __STDC_NO_THREADS__ )
#error "SM_ALLOC: C11 <threads.h> not available"
#endif

#if defined( __cplusplus )
#define sm_attr_noexcept noexcept
#else
#define sm_attr_noexcept
#endif

#if defined( __cplusplus ) && ( __cplusplus >= 201703L )
#define sm_decl_nodiscard [[nodiscard]]
#elif defined( __GNUC__ ) || defined( __clang__ )    // includes clang, icc, and clang-cl
#define sm_decl_nodiscard __attribute__( ( warn_unused_result ) )
#elif defined( _MSC_VER )
#define sm_decl_nodiscard _Check_return_
#else
#define sm_decl_nodiscard
#endif

#if defined( _MSC_VER )

#if !defined( SM_SHARED_LIB )
#define sm_decl_export
#elif defined( SM_SHARED_LIB_EXPORT )
#define sm_decl_export __declspec( dllexport )
#else
#define sm_decl_export __declspec( dllimport )
#endif

#define sm_decl_restrict __declspec( allocator ) __declspec( restrict )
#define sm_attr_malloc
#define sm_cdecl __cdecl
#define sm_attr_alloc_size( s )
#define sm_attr_alloc_size2( s1, s2 )
#define sm_attr_alloc_align( p )

#elif defined( __GNUC__ ) || defined( __clang__ )    // includes clang and icc

#if defined( SM_SHARED_LIB ) && defined( SM_SHARED_LIB_EXPORT )
#define sm_decl_export __attribute__( ( visibility( "default" ) ) )
#else
#define sm_decl_export
#endif

#define sm_cdecl
#define sm_decl_restrict
#define sm_attr_malloc __attribute__( ( malloc ) )

#if ( defined( __clang_major__ ) && ( __clang_major__ < 4 ) ) || ( __GNUC__ < 5 )
#define sm_attr_alloc_size( s )
#define sm_attr_alloc_size2( s1, s2 )
#define sm_attr_alloc_align( p )
#else
#define sm_attr_alloc_size( s )       __attribute__( ( alloc_size( s ) ) )
#define sm_attr_alloc_size2( s1, s2 ) __attribute__( ( alloc_size( s1, s2 ) ) )
#define sm_attr_alloc_align( p )      __attribute__( ( alloc_align( p ) ) )
#endif

#else
#define sm_cdecl
#define sm_decl_export
#define sm_decl_restrict
#define sm_attr_malloc
#define sm_attr_alloc_size( s )
#define sm_attr_alloc_size2( s1, s2 )
#define sm_attr_alloc_align( p )
#endif

#include <stdbool.h>    // bool
#include <stddef.h>     // size_t
//#include <stdint.h>     // INTPTR_MAX

#ifdef __cplusplus
extern "C"
{
#endif

sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_malloc( size_t size ) sm_attr_noexcept sm_attr_malloc
    sm_attr_alloc_size( 1 );
sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_calloc( size_t count, size_t size ) sm_attr_noexcept sm_attr_malloc
    sm_attr_alloc_size2( 1, 2 );
sm_decl_nodiscard sm_decl_export void* sm_realloc( void* p, size_t newsize ) sm_attr_noexcept sm_attr_alloc_size( 2 );
sm_decl_export void*                   sm_expand( void* p, size_t newsize ) sm_attr_noexcept sm_attr_alloc_size( 2 );

sm_decl_export void sm_free( void* p ) sm_attr_noexcept;
sm_decl_export void sm_free_size( void* p, size_t size ) sm_attr_noexcept;
sm_decl_export void sm_free_aligned( void* p, size_t alignment ) sm_attr_noexcept;
sm_decl_export void sm_free_size_aligned( void* p, size_t size, size_t alignment ) sm_attr_noexcept;

sm_decl_nodiscard sm_decl_export sm_decl_restrict char* sm_strdup( const char* s ) sm_attr_noexcept sm_attr_malloc;
sm_decl_nodiscard sm_decl_export sm_decl_restrict char* sm_strndup( const char* s, size_t n ) sm_attr_noexcept sm_attr_malloc;
sm_decl_nodiscard sm_decl_export sm_decl_restrict char* sm_realpath( const char* fname,
                                                                     char*       resolved_name ) sm_attr_noexcept sm_attr_malloc;

// The `sm_new` wrappers implement C++ semantics on out-of-memory instead of directly returning `NULL`.
// (and call `std::get_new_handler` and potentially raise a `std::bad_alloc` exception).
sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_new( size_t size ) sm_attr_malloc sm_attr_alloc_size( 1 );
sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_new_aligned( size_t size, size_t alignment ) sm_attr_malloc
    sm_attr_alloc_size( 1 ) sm_attr_alloc_align( 2 );
sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_new_nothrow( size_t size ) sm_attr_noexcept sm_attr_malloc
    sm_attr_alloc_size( 1 );
sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_new_aligned_nothrow( size_t size,
                                                                                size_t alignment ) sm_attr_noexcept sm_attr_malloc
    sm_attr_alloc_size( 1 ) sm_attr_alloc_align( 2 );
sm_decl_nodiscard sm_decl_export sm_decl_restrict void* sm_new_n( size_t count, size_t size ) sm_attr_malloc
    sm_attr_alloc_size2( 1, 2 );
sm_decl_nodiscard sm_decl_export void* sm_new_realloc( void* p, size_t newsize ) sm_attr_alloc_size( 2 );
sm_decl_nodiscard sm_decl_export void* sm_new_reallocn( void* p, size_t newcount, size_t size ) sm_attr_alloc_size2( 2, 3 );

#ifdef SM_OVERRIDE_STD_MALLOC

// Standard C allocation
#define malloc( n )     sm_malloc( n )
#define calloc( n, c )  sm_calloc( n, c )
#define realloc( p, n ) sm_realloc( p, n )
#define free( p )       sm_free( p )

#define strdup( s )      sm_strdup( s )
#define strndup( s, n )  sm_strndup( s, n )
#define realpath( f, n ) sm_realpath( f, n )

// Microsoft extensions
#define _expand( p, n )      sm_expand( p, n )
#define _msize( p )          sm_usable_size( p )
#define _recalloc( p, n, c ) sm_recalloc( p, n, c )

#define _strdup( s )          sm_strdup( s )
#define _strndup( s, n )      sm_strndup( s, n )
#define _wcsdup( s )          (wchar_t*) sm_wcsdup( (const unsigned short*) ( s ) )
#define _mbsdup( s )          sm_mbsdup( s )
#define _dupenv_s( b, n, v )  sm_dupenv_s( b, n, v )
#define _wdupenv_s( b, n, v ) sm_wdupenv_s( (unsigned short*) ( b ), n, (const unsigned short*) ( v ) )

// Various Posix and Unix variants
#define reallocf( p, n )        sm_reallocf( p, n )
#define malloc_size( p )        sm_usable_size( p )
#define malloc_usable_size( p ) sm_usable_size( p )
#define malloc_good_size( sz )  sm_malloc_good_size( sz )
#define cfree( p )              sm_free( p )

#define valloc( n )                sm_valloc( n )
#define pvalloc( n )               sm_pvalloc( n )
#define reallocarray( p, s, n )    sm_reallocarray( p, s, n )
#define reallocarr( p, s, n )      sm_reallocarr( p, s, n )
#define memalign( a, n )           sm_memalign( a, n )
#define aligned_alloc( a, n )      sm_aligned_alloc( a, n )
#define posix_memalign( p, a, n )  sm_posix_memalign( p, a, n )
#define _posix_memalign( p, a, n ) sm_posix_memalign( p, a, n )

// Microsoft aligned variants
#define _aligned_malloc( n, a )                   sm_malloc_aligned( n, a )
#define _aligned_realloc( p, n, a )               sm_realloc_aligned( p, n, a )
#define _aligned_recalloc( p, s, n, a )           sm_aligned_recalloc( p, s, n, a )
#define _aligned_msize( p, a, o )                 sm_usable_size( p )
#define _aligned_free( p )                        sm_free( p )
#define _aligned_offset_malloc( n, a, o )         sm_malloc_aligned_at( n, a, o )
#define _aligned_offset_realloc( p, n, a, o )     sm_realloc_aligned_at( p, n, a, o )
#define _aligned_offset_recalloc( p, s, n, a, o ) sm_recalloc_aligned_at( p, s, n, a, o )

#endif

#ifdef __cplusplus
} /* extern "C" */

#include <new>

#if defined( _MSC_VER ) && defined( _Ret_notnull_ ) && defined( _Post_writable_byte_size_ )
// stay consistent with VCRT definitions
#define sm_decl_new( n ) sm_decl_nodiscard sm_decl_restrict _Ret_notnull_ _Post_writable_byte_size_( n )
#define sm_decl_new_nothrow( n )                                                                                                 \
    sm_decl_nodiscard sm_decl_restrict _Ret_maybenull_ _Success_( return != NULL ) _Post_writable_byte_size_( n )
#else
#define sm_decl_new( n )         sm_decl_nodiscard sm_decl_restrict
#define sm_decl_new_nothrow( n ) sm_decl_nodiscard sm_decl_restrict
#endif

void
operator delete( void* p ) noexcept
{
    sm_free( p );
};
void
operator delete[]( void* p ) noexcept
{
    sm_free( p );
};

void
operator delete( void* p, const std::nothrow_t& ) noexcept
{
    sm_free( p );
}
void
operator delete[]( void* p, const std::nothrow_t& ) noexcept
{
    sm_free( p );
}

sm_decl_new( n ) void*
operator new( std::size_t n ) noexcept( false )
{
    return sm_new( n );
}
sm_decl_new( n ) void*
operator new[]( std::size_t n ) noexcept( false )
{
    return sm_new( n );
}

sm_decl_new_nothrow( n ) void*
operator new( std::size_t n, const std::nothrow_t& tag ) noexcept
{
    (void) ( tag );
    return sm_new_nothrow( n );
}
sm_decl_new_nothrow( n ) void*
operator new[]( std::size_t n, const std::nothrow_t& tag ) noexcept
{
    (void) ( tag );
    return sm_new_nothrow( n );
}

#if ( __cplusplus >= 201402L || _MSC_VER >= 1916 )
void
operator delete( void* p, std::size_t n ) noexcept
{
    sm_free_size( p, n );
};
void
operator delete[]( void* p, std::size_t n ) noexcept
{
    sm_free_size( p, n );
};
#endif /* __cplusplus >= 201402L || _MSC_VER >= 1916 */

#if ( __cplusplus > 201402L || defined( __cpp_aligned_new ) )
void
operator delete( void* p, std::align_val_t al ) noexcept
{
    sm_free_aligned( p, static_cast<size_t>( al ) );
}
void
operator delete[]( void* p, std::align_val_t al ) noexcept
{
    sm_free_aligned( p, static_cast<size_t>( al ) );
}
void
operator delete( void* p, std::size_t n, std::align_val_t al ) noexcept
{
    sm_free_size_aligned( p, n, static_cast<size_t>( al ) );
};
void
operator delete[]( void* p, std::size_t n, std::align_val_t al ) noexcept
{
    sm_free_size_aligned( p, n, static_cast<size_t>( al ) );
};
void
operator delete( void* p, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    sm_free_aligned( p, static_cast<size_t>( al ) );
}
void
operator delete[]( void* p, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    sm_free_aligned( p, static_cast<size_t>( al ) );
}

void*
operator new( std::size_t n, std::align_val_t al ) noexcept( false )
{
    return sm_new_aligned( n, static_cast<size_t>( al ) );
}
void*
operator new[]( std::size_t n, std::align_val_t al ) noexcept( false )
{
    return sm_new_aligned( n, static_cast<size_t>( al ) );
}
void*
operator new( std::size_t n, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    return sm_new_aligned_nothrow( n, static_cast<size_t>( al ) );
}
void*
operator new[]( std::size_t n, std::align_val_t al, const std::nothrow_t& ) noexcept
{
    return sm_new_aligned_nothrow( n, static_cast<size_t>( al ) );
}
#endif /* __cplusplus > 201402L || defined( __cpp_aligned_new ) */

#endif /* __cplusplus */

#endif /* SM_ALLOC_H_ */
