#include "unit-tests.h"

void
run_tests()
{
    initialize_malloc();

    test_cache_early();

    test_hyperceil();
    test_size_2_bin();
    test_makechunk();
    test_huge_malloc();
    test_large_malloc();
    test_small_malloc();
    test_realloc();
    test_malloc_usable_size();
    test_object_base();

    time_small_malloc();
}

int
main( int, char** )
{
    run_tests();
    return 0;
}
