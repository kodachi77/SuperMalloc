#ifndef SM_ALLOC_H_
#define SM_ALLOC_H_

#if defined( _DEBUG ) || defined( SM_TEST )
#define SM_ASSERTS_ENABLED 1
#endif

// SM_OVERRIDE_STD_MALLOC - override standard malloc/free with sm_alloc
// SM_STL_ALLOCATOR_ENABLED - enable STL allocator support
// SM_SHARED_LIB - build as shared library
// SM_SHARED_LIB_EXPORT - export symbols from shared library

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
#if defined( _MSC_VER )
#error "SM_ALLOC: Set /std:c++14 and /Zc:__cplusplus options to enable C++14 standard."
#else
#error "SM_ALLOC: Set /std:c++11 to enable C++11 standard."
#endif
#endif

#if defined( __STDC_VERSION__ ) && !defined( _MSC_VER ) && !defined( __STDC_NO_ATOMICS__ )
#error "SM_ALLOC: C11 <stdatomic.h> not available"
#endif

#if defined( __STDC_VERSION__ ) && !defined( _MSC_VER ) && !defined( __STDC_NO_THREADS__ )
#error "SM_ALLOC: C11 <threads.h> not available"
#endif

#if defined( __cplusplus )
#define sm_attr_noexcept noexcept
#else
#define sm_attr_noexcept
#endif

// nodiscard
#if defined( __cplusplus ) && ( __cplusplus >= 201703L )
#define sm_attr_nodiscard [[nodiscard]]
#elif defined( __GNUC__ ) || defined( __clang__ )
#define sm_attr_nodiscard __attribute__( ( warn_unused_result ) )
#elif defined( _MSC_VER )
#define sm_attr_nodiscard _Check_return_
#else
#define sm_attr_nodiscard
#endif

// export (dll / visibility)
#if defined( _MSC_VER )
#if !defined( SM_SHARED_LIB )
#define sm_attr_export
#elif defined( SM_SHARED_LIB_EXPORT )
#define sm_attr_export __declspec( dllexport )
#else
#define sm_attr_export __declspec( dllimport )
#endif
#elif defined( __GNUC__ ) || defined( __clang__ )
#if defined( SM_SHARED_LIB ) && defined( SM_SHARED_LIB_EXPORT )
#define sm_attr_export __attribute__( ( visibility( "default" ) ) )
#else
#define sm_attr_export
#endif
#else
#define sm_attr_export
#endif

// restrict/allocator (MSVC exposes both; on GCC/Clang leave empty)
#if defined( _MSC_VER )
#define sm_attr_restrict __declspec( allocator ) __declspec( restrict )
#else
#define sm_attr_restrict
#endif

// "function returns new storage" attribute
#if defined( __GNUC__ ) || defined( __clang__ )
#define sm_attr_malloc __attribute__( ( malloc ) )
#else
#define sm_attr_malloc /* (MSVC has no direct equivalent; allocator + restrict used above) */
#endif

// alloc_size / alloc_align (only where supported)
#if ( defined( __clang_major__ ) && ( __clang_major__ < 4 ) ) || ( defined( __GNUC__ ) && ( __GNUC__ < 5 ) )
#define sm_attr_alloc_size( S )
#define sm_attr_alloc_size2( S1, S2 )
#define sm_attr_alloc_align( P )
#else
#if defined( __GNUC__ ) || defined( __clang__ )
#define sm_attr_alloc_size( S )       __attribute__( ( alloc_size( S ) ) )
#define sm_attr_alloc_size2( S1, S2 ) __attribute__( ( alloc_size( S1, S2 ) ) )
#define sm_attr_alloc_align( P )      __attribute__( ( alloc_align( P ) ) )
#else
#define sm_attr_alloc_size( S )
#define sm_attr_alloc_size2( S1, S2 )
#define sm_attr_alloc_align( P )
#endif
#endif

// ============================================================================
//  Unified macros
// ============================================================================

#define sm_decl sm_attr_nodiscard sm_attr_export sm_attr_restrict

