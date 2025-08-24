#ifdef TESTING
#include <stdio.h>
#endif

#ifdef __linux__
#include <sys/mman.h>
#endif

#include "atomically.h"
#include "generated_constants.hxx"
#include "sm_assert.h"
#include "sm_internal.h"

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

static void
add_to_footprint( int64_t delta )
{
    check_cpuid();
    atomic_fetch_add( (_Atomic uint64_t*) &partitioned_footprint[prid.cpuid], delta );
}

static int64_t
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

enum
{
    n_large_classes = first_huge_bin_number - first_large_bin_number
};

static large_object_list_cell* free_large_objects
    [n_large_classes];    // For each large size, a list (threaded through the chunk headers) of all the free objects of that size.
// Later we'll be a little careful about purging those large objects (and we'll need to remember which are which, but we may also want thread-specific parts).  For now, just purge them all.

static lock_t large_lock = SM_LOCK_INITIALIZER;

large_object_list_cell*
do_large_malloc_pop( large_object_list_cell** free_head )
{
    large_object_list_cell* h = *free_head;
    SM_LOG_DEBUG( " dlmp: h=%p\n", h );
    if( h == NULL ) { return NULL; }
    else
    {
        *free_head = h->next;
        return h;
    }
}

SM_DECLARE_ATOMIC_OPERATION( large_malloc_pop, do_large_malloc_pop, large_object_list_cell*, large_object_list_cell** );

void
init_large_malloc()
{
    initialize_lock_array( &large_lock, 1 );
}

void*
large_malloc( size_t size )
// Effect: Allocate a large object (page allocated, multiple per chunk)
// Implementation notes: Since it is page allocated, any page is as
//  good as any other.  (Although perhaps someday we ought to prefer
//  to fullest chunk and arrange that full-enough chunks use
//  hugepages.  But for now it doesn't matter which page we use, since
//  for these pages we disable hugepages.)
{
    SM_LOG_DEBUG( "large_malloc(%" PRIu64 "):\n", size );
    uint32_t    footprint   = (uint32_t) ( pagesize * ceil64( size, pagesize ) );
    binnumber_t b           = size_2_bin( size );
    size_t      usable_size = bin_2_size( b );
    SM_ASSERT( b >= first_large_bin_number );
    SM_ASSERT( b < first_huge_bin_number );

    large_object_list_cell** free_head = &free_large_objects[b - first_large_bin_number];

    while( 1 )
    {    // Keep going until we find a free object and return it.

        // This needs to be done atomically (along the successful branch).
        // It cannot be done with a compare-and-swap since we read two locations that
        // are visible to other threads (getting h, and getting h->next).
        large_object_list_cell* h = *free_head;
        SM_LOG_DEBUG( "h==%p\n", h );
        if( h != NULL )
        {
            if( 0 )
            {
                // This is what we want the atomic code below to do.
                // The atomic code will have to re-read *free_head to get h again.
                *free_head = h->next;
            }
            else
            {
                // The strategy for the atomic version is that we set e.result to NULL if the list
                // becomes empty (so that we go around and do chunk allocation again).

                h = SM_INVOKE_ATOMIC_OPERATION( &large_lock, large_malloc_pop, free_head );
                if( h == NULL ) continue;    // Go try again
            }
            // that was the atomic part.
            h->footprint = footprint;
            add_to_footprint( footprint );
            SM_LOG_DEBUG( "setting its footprint to %d\n", h->footprint );
            SM_LOG_DEBUG( "returning the page corresponding to %p\n", h );
            void* chunk = address_2_chunkaddress( h );
            SM_LOG_DEBUG( "chunk=%p\n", chunk );
            large_object_list_cell* chunk_as_list_cell = chunk;
            size_t                  offset             = h - chunk_as_list_cell;
            SM_LOG_DEBUG( "offset=%" PRIu64 "\n", offset );
            void* address = (char*) chunk + offset_of_first_object_in_large_chunk + offset * usable_size;
            SM_ASSERT( address_2_chunknumber( address ) == address_2_chunknumber( chunk ) );
            SM_LOG_DEBUG( "result=%p\n", address );
            SM_ASSERT( bin_from_bin_and_size( chunk_infos[address_2_chunknumber( address )].bin_and_size ) == b );
            return address;
        }
        else
        {
            // No already free objects.  Get a chunk
            void* chunk = mmap_chunk_aligned_block( 1 );
            SM_ASSERT( chunk );
            SM_LOG_DEBUG( "chunk=%p\n", chunk );

            SM_LOG_DEBUG( "usable_size=%" PRIu64 "\n", usable_size );
            size_t objects_per_chunk =
                ( chunksize - offset_of_first_object_in_large_chunk )
                / usable_size;    // Should use magic for this, but it's already got an mmap(), so it doesn't matter
            SM_LOG_DEBUG( "opce=%" PRIu64 "\n", objects_per_chunk );
            size_t size_of_header = objects_per_chunk * sizeof( large_object_list_cell );
            SM_LOG_DEBUG( "soh=%" PRIu64 "\n", size_of_header );
            SM_ASSERT( size_of_header <= offset_of_first_object_in_large_chunk );

            large_object_list_cell* entry = (large_object_list_cell*) chunk;
            for( size_t i = 0; i + 1 < objects_per_chunk; i++ ) { entry[i].next = &entry[i + 1]; }

            bin_and_size_t b_and_s = bin_and_size_to_bin_and_size( b, footprint );
            SM_ASSERT( b_and_s != 0 );
            chunknumber_t chunknum = address_2_chunknumber( chunk );
            commit_ci_page_as_needed( chunknum );
            chunk_infos[chunknum].bin_and_size = b_and_s;

            // Do this atomically.
            if( 0 )
            {
                entry[objects_per_chunk - 1].next = *free_head;
                *free_head                        = &entry[0];
            }
            else
            {
                while( 1 )
                {
                    large_object_list_cell* old_first = *free_head;
                    entry[objects_per_chunk - 1].next = old_first;
                    if( atomic_compare_and_swap( (atomic_ptr*) free_head, &old_first, &entry[0] ) ) break;    //> added &
                }
            }

            SM_LOG_DEBUG( "Got object\n" );
        }
    }
}

