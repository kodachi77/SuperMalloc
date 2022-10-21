#pragma once

//#include <sys/types.h>
#include "config.h"

#if defined (__cplusplus)
extern "C"
{
#endif

    void* sm_malloc( size_t size ) __THROW;
    void* sm_calloc( size_t number, size_t size ) __THROW;
    void  sm_free( void* ptr ) __THROW;
    void* sm_aligned_alloc( size_t alignment, size_t size ) __THROW;
    int   sm_posix_memalign( void** memptr, size_t alignment, size_t size ) __THROW;
    void* sm_memalign( size_t alignment, size_t size ) __THROW;
    void* sm_realloc( void* p, size_t size ) __THROW;

    /* non-standard API */
    size_t sm_malloc_usable_size( const void* ptr );

#ifdef __cplusplus
}
#endif

