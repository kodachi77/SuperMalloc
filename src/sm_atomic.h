#pragma once

#include <stdbool.h>    //> TODO: remove later

#include <stddef.h>
#include <stdint.h>

#if defined( _MSC_VER ) && defined( __STDC_NO_ATOMICS__ ) && !defined( __clang__ )

#include <intrin.h>
#include <nmmintrin.h>

#pragma intrinsic( _InterlockedExchange8 )
#pragma intrinsic( _InterlockedExchange16 )
#pragma intrinsic( _InterlockedExchange )
#pragma intrinsic( _InterlockedExchange64 )

#pragma intrinsic( _InterlockedExchangeAdd8 )
#pragma intrinsic( _InterlockedExchangeAdd16 )
#pragma intrinsic( _InterlockedExchangeAdd )
#pragma intrinsic( _InterlockedExchangeAdd64 )

#pragma intrinsic( _InterlockedCompareExchange8 )
#pragma intrinsic( _InterlockedCompareExchange16 )
#pragma intrinsic( _InterlockedCompareExchange )
#pragma intrinsic( _InterlockedCompareExchange64 )

#pragma intrinsic( _InterlockedAnd8 )
#pragma intrinsic( _InterlockedAnd16 )
#pragma intrinsic( _InterlockedAnd )
#pragma intrinsic( _InterlockedAnd64 )

#pragma intrinsic( _InterlockedOr8 )
#pragma intrinsic( _InterlockedOr16 )
#pragma intrinsic( _InterlockedOr )
#pragma intrinsic( _InterlockedOr64 )

#pragma intrinsic( _InterlockedXor8 )
#pragma intrinsic( _InterlockedXor16 )
#pragma intrinsic( _InterlockedXor )
#pragma intrinsic( _InterlockedXor64 )

#pragma intrinsic( _InterlockedExchangePointer )
#pragma intrinsic( _InterlockedCompareExchangePointer )

#define _InterlockedExchange32        _InterlockedExchange
#define _InterlockedExchangeAdd32     _InterlockedExchangeAdd
#define _InterlockedCompareExchange32 _InterlockedCompareExchange
#define _InterlockedAnd32             _InterlockedAnd
#define _InterlockedOr32              _InterlockedOr
#define _InterlockedXor32             _InterlockedXor

