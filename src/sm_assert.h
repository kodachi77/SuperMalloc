#pragma once

#include <stdbool.h>
#include "sm_config.h"

#if defined( SM_ASSERTS_ENABLED )
#if !defined( SM_ASSERT )
#define SM_ASSERT( predicate ) ( ( predicate ) || ( sm_assert_internal( #predicate, __FILE__, __LINE__, __FUNCTION__ ), 0 ) )
#endif

#if defined( assert )
//>#warning Use of 'assert' is not recommended, use 'SM_ASSERT' instead.
#else
#define assert dont_use_crt_assert__use_SM_ASSERT
#endif

#ifdef __cplusplus
extern "C"
{
#endif

void sm_assert_internal( const char* predicate, const char* file, unsigned int line, const char* function );

#ifdef __cplusplus
}
#endif

#else
#define SM_ASSERT
#endif

// Helper functions to use for more complex predicates in assert statements.
static bool
OR( bool a, bool b )
{
    return a || b;
}

static bool
AND( bool a, bool b )
{
    return a && b;
}
