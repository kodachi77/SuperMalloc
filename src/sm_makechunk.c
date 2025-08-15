#include <errno.h>
#include <stdlib.h>
#ifdef __linux__
#include <sys/mman.h>
#endif
#ifdef _WIN64
#include "sm_platform.h"
#endif

#include "generated_constants.hxx"
#include "sm_assert.h"
#include "sm_internal.h"

#ifdef TESTING
#include <string.h>
#endif

// Nothing in this file seems to need locking.  We rely on the thread safety of mmap and munmap.

static _Atomic( size_t ) total_mapped          = 0;
static _Atomic( size_t ) mismapped_so_unmapped = 0;

void*
mmap_size( size_t size )
{
#ifdef __linux__
    void* r = mmap( NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON | MAP_NORESERVE, -1, 0 );
    if( r == MAP_FAILED )
    {
        fprintf( stderr, " Total mapped so far = %lu unmapped=%lu, size = %ld\n", total_mapped, mismapped_so_unmapped, size );
        perror( "Map failed" );
    }
    if( r == MAP_FAILED ) return NULL;
    atomic_fetch_add( &total_mapped, size );
    return r;

    // This is the old code...
    SM_ASSERT( r != NULL );
    if( r == MAP_FAILED )
    {
        fprintf( stderr, " Total mapped so far = %ld, size = %ld\n", total_mapped, size );
        perror( "Map failed" );
    }
    SM_ASSERT( r != MAP_FAILED );
#elif defined( _WIN64 )
    DWORD flag = 0;
    // FIXME: if( size > ::GetLargePageMinimum() ) flag |= MEM_LARGE_PAGES; /* this causes some issues in madvise */
    void* r = VirtualAlloc( NULL, size, flag | MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE );
    if( !r )
    {
        SM_LOG_FATAL( " VirtualAlloc failed. Error = %d\n", GetLastError() );
        return NULL;
    }

    atomic_fetch_add( &total_mapped, size );

#endif
    return r;
}

static void
unmap( void* p, size_t size )
{
#ifdef __linux__
    if( size > 0 )
    {
        int r = munmap( p, size );
        if( r != 0 )
        {
            fprintf( stderr, "Failure doing munmap(%p, %ld) error=%d\n", p, size, errno );
            abort();
        }
        SM_ASSERT( r == 0 );
        atomic_fetch_add( &mismapped_so_unmapped, size );
    }
#elif defined( _WIN64 )
    if( size > 0 )
    {
        MEMORY_BASIC_INFORMATION minfo;
        char*                    cptr = (char*) p;
        while( size )
        {
            if( VirtualQuery( cptr, &minfo, sizeof( minfo ) ) == 0 ) return;
            if( minfo.BaseAddress != cptr || minfo.AllocationBase != cptr || minfo.State != MEM_COMMIT
                || minfo.RegionSize > size )
                return;

            if( VirtualFree( cptr, 0, MEM_RELEASE ) == 0 ) return;
            cptr += minfo.RegionSize;
            size -= minfo.RegionSize;
        }

        atomic_fetch_add( &mismapped_so_unmapped, size );
    }
#endif
}

#if defined( _WIN64 )
void*
mmap_allocate_space( size_t size )
{
    DWORD flag = 0;
    // FIXME: if( size > ::GetLargePageMinimum() ) flag |= MEM_LARGE_PAGES; /* this causes some issues in madvise */
    void* r = VirtualAlloc( NULL, size, flag | MEM_RESERVE, PAGE_READWRITE );
    if( !r )
    {
        SM_LOG_FATAL( " VirtualAlloc failed. Error = %d\n", GetLastError() );
        return NULL;
    }

    //atomic_fetch_add( &total_mapped, size );

    return r;
}

