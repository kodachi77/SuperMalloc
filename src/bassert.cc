#include "bassert.h"
#include "config.h"

#include <stdarg.h>
#include <stdio.h>

//#if defined( __linux__ )
#include <stdio.h>
#include <stdlib.h>
//#elif defined( _WIN64 )
//#include <strsafe.h>
//#include <windows.h>
//#endif
#ifdef __cplusplus
extern "C"
{
#endif

    void
sm_assert_internal( const char* predicate, const char* file, unsigned int line, const char* function )
{
//#if defined( __linux__ )
    fprintf( stderr, "ASSERTION FAILED (%s:%u) %s in \"%s\".", file, line, predicate, function );
    abort();
/*
#elif defined( _WIN64 )
    char    assert_msg[256] = { 0 };
    ::StringCbPrintfA( assert_msg, 256, "ASSERTION FAILED (%s:%u) %s in \"%s\".", file, line, predicate, function );
    ::OutputDebugStringA( assert_msg );
    if( ::IsDebuggerPresent() ) 
    { 
        ::DebugBreak(); 
    }
    else
    {
        ::ExitProcess( 13 );
    }
#endif
*/
}

    #ifdef __cplusplus
}
#endif