#if defined( __cplusplus )
extern "C"
{
#endif

#define _Atomic volatile

// clang-format off
#define ATOMIC_FLAG_INIT { 0 }
#define ATOMIC_VAR_INIT( value ) ( value )
#define kill_dependency( y ) ( y )
    // clang-format on

    typedef char             atomic_int8;
    typedef unsigned char    atomic_uint8;
    typedef short            atomic_int16;
    typedef unsigned short   atomic_uint16;
    typedef long             atomic_int32;
    typedef unsigned long    atomic_uint32;
    typedef __int64          atomic_int64;
    typedef unsigned __int64 atomic_uint64;
    typedef unsigned char    atomic_bool;
    typedef void*            atomic_ptr;

    typedef enum memory_order
    {

        memory_order_relaxed = 0,
        memory_order_consume,
        memory_order_acquire,
        memory_order_release,
        memory_order_acq_rel,
        memory_order_seq_cst
    } memory_order;
    //------------------------------------------------------------------------------

#define SM_ATOMIC_INLINE __forceinline

#define __concat2( x, y )    x##y
#define __concat3( x, y, z ) x##y##z

// clang-format off
#define SM_ATOMIC_FUNCTIONS_TEMPLATE(A, C, T) \
    static SM_ATOMIC_INLINE A __concat3( _, A, _load_explicit )( volatile A * ptr, enum memory_order order ) { \
        (void) order; \
        _ReadBarrier(); \
        T ret = *ptr; \
        _ReadWriteBarrier(); \
        return ret; \
    } \
    static SM_ATOMIC_INLINE atomic_bool __concat3( _, A, _compare_and_swap_explicit)( volatile A* dst, A expected, A desired, enum memory_order order ) { \
        (void) order; \
        return __concat2(_InterlockedCompareExchange, C)( (volatile T*) dst, ( T ) desired, ( T )expected ) == expected; \
    } \
    static SM_ATOMIC_INLINE A __concat3( _, A, _exchange_explicit)( volatile A* dst, A src, enum memory_order order ) { \
        (void) order; \
        return (A) __concat2(_InterlockedExchange, C)( (volatile T*) dst, (T) src ); \
    } \
    static SM_ATOMIC_INLINE A __concat3( _, A, _fetch_and_explicit)( volatile A* dst, A src, enum memory_order order ) { \
        (void) order; \
        return (A) __concat2( _InterlockedAnd, C )( (volatile T*) dst, (T) src ); \
    } \
    static SM_ATOMIC_INLINE A __concat3( _, A, _fetch_or_explicit)( volatile T* dst, T src, enum memory_order order ) { \
        (void) order; \
        return (A) __concat2(_InterlockedOr, C)( (volatile T*) dst, (T)src ); \
    } \
    static SM_ATOMIC_INLINE A __concat3( _, A, _fetch_xor_explicit)( volatile A* dst, A src, enum memory_order order ) { \
        (void) order; \
        return (A) __concat2( _InterlockedXor, C)( (volatile T*) dst, (T)src ); \
    } \
    static SM_ATOMIC_INLINE A __concat3( _, A, _fetch_add_explicit)( volatile A* dst, A src, enum memory_order order ) { \
        (void) order; \
        return (A) __concat2( _InterlockedExchangeAdd, C)( (volatile T*) dst, (T) src ); \
    } \
    static SM_ATOMIC_INLINE A __concat3( _, A, _fetch_sub_explicit)( volatile A* dst, A src, enum memory_order order ) { \
        (void) order; \
        return (A) __concat2( _InterlockedExchangeAdd, C)( (volatile T*) dst, (T)( -( src ) ) ); \
    } \
    static SM_ATOMIC_INLINE atomic_bool __concat3( _, A, _compare_exchange_strong_explicit)( volatile A* dst, A* expected, A desired, enum memory_order success_order, enum memory_order failure_order ) { \
        A expected_value; \
        A result; \
        expected_value = __concat3( _, A, _load_explicit)( expected, memory_order_seq_cst ); \
        result         = __concat3( _, A, _compare_and_swap_explicit)( dst, expected_value, desired, success_order ); \
        if( result == expected_value ) { \
            return 1; \
        } else {  \
            __concat3( _, A, _exchange_explicit)( expected, result, failure_order ); \
            return 0; \
        } \
    } \
    static SM_ATOMIC_INLINE atomic_bool __concat3( _, A, _is_lock_free)( volatile void* ptr ) { \
        (void) ptr; \
        return 1; \
    }

    // clang-format on

    //------------------------------------------------------------------------------
#pragma warning( push )
#pragma warning( disable : 4146 )

    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_int8, 8, char )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_uint8, 8, char )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_int16, 16, short )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_uint16, 16, short )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_int32, 32, long )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_uint32, 32, long )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_int64, 64, __int64 )
    SM_ATOMIC_FUNCTIONS_TEMPLATE( atomic_uint64, 64, __int64 )

#pragma warning( pop )
    //------------------------------------------------------------------------------

    static SM_ATOMIC_INLINE atomic_ptr _atomic_ptr_load_explicit( volatile atomic_ptr* ptr, enum memory_order order )
    {
        (void) order;
        _ReadBarrier();
        atomic_ptr ret = *ptr;
        _ReadWriteBarrier();
        return ret;
    }

    static SM_ATOMIC_INLINE atomic_bool _atomic_ptr_compare_and_swap_explicit( volatile atomic_ptr* dst, atomic_ptr expected,
                                                                               atomic_ptr desired, enum memory_order order )
    {
        (void) order;
        return _InterlockedCompareExchangePointer( (void* volatile*) dst, (void*) desired, (void*) expected ) == expected;
    }

#define _atomic_load_f( ptr )                                                                                                    \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_load_explicit, volatile atomic_uint8 *                                                              \
              : _atomic_uint8_load_explicit, volatile atomic_int16 *                                                             \
              : _atomic_int16_load_explicit, volatile atomic_uint16 *                                                            \
              : _atomic_uint16_load_explicit, volatile atomic_int32 *                                                            \
              : _atomic_int32_load_explicit, volatile atomic_uint32 *                                                            \
              : _atomic_uint32_load_explicit, volatile atomic_int64 *                                                            \
              : _atomic_int64_load_explicit, volatile atomic_uint64 *                                                            \
              : _atomic_uint64_load_explicit, volatile atomic_ptr *                                                              \
              : _atomic_ptr_load_explicit )

