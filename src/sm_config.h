#pragma once

#include <stdint.h>

// sanity checks
#if !( defined( _M_X64 ) || defined( _M_AMD64 ) || defined( __x86_64__ ) || defined( __amd64__ ) || defined( __amd64 ) )
#error "This allocator only supports x86-64 architecture."
#endif

#if defined( _M_ARM64 ) || defined( __arm64__ ) || defined( __arm64 ) || defined( __aarch64__ )
#error "This allocator doesn't support ARM architecture just yet."
#endif

#if defined( _MSC_VER ) && !defined( _WIN64 )
#error "This allocator only supports 64-bit Windows."
#endif

#if INTPTR_MAX == INT32_MAX
#error "This allocator requires 64-bit pointers."
#endif

#if defined( __STDC_VERSION__ ) && ( __STDC_VERSION__ < 201112L )
#error "This allocator requires C11 standard support"
#endif

#if defined( _MSC_VER ) && !_ISO_VOLATILE
#error "Option /volatile:iso option must be used."
#endif

#if defined( _MSC_VER ) && _MSVC_TRADITIONAL
#error "Option /Zc:preprocessor option must be used."
#endif

#if defined( _WIN64 )
#define SM_PLATFORM_WINDOWS 1
#endif

#if defined( __linux ) || defined( __linux__ )
#define SM_PLATFORM_LINUX 1
#endif

//> TODO: replace with SM_TESTING
#if defined( _DEBUG ) || defined( TESTING )
#define SM_ASSERTS_ENABLED 1
#endif

#if defined( _MSC_VER ) & !defined( __clang__ )

#ifdef __cplusplus
#define SM_ALIGNED( num ) __declspec( align( ( num ) ) )
#else
#define SM_ALIGNED( num ) _Alignas( ( num ) )
#endif

#define SM_ATTRIBUTE_THREAD __declspec( thread )
#define SM_ATTRIBUTE_UNROLL

// clang-format off
#define SM_LOCK_INITIALIZER  {0}
// clang-format on

#elif defined( __GNUC__ )

#define SM_LOCK_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#define SM_ALIGNED( num )   __attribute__( ( aligned( ( num ) ) ) )
#define SM_ATTRIBUTE_THREAD __thread
#define SM_ATTRIBUTE_UNROLL __attribute__( ( optimize( "unroll-loops" ) ) )

#endif