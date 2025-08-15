#ifdef ENABLE_STATS

#include "atomically.h"
#include "generated_constants.h"
#include "sm_internal.h"

struct bin_stats
{
    uint64_t n_mallocs, net_n_mallocs, highwater_mark;
};

static struct stats
{
    bin_stats b[first_huge_bin_number + 1];    // the last one is all the huge bin stats
} stats;

void
bin_stats_note_malloc( binnumber_t b )
{
    b = min( first_huge_bin_number, b );
    atomic_fetch_add( &stats.b[b].n_mallocs, 1 );
    uint64_t net = atomic_fetch_add( &stats.b[b].net_n_mallocs, 1 );
    fetch_and_max( &stats.b[b].highwater_mark, net );
}

void
bin_stats_note_free( binnumber_t b )
{
    b = min( first_huge_bin_number, b );
    atomic_fetch_add( &stats.b[b].net_n_mallocs, -1 );
}

void
print_bin_stats()
{
    printf( "bin: n_mallocs net_n_mallocs highwater\n" );
    for( binnumber_t i = 0; i <= first_huge_bin_number; i++ )
    {
        printf( "%d: %lu %lu %lu\n", i, stats.b[i].n_mallocs, stats.b[i].net_n_mallocs, stats.b[i].highwater_mark );
    }
}

#endif