#define atomic_load( ptr )                 _atomic_load_f( ptr )( ( ptr ), memory_order_seq_cst )
#define atomic_load_explicit( ptr, order ) _atomic_load_f( ptr )( ( ptr ), ( order ) )

    //------------------------------------------------------------------------------

#define _atomic_exchange_f( ptr )                                                                                                \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_exchange_explicit, volatile atomic_uint8 *                                                          \
              : _atomic_uint8_exchange_explicit, volatile atomic_int16 *                                                         \
              : _atomic_int16_exchange_explicit, volatile atomic_uint16 *                                                        \
              : _atomic_uint16_exchange_explicit, volatile atomic_int32 *                                                        \
              : _atomic_int32_exchange_explicit, volatile atomic_uint32 *                                                        \
              : _atomic_uint32_exchange_explicit, volatile atomic_int64 *                                                        \
              : _atomic_int64_exchange_explicit, volatile atomic_uint64 *                                                        \
              : _atomic_uint64_exchange_explicit )

#define atomic_store( dst, src )                 _atomic_exchange_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_store_explicit( dst, src, order ) _atomic_exchange_f( ( dst ) )( ( dst ), ( src ), ( order ) )

// store 1
#define atomic_test_and_set( dst )                 _atomic_exchange_f( ( dst ) )( ( dst ), 1, memory_order_seq_cst )
#define atomic_test_and_set_explicit( dst, order ) _atomic_exchange_f( ( dst ) )( ( dst ), 1, ( order ) )

// store 0
#define atomic_clear( dst )                 _atomic_exchange_f( ( dst ) )( ( dst ), 0, memory_order_seq_cst )
#define atomic_clear_explicit( dst, order ) _atomic_exchange_f( ( dst ) )( ( dst ), 0, ( order ) )

#define atomic_exchange( dst, src )                 _atomic_exchange_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_exchange_explicit( dst, src, order ) _atomic_exchange_f( ( dst ) )( ( dst ), ( src ), ( order ) )

    //------------------------------------------------------------------------------
#define _atomic_compare_and_swap_f( ptr )                                                                                        \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_compare_and_swap_explicit, volatile atomic_uint8 *                                                  \
              : _atomic_uint8_compare_and_swap_explicit, volatile atomic_int16 *                                                 \
              : _atomic_int16_compare_and_swap_explicit, volatile atomic_uint16 *                                                \
              : _atomic_uint16_compare_and_swap_explicit, volatile atomic_int32 *                                                \
              : _atomic_int32_compare_and_swap_explicit, volatile atomic_uint32 *                                                \
              : _atomic_uint32_compare_and_swap_explicit, volatile atomic_int64 *                                                \
              : _atomic_int64_compare_and_swap_explicit, volatile atomic_uint64 *                                                \
              : _atomic_uint64_compare_and_swap_explicit, volatile atomic_ptr *                                                  \
              : _atomic_ptr_compare_and_swap_explicit )

#define atomic_compare_and_swap( dst, exp, des )                                                                                 \
    _atomic_compare_and_swap_f( ( dst ) )( ( dst ), ( exp ), ( des ), memory_order_seq_cst )

#define atomic_compare_and_swap_explicit( dst, src, order ) _atomic_compare_and_swap_f( ( dst ) )(  dst ), ( exp ), (des), ( order ) )
    //------------------------------------------------------------------------------

#define _atomic_fetch_and_f( ptr )                                                                                               \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_fetch_and_explicit, volatile atomic_uint8 *                                                         \
              : _atomic_uint8_fetch_and_explicit, volatile atomic_int16 *                                                        \
              : _atomic_int16_fetch_and_explicit, volatile atomic_uint16 *                                                       \
              : _atomic_uint16_fetch_and_explicit, volatile atomic_int32 *                                                       \
              : _atomic_int32_fetch_and_explicit, volatile atomic_uint32 *                                                       \
              : _atomic_uint32_fetch_and_explicit, volatile atomic_int64 *                                                       \
              : _atomic_int64_fetch_and_explicit, volatile atomic_uint64 *                                                       \
              : _atomic_uint64_fetch_and_explicit )

