#pragma once

#include <stdatomic.h>

typedef _Atomic( uintptr_t ) atomic_ptr;

#define atomic_test_and_set( dst )               atomic_exchange_explicit( ( dst ), 1, memory_order_seq_cst )
#define atomic_clear( dst )                      atomic_exchange_explicit( ( dst ), 0, memory_order_seq_cst )
#define atomic_compare_and_swap( dst, exp, des ) atomic_compare_exchange_strong( ( dst ), ( exp ), ( des ) )

//atomic_compare_exchange_strong_explicit( ( dst ), ( exp ), ( des ), memory_order_seq_cst )
