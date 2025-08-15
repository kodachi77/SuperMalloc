#include "sm_assert.h"
#include "sm_config.h"

#if defined( SM_PLATFORM_LINUX )
#include <stdio.h>
#include <stdlib.h>
#elif defined( SM_PLATFORM_WINDOWS )
#include <strsafe.h>
#include "_windows.h"
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void
sm_assert_internal( const char* predicate, const char* file, unsigned int line, const char* function )
{
#if defined( SM_PLATFORM_LINUX )
    fprintf( stderr, "ASSERTION FAILED (%s:%u) %s in \"%s\".", file, line, predicate, function );
    abort();
#elif defined( SM_PLATFORM_WINDOWS )
    char assert_msg[256];
    StringCbPrintfA( assert_msg, 256, "ASSERTION FAILED (%s:%u) %s in \"%s\".", file, line, predicate, function );
    OutputDebugStringA( assert_msg );
    //> TODO: different debug break to not aggravate antivurus software
    if( IsDebuggerPresent() ) { DebugBreak(); }
    else { ExitProcess( 13 ); }
#endif
}

#ifdef __cplusplus
}
#endif