#define atomic_fetch_and( dst, src )                 _atomic_fetch_and_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_fetch_and_explicit( dst, src, order ) _atomic_fetch_and_f( ( dst ) )( ( dst ), ( src ), ( order ) )

    //------------------------------------------------------------------------------

#define _atomic_fetch_or_f( ptr )                                                                                                \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_fetch_or_explicit, volatile atomic_uint8 *                                                          \
              : _atomic_uint8_fetch_or_explicit, volatile atomic_int16 *                                                         \
              : _atomic_int16_fetch_or_explicit, volatile atomic_uint16 *                                                        \
              : _atomic_uint16_fetch_or_explicit, volatile atomic_int32 *                                                        \
              : _atomic_int32_fetch_or_explicit, volatile atomic_uint32 *                                                        \
              : _atomic_uint32_fetch_or_explicit, volatile atomic_int64 *                                                        \
              : _atomic_int64_fetch_or_explicit, volatile atomic_uint64 *                                                        \
              : _atomic_uint64_fetch_or_explicit )

#define atomic_fetch_or( dst, src )                 _atomic_fetch_or_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_fetch_or_explicit( dst, src, order ) _atomic_fetch_or_f( ( dst ) )( ( dst ), ( src ), ( order ) )

    //------------------------------------------------------------------------------

#define _atomic_fetch_xor_f( ptr )                                                                                               \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_fetch_xor_explicit, volatile atomic_uint8 *                                                         \
              : _atomic_uint8_fetch_xor_explicit, volatile atomic_int16 *                                                        \
              : _atomic_int16_fetch_xor_explicit, volatile atomic_uint16 *                                                       \
              : _atomic_uint16_fetch_xor_explicit, volatile atomic_int32 *                                                       \
              : _atomic_int32_fetch_xor_explicit, volatile atomic_uint32 *                                                       \
              : _atomic_uint32_fetch_xor_explicit, volatile atomic_int64 *                                                       \
              : _atomic_int64_fetch_xor_explicit, volatile atomic_uint64 *                                                       \
              : _atomic_uint64_fetch_xor_explicit )

#define atomic_fetch_xor( dst, src )                 _atomic_fetch_xor_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_fetch_xor_explicit( dst, src, order ) _atomic_fetch_xor_f( ( dst ) )( ( dst ), ( src ), ( order ) )

    //------------------------------------------------------------------------------

#define _atomic_fetch_add_f( ptr )                                                                                               \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_fetch_add_explicit, volatile atomic_uint8 *                                                         \
              : _atomic_uint8_fetch_add_explicit, volatile atomic_int16 *                                                        \
              : _atomic_int16_fetch_add_explicit, volatile atomic_uint16 *                                                       \
              : _atomic_uint16_fetch_add_explicit, volatile atomic_int32 *                                                       \
              : _atomic_int32_fetch_add_explicit, volatile atomic_uint32 *                                                       \
              : _atomic_uint32_fetch_add_explicit, volatile atomic_int64 *                                                       \
              : _atomic_int64_fetch_add_explicit, volatile atomic_uint64 *                                                       \
              : _atomic_uint64_fetch_add_explicit )

#define atomic_fetch_add( dst, src )                 _atomic_fetch_add_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_fetch_add_explicit( dst, src, order ) _atomic_fetch_add_f( ( dst ) )( ( dst ), ( src ), ( order ) )

    //------------------------------------------------------------------------------

#define _atomic_fetch_sub_f( ptr )                                                                                               \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_fetch_sub_explicit, volatile atomic_uint8 *                                                         \
              : _atomic_uint8_fetch_sub_explicit, volatile atomic_int16 *                                                        \
              : _atomic_int16_fetch_sub_explicit, volatile atomic_uint16 *                                                       \
              : _atomic_uint16_fetch_sub_explicit, volatile atomic_int32 *                                                       \
              : _atomic_int32_fetch_sub_explicit, volatile atomic_uint32 *                                                       \
              : _atomic_uint32_fetch_sub_explicit, volatile atomic_int64 *                                                       \
              : _atomic_int64_fetch_sub_explicit, volatile atomic_uint64 *                                                       \
              : _atomic_uint64_fetch_sub_explicit )

