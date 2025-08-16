#pragma once

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

#ifdef _WIN64
#include <immintrin.h>
#include "_windows.h"
#endif

#include <stdbool.h>

#include "sm_config.h"

#include "sm_assert.h"
#include "sm_atomic.h"
#include "sm_bitops.h"
#include "sm_platform.h"

typedef struct lock_t
{
    union
    {
#ifdef __linux__
        SM_ALIGNED( 64 ) pthread_mutex_t pt_m;
#elif defined( _WIN64 )
        SM_ALIGNED( 64 ) CRITICAL_SECTION cs_m;
#endif
    };
} lock_t;

//>typedef sm_lock_t lock_t;

static inline void
initialize_lock_array( lock_t* locks, size_t count )
{
#ifdef __linux__
    pthread_mutexattr_t attr;
    pthread_mutexattr_init( &attr );
    for( size_t i = 0; i < count; ++i ) { pthread_mutex_init( &locks[i].pt_m, &attr ); }
#elif defined( _WIN64 )
    for( size_t i = 0; i < count; ++i ) { InitializeCriticalSectionAndSpinCount( &locks[i].cs_m, 4000 ); }
#endif
    //>for( size_t i = 0; i < count; ++i ) { sm_lock_init( &locks[i] ); }
}
static inline void
sm_lock( lock_t* mylock )
{
#ifdef __linux__
    pthread_mutex_lock( &mylock->pt_m );
#elif defined( _WIN64 )
    EnterCriticalSection( &mylock->cs_m );
#endif
}

static inline void
sm_unlock( lock_t* mylock )
{
#ifdef __linux__
    pthread_mutex_unlock( &mylock->pt_m );
#elif defined( _WIN64 )
    LeaveCriticalSection( &mylock->cs_m );
#endif
}

#define __GET_NTH_ARG__( _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, N, ... ) N
#define __NARG__( ... )                                                                                       __GET_NTH_ARG__( "ignored", ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0 )

#define __VA_0( arg, n, ... )
#define __VA_1( arg, n, x )       arg( x, 1 )
#define __VA_2( arg, n, x, ... )  arg( x, n ), __VA_1( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_3( arg, n, x, ... )  arg( x, n ), __VA_2( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_4( arg, n, x, ... )  arg( x, n ), __VA_3( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_5( arg, n, x, ... )  arg( x, n ), __VA_4( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_6( arg, n, x, ... )  arg( x, n ), __VA_5( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_7( arg, n, x, ... )  arg( x, n ), __VA_6( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_8( arg, n, x, ... )  arg( x, n ), __VA_7( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_9( arg, n, x, ... )  arg( x, n ), __VA_8( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_10( arg, n, x, ... ) arg( x, n ), __VA_9( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_11( arg, n, x, ... ) arg( x, n ), __VA_10( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_12( arg, n, x, ... ) arg( x, n ), __VA_11( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_13( arg, n, x, ... ) arg( x, n ), __VA_12( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_14( arg, n, x, ... ) arg( x, n ), __VA_13( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_15( arg, n, x, ... ) arg( x, n ), __VA_14( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )
#define __VA_16( arg, n, x, ... ) arg( x, n ), __VA_15( arg, __NARG__( __VA_ARGS__ ), __VA_ARGS__ )

#define SM_CALL_MACRO_FOR_EACH( m, ... )                                                                                         \
    __GET_NTH_ARG__( "ignored", ##__VA_ARGS__, __VA_16, __VA_15, __VA_14, __VA_13, __VA_12, __VA_11, __VA_10, __VA_9, __VA_8,    \
                     __VA_7, __VA_6, __VA_5, __VA_4, __VA_3, __VA_2, __VA_1, __VA_0 )                                            \
    ( m, __NARG__( __VA_ARGS__ ), ##__VA_ARGS__ )

#define _DEFINE_ARG( type, n ) type a##n
#define _INVOKE_ARG( type, n ) a##n

#define SM_DECLARE_ATOMIC_OPERATION( atomic_name, fun, ret_type, ... )                                                        \
    ret_type atomic_name( lock_t* mylock, SM_CALL_MACRO_FOR_EACH( _DEFINE_ARG, ##__VA_ARGS__ ) )                                     \
    {                                                                                                                                \
        sm_lock( mylock );                                                                                                           \
        ret_type r = fun( SM_CALL_MACRO_FOR_EACH( _INVOKE_ARG, ##__VA_ARGS__ ) );                                                    \
        sm_unlock( mylock );                                                                                                         \
        return r;                                                                                                                    \
    }

#define SM_DECLARE_ATOMIC_OPERATION2( atomic_name, fun, ret_type, ... )                                                       \
    ret_type atomic_name( lock_t* mylock1, lock_t* mylock2, SM_CALL_MACRO_FOR_EACH( _DEFINE_ARG, ##__VA_ARGS__ ) )                   \
    {                                                                                                                                \
        sm_lock( mylock1 );                                                                                                          \
        sm_lock( mylock2 );                                                                                                          \
        ret_type r = fun( SM_CALL_MACRO_FOR_EACH( _INVOKE_ARG, ##__VA_ARGS__ ) );                                                    \
        sm_unlock( mylock2 );                                                                                                        \
        sm_unlock( mylock1 );                                                                                                        \
        return r;                                                                                                                    \
    }

#define SM_INVOKE_ATOMIC_OPERATION( mylock, atomic_name, ... )            atomic_name( mylock, ##__VA_ARGS__ )
#define SM_INVOKE_ATOMIC_OPERATION2( mylock1, mylock2, atomic_name, ... ) atomic_name( mylock1, mylock2, ##__VA_ARGS__ )

static inline void
fetch_and_max( _Atomic uint64_t* ptr, uint64_t val )
{
    while( 1 )
    {
        uint64_t old = atomic_load( ptr );
        if( val <= old ) return;
        if( atomic_compare_and_swap( ptr, &old, val ) ) return;
    }
}
