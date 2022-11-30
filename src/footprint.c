/* Maintain a count of the footprint. */

#ifdef __linux__
#include <sched.h>
#endif
#include "sm_atomic.h"
#include "sm_config.h"

#include "atomically.h"
#include "malloc_internal.h"

enum
{
    // should be enough for now.
    processor_number_limit = 64
};

static uint64_t partitioned_footprint[processor_number_limit];

typedef struct processor_id
{
    SM_ALIGNED( 64 ) int cpuid;
    int count;
} processor_id;

static const int                        prid_cache_time = 128;
static SM_ATTRIBUTE_THREAD processor_id prid;

static inline void
check_cpuid( void )
{
    if( prid.count++ % prid_cache_time == 0 ) { prid.cpuid = sched_getcpu() % processor_number_limit; }
}

void
add_to_footprint( int64_t delta )
{
    check_cpuid();
    atomic_fetch_add( (_Atomic uint64_t*) &partitioned_footprint[prid.cpuid], delta );
}

int64_t
get_footprint( void )
{
    int64_t sum = 0;
    for( int i = 0; i < processor_number_limit; i++ )
    {
        // don't really care if we slightly stale answers.
        // so no atomic_load
        sum += partitioned_footprint[i];
    }
    return sum;
}
