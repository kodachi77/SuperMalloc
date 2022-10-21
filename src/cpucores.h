#ifndef CPUCORES_H
#define CPUCORES_H

#if defined( __linux__ )

#if defined( HAVE_CPUCORES_SYSCONF )
#include <unistd.h>

#elif defined( HAVE_CPUCORES_SYSCTL )
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#endif

static inline uint32_t
cpucores( void )
{
    uint32_t ret;
#if defined( HAVE_CPUCORES_SYSCONF )
    const long cpus = sysconf( _SC_NPROCESSORS_ONLN );
    if( cpus > 0 ) ret = static_cast<uint32_t>( cpus );

#elif defined( HAVE_CPUCORES_SYSCTL )
    int    name[2] = { CTL_HW, HW_NCPU };
    int    cpus;
    size_t cpus_size = sizeof( cpus );
    if( !sysctl( name, &cpus, &cpus_size, NULL, NULL ) && cpus_size == sizeof( cpus ) && cpus > 0 )
        ret = static_cast<uint32_t>( cpus );
#endif

    return ret;
}

#elif defined( _WIN64 )

#include "config.h"

static inline uint32_t
cpucores( void )
{
    SYSTEM_INFO si;
    ::GetSystemInfo( &si );
    return si.dwNumberOfProcessors;
}

#endif
#endif /* CPUCORES_H */
