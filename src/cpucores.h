#ifndef CPUCORES_H
#define CPUCORES_H

static inline uint32_t
cpucores( void )
{
    SYSTEM_INFO si;
    GetSystemInfo( &si );
    return si.dwNumberOfProcessors;
}

#endif /* CPUCORES_H */