#define sm_decl_malloc sm_decl sm_attr_malloc

#define sm_decl_malloc_size( S )  sm_decl sm_attr_malloc sm_attr_alloc_size( S )
#define sm_decl_realloc_size( S ) sm_attr_nodiscard sm_attr_export sm_attr_malloc sm_attr_alloc_size( S )

#define sm_decl_malloc_size2( S1, S2 )  sm_decl sm_attr_malloc sm_attr_alloc_size2( S1, S2 )
#define sm_decl_realloc_size2( S1, S2 ) sm_attr_nodiscard sm_attr_export sm_attr_malloc sm_attr_alloc_size2( S1, S2 )

#define sm_decl_malloc_align( P ) sm_decl sm_attr_malloc sm_attr_alloc_align( P )

#define sm_decl_malloc_size_align( S, P )       sm_decl sm_attr_malloc sm_attr_alloc_size( S ) sm_attr_alloc_align( P )
#define sm_decl_malloc_size2_align( S1, S2, P ) sm_decl sm_attr_malloc sm_attr_alloc_size2( S1, S2 ) sm_attr_alloc_align( P )

#define sm_decl_realloc_size_align( S, P )                                                                                       \
    sm_attr_nodiscard sm_attr_export sm_attr_malloc sm_attr_alloc_size( S ) sm_attr_alloc_align( P )
#define sm_decl_realloc_size2_align( S1, S2, P )                                                                                 \
    sm_attr_nodiscard sm_attr_export sm_attr_malloc sm_attr_alloc_size2( S1, S2 ) sm_attr_alloc_align( P )

#include <stddef.h>    // size_t

#if defined( __cplusplus )
#include <new>

//using std::size_t;

#if defined( SM_STL_ALLOCATOR_ENABLED )
#include <cstddef>        // std::size_t
#include <limits>         // std::numeric_limits
#include <type_traits>    // std::true_type
#include <utility>        // std::forward
#endif

//#else
//#include <stdbool.h>    // bool
//#include <stddef.h>     // size_t
#endif

