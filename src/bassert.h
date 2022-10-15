#ifndef BASSERT_H
#define BASSERT_H

#include <stdio.h>
#include <stdlib.h>

#include "config.h"

static inline void
bassert_f( bool test, const char* pred, const char* file, int line, const char* fun )
{
    if( !test )
    {
        fprintf( stderr, "assertion failed: %s in %s at %s:%d\n", pred, fun, file, line );
        abort();
        ;
    }
}

#define bassert( e ) bassert_f( ( e ) != 0, #e, __FILE__, __LINE__, __FUNCTION__ )
#ifdef assert
#warning assert is deprecated, use bassert
#else
#define assert dont_use_assert_do_use_bassert
#endif

// A couple more functions to remove never-executing branches out of the main code.
static bool OR( bool a, bool b );
static bool
OR( bool a, bool b )
{
    return a || b;
}
static bool AND( bool a, bool b );
static bool
AND( bool a, bool b )
{
    return a && b;
}

#endif /* BASSERT_H */
