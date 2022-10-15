#include "rng.h"

#include "config.h"

// This is copied out of Dave Dice's web log
//  https://blogs.oracle.com/dave/entry/a_simple_prng_idiom

static const uint64_t MIX = 0xdaba0b6eb09322e3ull;

static uint64_t
Mix64( uint64_t z )
{
    z = ( z ^ ( z >> 32 ) ) * MIX;
    z = ( z ^ ( z >> 32 ) ) * MIX;
    return z ^ ( z >> 32 );
}

uint64_t
prandnum()
{
    // Instead of incrementing by 1 we could also increment by a large
    // prime or by the MIX constant.

    // One clever idea from Dave Dice: We use the address of the
    // __thread variable as part of the hashed input, so that our random
    // number generator doesn't need to be initialized.

    // Make this aligned(64) so that we don't get any false sharing.
    // Perhaps __thread variables already avoid false sharing, but I
    // don't want to verify that.
    static ATTRIBUTE_THREAD uint64_t ATTRIBUTE_ALIGNED( 64 ) rv = 0;
    return Mix64( reinterpret_cast<int64_t>( &rv ) + ( ++rv ) );
}