#ifdef __cplusplus
extern "C"
{
#endif

// -----------------------------------------------------------------------------
// Standard malloc/free
// -----------------------------------------------------------------------------

sm_decl_malloc_size( 1 ) void* sm_malloc( size_t size ) sm_attr_noexcept;
sm_decl_malloc_size2( 1, 2 ) void* sm_calloc( size_t count, size_t size ) sm_attr_noexcept;
sm_decl_realloc_size( 2 ) void* sm_realloc( void* p, size_t newsize ) sm_attr_noexcept;
sm_decl_malloc_size( 2 ) void* sm_expand( void* p, size_t newsize ) sm_attr_noexcept;

sm_attr_export void sm_free( void* p ) sm_attr_noexcept;
sm_attr_export void sm_free_size( void* p, size_t size ) sm_attr_noexcept;
sm_attr_export void sm_free_aligned( void* p, size_t alignment ) sm_attr_noexcept;
sm_attr_export void sm_free_size_aligned( void* p, size_t size, size_t alignment ) sm_attr_noexcept;

// -----------------------------------------------------------------------------
// Size/introspection
// -----------------------------------------------------------------------------
sm_attr_export size_t sm_usable_size( void* p ) sm_attr_noexcept;
sm_attr_export size_t sm_malloc_good_size( size_t sz ) sm_attr_noexcept;

// -----------------------------------------------------------------------------
// Dup helpers
// -----------------------------------------------------------------------------
sm_decl_malloc char*           sm_strdup( const char* s ) sm_attr_noexcept;
sm_decl_malloc char*           sm_strndup( const char* s, size_t n ) sm_attr_noexcept;
sm_decl_malloc char*           sm_realpath( const char* fname, char* resolved_name ) sm_attr_noexcept;
sm_decl_malloc char*           sm_mbsdup( const char* s ) sm_attr_noexcept;
sm_decl_malloc unsigned short* sm_wcsdup( const unsigned short* s ) sm_attr_noexcept;

// Windows-style env dup (return 0 on success; errno-style otherwise)
sm_attr_export int sm_dupenv_s( char** buffer, size_t* numberOfElements, const char* varname ) sm_attr_noexcept;
sm_attr_export int sm_wdupenv_s( unsigned short** buffer, size_t* numberOfElements,
                                 const unsigned short* varname ) sm_attr_noexcept;

// -----------------------------------------------------------------------------
// Realloc variants
// -----------------------------------------------------------------------------
sm_decl_realloc_size( 2 ) void* sm_reallocf( void* p, size_t newsize ) sm_attr_noexcept;    // frees on failure
sm_decl_realloc_size2( 2, 3 ) void* sm_reallocarray( void* p, size_t nmemb,
                                                     size_t size ) sm_attr_noexcept;         // nmemb*size overflow-safe
sm_attr_export int sm_reallocarr( void** p, size_t nmemb, size_t size ) sm_attr_noexcept;    // 0 on success
sm_decl_realloc_size2( 2, 3 ) void* sm_recalloc( void* p, size_t count, size_t size ) sm_attr_noexcept;    // like _recalloc

// -----------------------------------------------------------------------------
// Aligned allocation family
// -----------------------------------------------------------------------------
sm_decl_malloc_size_align( 1, 2 ) void* sm_malloc_aligned( size_t size, size_t alignment ) sm_attr_noexcept;
sm_decl sm_attr_malloc sm_attr_alloc_size( 2 )
    sm_attr_alloc_align( 3 ) void* sm_realloc_aligned( void* p, size_t newsize, size_t alignment ) sm_attr_noexcept;
sm_decl sm_attr_malloc sm_attr_alloc_size2( 2, 3 )
    sm_attr_alloc_align( 4 ) void* sm_aligned_recalloc( void* p, size_t count, size_t size, size_t alignment ) sm_attr_noexcept;

// With explicit offset
sm_decl_malloc_size_align( 1, 2 ) void* sm_malloc_aligned_at( size_t size, size_t alignment, size_t offset ) sm_attr_noexcept;
sm_decl_realloc_size_align( 2, 3 ) void* sm_realloc_aligned_at( void* p, size_t newsize, size_t alignment,
                                                                size_t offset ) sm_attr_noexcept;
sm_decl_realloc_size2_align( 2, 3, 4 ) void* sm_recalloc_aligned_at( void* p, size_t count, size_t size, size_t alignment,
                                                                     size_t offset ) sm_attr_noexcept;

// -----------------------------------------------------------------------------
// POSIX / BSD / GNU / C11 compatibles
// -----------------------------------------------------------------------------
sm_decl_malloc_size( 1 ) void* sm_valloc( size_t size ) sm_attr_noexcept;
sm_decl_malloc_size( 1 ) void* sm_pvalloc( size_t size ) sm_attr_noexcept;

sm_decl_malloc_size_align( 2, 1 ) void* sm_memalign( size_t alignment, size_t size ) sm_attr_noexcept;
sm_decl_malloc_size_align( 2, 1 ) void* sm_aligned_alloc( size_t alignment, size_t size ) sm_attr_noexcept;

sm_attr_export int sm_posix_memalign( void** memptr, size_t alignment, size_t size ) sm_attr_noexcept;

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

// The 'sm_new' wrappers implement C++ semantics on out-of-memory instead of directly returning NULL.
// They call 'std::get_new_handler' and potentially raise a 'std::bad_alloc' exception).
sm_decl_malloc_size( 1 ) void* sm_new( size_t size );
sm_decl_malloc_size_align( 1, 2 ) void* sm_new_aligned( size_t size, size_t alignment );
sm_decl_malloc_size( 1 ) void* sm_new_nothrow( size_t size ) noexcept;
sm_decl_malloc_size_align( 1, 2 ) void* sm_new_aligned_nothrow( size_t size, size_t alignment ) noexcept;
sm_decl_malloc_size2( 1, 2 ) void* sm_new_n( size_t count, size_t size );
sm_decl_realloc_size( 2 ) void* sm_new_realloc( void* p, size_t newsize );
sm_decl_realloc_size2( 2, 3 ) void* sm_new_reallocn( void* p, size_t newcount, size_t size );

#if defined( _MSC_VER )
#define sm_decl_new( n ) sm_attr_nodiscard sm_attr_restrict _Ret_notnull_ _Post_writable_byte_size_( n )
#define sm_decl_new_nothrow( n )                                                                                                 \
    sm_attr_nodiscard sm_attr_restrict _Ret_maybenull_ _Success_( return != NULL ) _Post_writable_byte_size_( n )
#else
#define sm_decl_new( n )         sm_attr_nodiscard sm_attr_restrict
#define sm_decl_new_nothrow( n ) sm_attr_nodiscard sm_attr_restrict
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

#if ( __cplusplus >= 201402L )
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

#endif /* __cplusplus >= 201402L */

#if ( __cplusplus >= 201703L )

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
#endif /* __cplusplus >= 201703L */

#if defined( SM_STL_ALLOCATOR_ENABLED )

template <class T>
struct _sm_stl_allocator_common
{
    typedef T                 value_type;
    typedef std::size_t       size_type;
    typedef std::ptrdiff_t    difference_type;
    typedef value_type&       reference;
    typedef value_type const& const_reference;
    typedef value_type*       pointer;
    typedef value_type const* const_pointer;

#if ( __cplusplus >= 201103L )    // C++11
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;
    template <class U, class... Args>
    void construct( U* p, Args&&... args )
    {
        ::new( p ) U( std::forward<Args>( args )... );
    }
    template <class U>
    void destroy( U* p ) noexcept
    {
        p->~U();
    }
#else
    void construct( pointer p, value_type const& val ) { ::new( p ) value_type( val ); }
    void destroy( pointer p ) { p->~value_type(); }
#endif

    size_type     max_size() const noexcept { return ( std::numeric_limits<difference_type>::max() / sizeof( value_type ) ); }
    pointer       address( reference x ) const { return &x; }
    const_pointer address( const_reference x ) const { return &x; }
};

template <class T>
struct sm_stl_allocator : public _sm_stl_allocator_common<T>
{
    using typename _sm_stl_allocator_common<T>::size_type;
    using typename _sm_stl_allocator_common<T>::value_type;
    using typename _sm_stl_allocator_common<T>::pointer;
    template <class U>
    struct rebind
    {
        typedef sm_stl_allocator<U> other;
    };

    sm_stl_allocator() noexcept                          = default;
    sm_stl_allocator( const sm_stl_allocator& ) noexcept = default;
    template <class U>
    sm_stl_allocator( const sm_stl_allocator<U>& ) noexcept
    {
    }
    sm_stl_allocator select_on_container_copy_construction() const { return *this; }
    void             deallocate( T* p, size_type ) { sm_free( p ); }

#if ( __cplusplus >= 201703L )    // C++17
    sm_attr_nodiscard T* allocate( size_type count ) { return static_cast<T*>( sm_new_n( count, sizeof( T ) ) ); }
    sm_attr_nodiscard T* allocate( size_type count, const void* ) { return allocate( count ); }
#else
    sm_attr_nodiscard pointer allocate( size_type count, const void* = 0 )
    {
        return static_cast<pointer>( sm_new_n( count, sizeof( value_type ) ) );
    }
#endif

#if ( __cplusplus >= 201103L )    // C++11
    using is_always_equal = std::true_type;
#endif
};

template <class T1, class T2>
bool
operator==( const sm_stl_allocator<T1>&, const sm_stl_allocator<T2>& ) noexcept
{
    return true;
}
template <class T1, class T2>
bool
operator!=( const sm_stl_allocator<T1>&, const sm_stl_allocator<T2>& ) noexcept
{
    return false;
}

#endif /* SM_STL_ALLOCATOR_ENABLED */

#endif /* __cplusplus */

#endif /* SM_ALLOC_H_ */
