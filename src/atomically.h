#ifndef ATOMICALLY_H
#define ATOMICALLY_H

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

#include <immintrin.h>
#include <stdbool.h>

#include "bassert.h"
#include "rng.h"

#include "config.h"

struct lock_t
{
    union
    {
#ifdef __linux__
        pthread_mutex_t ALIGNED( 64 ) pt_m;
#elif defined( _WIN64 )
        CRITICAL_SECTION ALIGNED( 64 ) cs_m;
#endif
    };
#ifdef __linux__
    //  lock_t() : pt_m( PTHREAD_MUTEX_INITIALIZER ) {}
#elif defined( _WIN64 )
    lock_t() { InitializeCriticalSectionAndSpinCount( &cs_m, 4000 ); }
#endif
};

// Lock wrapper class.
class mylock_raii
{
    lock_t* mylock;

   public:
    mylock_raii( lock_t* mylock ) : mylock( mylock )
    {
#ifdef __linux__
        pthread_mutex_lock( &mylock->pt_m );
#elif defined( _WIN64 )
        EnterCriticalSection( &mylock->cs_m );
#endif
    }
    ~mylock_raii()
    {
#ifdef __linux__
        pthread_mutex_unlock( &mylock->pt_m );
#elif defined( _WIN64 )
        LeaveCriticalSection( &mylock->cs_m );
#endif
    }
};

extern bool do_predo;

#define XABORT_LOCK_HELD 9

//#define DO_FAILED_COUNTS
#ifdef DO_FAILED_COUNTS
struct atomic_stats_s
{
    uint64_t atomic_count __attribute__( ( aligned( 64 ) ) );
    uint64_t locked_count;
};
extern struct atomic_stats_s atomic_stats;

struct failed_counts_s
{
    const char*  name;
    unsigned int code;
    uint64_t     count;
};
extern lock_t                 failed_counts_mutex;
static const int              max_failed_counts = 100;
extern int                    failed_counts_n;
extern struct failed_counts_s failed_counts[max_failed_counts];
#endif

template <typename ReturnType, typename... Arguments>
static inline ReturnType
atomically( lock_t* mylock, const char* /*name*/, void ( *predo )( Arguments... args ), ReturnType ( *fun )( Arguments... args ),
            Arguments... args )
{
    {
        mylock_raii m( mylock );
        ReturnType  r = fun( args... );
        return r;
    }
#ifdef DO_FAILED_COUNTS
    __sync_fetch_and_add( &atomic_stats.atomic_count, 1 );
#endif
    unsigned int xr = 0xfffffff2;

// We finally give up and acquire the lock.
#ifdef DO_FAILED_COUNTS
    {
        mylock_raii m( &failed_counts_mutex );
        for( int i = 0; i < failed_counts_n; i++ )
        {
            if( failed_counts[i].name == name && failed_counts[i].code == xr )
            {
                failed_counts[i].count++;
                goto didit;
            }
        }
        SM_ASSERT( failed_counts_n < max_failed_counts );
        failed_counts[failed_counts_n++] = ( struct failed_counts_s ) { name, xr, 1 };
    didit:;
    }

    __sync_fetch_and_add( &atomic_stats.locked_count, 1 );
#endif
    if( do_predo ) predo( args... );
    mylock_raii mr( mylock );
    ReturnType  r = fun( args... );
    return r;
}

template <typename ReturnType, typename... Arguments>
static inline ReturnType
atomically2( lock_t* lock0, lock_t* lock1, const char* /*name*/, void ( *predo )( Arguments... args ),
             ReturnType ( *fun )( Arguments... args ), Arguments... args )
{
    {
        mylock_raii m0( lock0 );
        mylock_raii m1( lock1 );
        ReturnType  r = fun( args... );
        return r;
    }
#ifdef DO_FAILED_COUNTS
    __sync_fetch_and_add( &atomic_stats.atomic_count, 1 );
#endif
    unsigned int xr = 0xfffffff2;
// We finally give up and acquire the lock.
#ifdef DO_FAILED_COUNTS
    {
        mylock_raii m( &failed_counts_mutex );
        for( int i = 0; i < failed_counts_n; i++ )
        {
            if( failed_counts[i].name == name && failed_counts[i].code == xr )
            {
                failed_counts[i].count++;
                goto didit;
            }
        }
        SM_ASSERT( failed_counts_n < max_failed_counts );
        failed_counts[failed_counts_n++] = ( struct failed_counts_s ) { name, xr, 1 };
    didit:;
    }

    __sync_fetch_and_add( &atomic_stats.locked_count, 1 );
#endif
    if( do_predo ) predo( args... );
    mylock_raii mr0( lock0 );
    mylock_raii mr1( lock1 );
    ReturnType  r = fun( args... );
    return r;
}

#ifdef __linux__
#define atomic_load( addr )       __atomic_load_n( addr, __ATOMIC_CONSUME )
#define atomic_store( addr, val ) __atomic_store_n( addr, val, __ATOMIC_RELEASE )

#define prefetch_read( addr )  __builtin_prefetch( addr, 0, 3 )
#define prefetch_write( addr ) __builtin_prefetch( addr, 1, 3 )
#define load_and_prefetch_write( addr )                                                                                          \
    (                                                                                                                            \
        {                                                                                                                        \
            __typeof__( *addr ) ignore __attribute__( ( unused ) ) = atomic_load( addr );                                        \
            prefetch_write( addr );                                                                                              \
        } )
#elif _WIN64
#define prefetch_read( addr )  _mm_prefetch( (const char*) addr, _MM_HINT_T0 )
#define prefetch_write( addr ) _mm_prefetch( (const char*) addr, _MM_HINT_T0 )
#define load_and_prefetch_write( addr )                                                                                          \
    do {                                                                                                                         \
        auto s = atomic_load( addr );                                                                                            \
        _mm_prefetch( (const char*) addr, _MM_HINT_T0 );                                                                         \
    } while( 0 )

#endif

static inline void
fetch_and_max( uint64_t* ptr, uint64_t val )
{
    while( 1 )
    {
        uint64_t old = atomic_load( ptr );
        if( val <= old ) return;
        if( __sync_bool_compare_and_swap( ptr, old, val ) ) return;
    }
}

#endif    // ATOMICALLY_H
