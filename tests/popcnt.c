// cl /W4 /O2 popcnt.c
#include <intrin.h>
#include <stdint.h>
#include <stdio.h>

static inline int
sw_popcount32( uint32_t x )
{
    x -= ( x >> 1 ) & 0x55555555u;
    x = ( x & 0x33333333u ) + ( ( x >> 2 ) & 0x33333333u );
    x = ( x + ( x >> 4 ) ) & 0x0F0F0F0Fu;
    return (int) ( ( x * 0x01010101u ) >> 24 );
}

static inline int
hw_popcount32( uint32_t x )
{
    return (int) __popcnt( x );
}

void
main()
{
    for( size_t i = 0; i <= 0xFFFFFFFF; i++ )
    {
        if( sw_popcount32( i ) != hw_popcount32( i ) )
        {
            printf( "popcounts differ at %z\n", i );
            exit( 1 );
        }
    }
    printf( "ok\n" );
}