#define atomic_fetch_sub( dst, src )                 _atomic_fetch_sub_f( ( dst ) )( ( dst ), ( src ), memory_order_seq_cst )
#define atomic_fetch_sub_explicit( dst, src, order ) _atomic_fetch_sub_f( ( dst ) )( ( dst ), ( src ), ( order ) )

    //------------------------------------------------------------------------------

#define _atomic_compare_exchange_f( ptr )                                                                                        \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_compare_exchange_strong_explicit, volatile atomic_uint8 *                                           \
              : _atomic_uint8_compare_exchange_strong_explicit, volatile atomic_int16 *                                          \
              : _atomic_int16_compare_exchange_strong_explicit, volatile atomic_uint16 *                                         \
              : _atomic_uint16_compare_exchange_strong_explicit, volatile atomic_int32 *                                         \
              : _atomic_int32_compare_exchange_strong_explicit, volatile atomic_uint32 *                                         \
              : _atomic_uint32_compare_exchange_strong_explicit, volatile atomic_int64 *                                         \
              : _atomic_int64_compare_exchange_strong_explicit, volatile atomic_uint64 *                                         \
              : _atomic_uint64_compare_exchange_strong_explicit )

#define atomic_compare_exchange_strong( dst, expected, desired )                                                                 \
    _atomic_compare_exchange_f( ( dst ) )( ( dst ), ( expected ), ( desired ), memory_order_seq_cst, memory_order_seq_cst )
#define atomic_compare_exchange_strong_explicit( dst, expected, desired, success_order, failure_order )                          \
    _atomic_compare_exchange_f( ( dst ) )( ( dst ), ( expected ), ( desired ), ( success_order ), ( failure_order ) )
#define atomic_compare_exchange_weak( dst, expected, desired )                                                                   \
    _atomic_compare_exchange_f( ( dst ) )( ( dst ), ( expected ), ( desired ), memory_order_seq_cst, memory_order_seq_cst )
#define atomic_compare_exchange_weak_explicit( dst, expected, desired, success_order, failure_order )                            \
    _atomic_compare_exchange_f( ( dst ) )( ( dst ), ( expected ), ( desired ), ( success_order ), ( failure_order ) )

    //------------------------------------------------------------------------------

    typedef atomic_uint8 atomic_flag;

#define atomic_flag_test_and_set_explicit( ptr, order ) _atomic_uint8_exchange_explicit( ( ptr ), 1, ( order ) )
#define atomic_flag_clear_explicit( ptr, order )        _atomic_uint8_exchange_explicit( ( ptr ), 0, ( order ) )

#define atomic_flag_test_and_set( ptr ) _atomic_uint8_exchange_explicit( ( ptr ), 1, memory_order_seq_cst )
#define atomic_flag_clear( ptr )        _atomic_uint8_exchange_explicit( ( ptr ), 0, memory_order_seq_cst )

    //------------------------------------------------------------------------------

    static SM_ATOMIC_INLINE void atomic_thread_fence( enum memory_order order )
    {
        (void) order;
        // On Intel processors sfense is 3x faster than __faststorefence()
        // On AMD they are approximately equal
        _mm_sfence();
    }

    static SM_ATOMIC_INLINE void atomic_signal_fence( enum memory_order order ) { _ReadWriteBarrier(); }

#define atomic_is_lock_free( ptr )                                                                                               \
    _Generic( ( ptr ), volatile atomic_int8 *                                                                                    \
              : _atomic_int8_is_lock_free, volatile atomic_uint8 *                                                               \
              : _atomic_uint8_is_lock_free, volatile atomic_int16 *                                                              \
              : _atomic_int16_is_lock_free, volatile atomic_uint16 *                                                             \
              : _atomic_uint16_is_lock_free, volatile atomic_int32 *                                                             \
              : _atomic_int32_is_lock_free, volatile atomic_uint32 *                                                             \
              : _atomic_uint32_is_lock_free, volatile atomic_int64 *                                                             \
              : _atomic_int64_is_lock_free, volatile atomic_uint64 *                                                             \
              : _atomic_uint64_is_lock_free )( ( ptr ) )

#if defined( __cplusplus )
}
#endif

#else
#include <stdatomic.h>
#endif