void
mmap_commit_page( void* ptr, size_t size )
{
    // VirtualAlloc cannot reserve a reserved page. It can commit a page that is already committed.
    void* r = VirtualAlloc( ptr, size, MEM_COMMIT, PAGE_READWRITE );
    if( !r ) { SM_LOG_FATAL( " VirtualAlloc failed. Error = %d\n", GetLastError() ); }
    //atomic_fetch_add( &total_mapped, size );
}

#endif

static void*
chunk_create_slow( size_t n_chunks )
{
    size_t total_size =
        ( 1 + n_chunks ) * chunksize;    // The original code mapped one page less than this, but it seemed to cause misalignment
    void* m = mmap_size( total_size );
    if( m == NULL ) return m;
    size_t m_offset = offset_in_chunk( m );
    if( m_offset == 0 )
    {
        unmap( (char*) m + n_chunks * chunksize, chunksize );
        return m;
    }
    else
    {
        size_t leading_useless = chunksize - m_offset;    // guaranteed to be non-negative
        unmap( m, leading_useless );
        void* final_m = (char*) m + leading_useless;
        unmap( (char*) final_m + n_chunks * chunksize, m_offset );
        return final_m;
    }
}

void*
mmap_chunk_aligned_block( size_t n_chunks )
// Effect: Return a pointer to a chunk.
//   The chunk is in a purged-stated (not in memory, zero-fill-on-demand).
//   The chunk might map as either a huge page or a lot of little
//    pages: the exact behavior isn't specified.  Use
//    madvise(MADV_HUGEPAGE) or MADV_NOHUGEPAGE to force the behavior you want.
//   We want this to be nonblocking, and we don't want to do a lot of crazy extra work if we
//    happen to create an extra chunk.
{
    // Quoting from jemalloc's chunk_mmap.c:
    /***
     * Ideally, there would be a way to specify alignment to mmap() (like
     * NetBSD has), but in the absence of such a feature, we have to work
     * hard to efficiently create aligned mappings.  The reliable, but
     * slow method is to create a mapping that is over-sized, then trim the
     * excess.  However, that always results in one or two calls to
     * pages_unmap().
     *
     * Optimistically try mapping precisely the right amount before falling
     * back to the slow method, with the expectation that the optimistic
     * approach works most of the time.
     */

    void* r = mmap_size( n_chunks * chunksize );
    if( r == 0 ) return NULL;
    if( offset_in_chunk( r ) != 0 )
    {
        // Do it the slow way.
        unmap( r, n_chunks * chunksize );
        return chunk_create_slow( n_chunks );
    }
    else
    {
        //printf("%s:%d returning %p\n", __FILE__, __LINE__, r);
        return r;
    }
}

void
test_makechunk( void )
{
    {
        void* v = mmap_size( 4096 );
        SM_ASSERT( v );
        unmap( v, 4096 );
        unmap( NULL, 0 );
    }
    {
        void* v = mmap_chunk_aligned_block( 1 );
        SM_ASSERT( v );
        SM_ASSERT( offset_in_chunk( v ) == 0 );
        unmap( v, 1 * chunksize );
    }

    // Test chunk_create_slow
    {
        void* v = chunk_create_slow( 3 );
        SM_ASSERT( v );
        SM_ASSERT( offset_in_chunk( v ) == 0 );
        void* w = chunk_create_slow( 3 );
        SM_ASSERT( w );
        SM_ASSERT( offset_in_chunk( w ) == 0 );
        unmap( v, 3 * chunksize );
        unmap( w, 3 * chunksize );
    }
    {
        void* p = mmap_size( 4096 );
        void* w = chunk_create_slow( 3 );
        SM_ASSERT( p );
        SM_ASSERT( w );
        SM_ASSERT( offset_in_chunk( w ) == 0 );
    }
    {
        void* p = mmap_size( chunksize - 4096 );
        void* w = chunk_create_slow( 3 );
        SM_ASSERT( p );
        SM_ASSERT( w );
        SM_ASSERT( offset_in_chunk( w ) == 0 );
    }
}