size_t
large_footprint( void* p )
{
    SM_LOG_DEBUG( "large_footprint(%p):\n", p );
    bin_and_size_t b_and_s = chunk_infos[address_2_chunknumber( p )].bin_and_size;
    SM_ASSERT( b_and_s != 0 );
    binnumber_t bin = bin_from_bin_and_size( b_and_s );
    SM_ASSERT( first_large_bin_number <= bin );
    SM_ASSERT( bin < first_huge_bin_number );
    uint64_t usable_size = bin_2_size( bin );
    uint64_t offset      = offset_in_chunk( p );
    uint64_t objnum      = ( offset - offset_of_first_object_in_large_chunk ) / usable_size;
    SM_LOG_DEBUG( "objnum %p is in bin %d, usable_size=%" PRIu64 ", objnum=%" PRIu64 "\n", p, bin, usable_size, objnum );
    large_object_list_cell* entries = address_2_chunkaddress( p );

    SM_LOG_DEBUG( "entries are %p\n", entries );
    uint32_t footprint = entries[objnum].footprint;
    SM_LOG_DEBUG( "footprint=%u\n", footprint );
    return footprint;
}

void
large_free( void* p )
{
    bin_and_size_t b_and_s = chunk_infos[address_2_chunknumber( p )].bin_and_size;
    SM_ASSERT( b_and_s != 0 );
    binnumber_t bin = bin_from_bin_and_size( b_and_s );
    SM_ASSERT( first_large_bin_number <= bin && bin < first_huge_bin_number );
    uint64_t usable_size = bin_2_size( bin );

    madvise( p, usable_size, MADV_DONTNEED );

    uint64_t offset = offset_in_chunk( p );
    uint64_t objnum = divide_offset_by_objsize( (uint32_t) ( offset - offset_of_first_object_in_large_chunk ), bin );
    if( IS_TESTING )
    {
        uint64_t objnum2 = ( offset - offset_of_first_object_in_large_chunk ) / usable_size;
        SM_ASSERT( objnum == objnum2 );
    }
    large_object_list_cell* entries   = (large_object_list_cell*) address_2_chunkaddress( p );
    uint32_t                footprint = entries[objnum].footprint;
    add_to_footprint( -(int32_t) footprint );
    large_object_list_cell** h  = &free_large_objects[bin - first_large_bin_number];
    large_object_list_cell*  ei = entries + objnum;
    // This part atomic. Can be done with compare_and_swap
    if( 0 )
    {
        ei->next = *h;
        *h       = ei;
    }
    else
    {
        while( 1 )
        {
            large_object_list_cell* first = atomic_load( (atomic_ptr*) h );
            ei->next                      = first;
            if( atomic_compare_and_swap( (atomic_ptr*) h, &first, ei ) ) break;    //> added &
        }
    }
}

