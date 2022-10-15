#ifndef BMALLOC_H
#define BMALLOC_H

#include <sys/types.h>

#include "config.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void* super_malloc( size_t /*size*/ ) __THROW ATTRIBUTE_MALLOC;
    void* super_calloc( size_t /*number*/, size_t /*size*/ ) __THROW ATTRIBUTE_MALLOC;
    void  super_free( void* /*ptr*/ ) __THROW;
    void* super_aligned_alloc( size_t /*alignment*/, size_t /*size*/ ) __THROW;
    int   super_posix_memalign( void** memptr, size_t alignment, size_t size ) __THROW;
    void* super_memalign( size_t alignment, size_t size ) __THROW;
    void* super_realloc( void* p, size_t size ) __THROW;

    // non_standard API
    size_t super_malloc_usable_size( const void* ptr );

#ifdef __cplusplus
}
#endif

#endif