void
test_large_malloc( void )
{
    size_t  msize = 4 * pagesize;
    int64_t fp    = get_footprint();
    {
        void* x = large_malloc( msize );
        SM_ASSERT( x );
        SM_ASSERT( offset_in_chunk( x ) == offset_of_first_object_in_large_chunk );

        void* y = large_malloc( msize );
        SM_ASSERT( y );
        SM_ASSERT( offset_in_chunk( y ) == offset_of_first_object_in_large_chunk + msize );

        size_t fy = large_footprint( y );
        SM_ASSERT( fy == msize );

        SM_ASSERT( get_footprint() - fp == (int64_t) ( 2 * msize ) );

        large_free( x );
        void* z = large_malloc( msize );
        SM_ASSERT( z == x );

        large_free( z );
        large_free( y );
    }

    SM_ASSERT( get_footprint() - fp == 0 );
    {
        void* x = large_malloc( 2 * msize );
        SM_ASSERT( x );
        SM_ASSERT( offset_in_chunk( x ) == offset_of_first_object_in_large_chunk + 0 * msize );

        SM_ASSERT( get_footprint() - fp == (int64_t) ( 2 * msize ) );

        void* y = large_malloc( 2 * msize );
        SM_ASSERT( y );
        SM_ASSERT( offset_in_chunk( y ) == offset_of_first_object_in_large_chunk + 2 * msize );

        SM_ASSERT( get_footprint() - fp == (int64_t) ( 4 * msize ) );

        large_free( x );
        void* z = large_malloc( 2 * msize );
        SM_ASSERT( z == x );

        large_free( z );
        large_free( y );
    }
    SM_ASSERT( get_footprint() - fp == 0 );
    {
        void* x = large_malloc( largest_large );
        SM_ASSERT( x );
        SM_ASSERT( offset_in_chunk( x ) == offset_of_first_object_in_large_chunk + 0 * largest_large );

        void* y = large_malloc( largest_large );
        SM_ASSERT( y );
        SM_ASSERT( offset_in_chunk( y ) == offset_of_first_object_in_large_chunk + 1 * largest_large );

        large_free( x );
        void* z = large_malloc( largest_large );
        SM_ASSERT( z == x );

        large_free( z );
        large_free( y );
    }
    SM_ASSERT( get_footprint() - fp == 0 );
    {
        size_t s = 100 * 4096;
        SM_LOG_DEBUG( "s=%" PRIu64 "\n", s );

        void* x = large_malloc( s );
        SM_ASSERT( x );
        SM_ASSERT( offset_in_chunk( x ) == offset_of_first_object_in_large_chunk + 0 * msize );

        SM_ASSERT( large_footprint( x ) == s );

        SM_ASSERT( get_footprint() - fp == (int64_t) s );

        void* y = large_malloc( s );
        SM_ASSERT( y );
        SM_ASSERT( offset_in_chunk( y ) == offset_of_first_object_in_large_chunk + bin_2_size( size_2_bin( s ) ) );

        SM_ASSERT( large_footprint( y ) == s );

        large_free( x );
        large_free( y );
    }
    SM_ASSERT( get_footprint() - fp == 0 );
}
