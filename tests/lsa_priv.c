// cl /W4 /O2 lsa_piv.c

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <ntsecapi.h>    // LSA
#include <sddl.h>        // ConvertSidToStringSidW
#include <stdio.h>
#include <wchar.h>
#include <windows.h>

#pragma comment( lib, "Advapi32.lib" )

static const wchar_t* kRight = L"SeLockMemoryPrivilege";

static void
print_winerr( const wchar_t* msg, DWORD err )
{
    LPWSTR buf = NULL;
    FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err, 0,
                    (LPWSTR) &buf, 0, NULL );
    fwprintf( stderr, L"%s (err=%lu)%s%s", msg, err, buf ? L": " : L"", buf ? buf : L"\n" );
    if( buf ) LocalFree( buf );
}

static BOOL
is_elevated_admin( void )
{
    HANDLE token = NULL;
    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ) ) return FALSE;
    TOKEN_ELEVATION elev   = { 0 };
    DWORD           retLen = 0;
    BOOL            ok     = GetTokenInformation( token, TokenElevation, &elev, sizeof( elev ), &retLen );
    CloseHandle( token );
    return ok && elev.TokenIsElevated;
}

static BOOL
get_current_user_sid( PSID* outSid )
{
    *outSid      = NULL;
    HANDLE token = NULL;
    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token ) ) return FALSE;

    DWORD needed = 0;
    GetTokenInformation( token, TokenUser, NULL, 0, &needed );
    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER )
    {
        CloseHandle( token );
        return FALSE;
    }
    PTOKEN_USER tu = (PTOKEN_USER) LocalAlloc( LMEM_FIXED, needed );
    if( !tu )
    {
        CloseHandle( token );
        return FALSE;
    }

    BOOL ok = GetTokenInformation( token, TokenUser, tu, needed, &needed );
    CloseHandle( token );
    if( !ok )
    {
        LocalFree( tu );
        return FALSE;
    }

    DWORD sidLen = GetLengthSid( tu->User.Sid );
    PSID  sid    = (PSID) LocalAlloc( LMEM_FIXED, sidLen );
    if( !sid )
    {
        LocalFree( tu );
        return FALSE;
    }
    if( !CopySid( sidLen, sid, tu->User.Sid ) )
    {
        LocalFree( sid );
        LocalFree( tu );
        return FALSE;
    }
    LocalFree( tu );
    *outSid = sid;
    return TRUE;
}

static BOOL
resolve_account_to_sid( const wchar_t* account, PSID* outSid )
{
    *outSid = NULL;
    if( !account || !account[0] ) return FALSE;

    DWORD        sidSize = 0, domSize = 0;
    SID_NAME_USE use;
    LookupAccountNameW( NULL, account, NULL, &sidSize, NULL, &domSize, &use );
    DWORD err = GetLastError();
    if( err != ERROR_INSUFFICIENT_BUFFER )
    {
        print_winerr( L"LookupAccountNameW (size query) failed", err );
        return FALSE;
    }
    PSID     sid    = (PSID) LocalAlloc( LMEM_FIXED, sidSize );
    wchar_t* domain = (wchar_t*) LocalAlloc( LMEM_FIXED, domSize * sizeof( wchar_t ) );
    if( !sid || !domain )
    {
        if( sid ) LocalFree( sid );
        if( domain ) LocalFree( domain );
        fwprintf( stderr, L"Out of memory.\n" );
        return FALSE;
    }
    if( !LookupAccountNameW( NULL, account, sid, &sidSize, domain, &domSize, &use ) )
    {
        err = GetLastError();
        print_winerr( L"LookupAccountNameW failed", err );
        LocalFree( sid );
        LocalFree( domain );
        return FALSE;
    }
    LocalFree( domain );
    *outSid = sid;
    return TRUE;
}

static void
usage( const wchar_t* exe )
{
    wprintf( L"Usage:\n" );
    wprintf( L"  %ls                 # add SeLockMemoryPrivilege to CURRENT user\n", exe );
    wprintf( L"  %ls DOMAIN\\User     # add privilege to specified account\n", exe );
    wprintf( L"  %ls --remove [acct] # remove privilege from current or specified account\n", exe );
    wprintf( L"  %ls --list   [acct] # list all rights for current or specified account\n", exe );
}

int
wmain( int argc, wchar_t** argv )
{
    if( !is_elevated_admin() )
    {
        fwprintf( stderr, L"Please run this program as Administrator (elevated).\n" );
        return 1;
    }

    enum
    {
        MODE_ADD,
        MODE_REMOVE,
        MODE_LIST
    } mode                    = MODE_ADD;
    const wchar_t* accountArg = NULL;

    // Parse args
    for( int i = 1; i < argc; i++ )
    {
        if( wcscmp( argv[i], L"--remove" ) == 0 ) { mode = MODE_REMOVE; }
        else if( wcscmp( argv[i], L"--list" ) == 0 ) { mode = MODE_LIST; }
        else if( wcscmp( argv[i], L"--help" ) == 0 || wcscmp( argv[i], L"-h" ) == 0 || wcscmp( argv[i], L"/?" ) == 0 )
        {
            usage( argv[0] );
            return 0;
        }
        else { accountArg = argv[i]; }
    }

    // Resolve SID
    PSID sid = NULL;
    if( accountArg )
    {
        if( !resolve_account_to_sid( accountArg, &sid ) ) return 1;
        wprintf( L"Target account: %ls\n", accountArg );
    }
    else
    {
        if( !get_current_user_sid( &sid ) )
        {
            print_winerr( L"Failed to get current user SID", GetLastError() );
            return 1;
        }
        wprintf( L"Target account: (current user)\n" );
    }

    // Show SID
    LPWSTR sidStr = NULL;
    if( ConvertSidToStringSidW( sid, &sidStr ) )
    {
        wprintf( L"SID           : %ls\n", sidStr );
        LocalFree( sidStr );
        sidStr = NULL;
    }

    // Open LSA Policy
    LSA_OBJECT_ATTRIBUTES oa;
    ZeroMemory( &oa, sizeof( oa ) );
    LSA_HANDLE policy = NULL;
    NTSTATUS   st =
        LsaOpenPolicy( NULL, &oa, POLICY_CREATE_ACCOUNT | POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION, &policy );
    if( st != 0 )
    {
        print_winerr( L"LsaOpenPolicy failed", LsaNtStatusToWinError( st ) );
        LocalFree( sid );
        return 1;
    }

    // Prepare privilege name
    LSA_UNICODE_STRING lus;
    lus.Buffer        = (PWSTR) kRight;
    lus.Length        = (USHORT) ( wcslen( kRight ) * sizeof( WCHAR ) );
    lus.MaximumLength = lus.Length;

    DWORD werr = 0;

    if( mode == MODE_ADD )
    {
        st = LsaAddAccountRights( policy, sid, &lus, 1 );
        if( st != 0 )
        {
            werr = LsaNtStatusToWinError( st );
            print_winerr( L"LsaAddAccountRights failed", werr );
            LsaClose( policy );
            LocalFree( sid );
            return 1;
        }
        wprintf( L"Successfully ASSIGNED %ls.\n", kRight );
    }
    else if( mode == MODE_REMOVE )
    {
        st = LsaRemoveAccountRights( policy, sid, FALSE, &lus, 1 );
        if( st != 0 )
        {
            werr = LsaNtStatusToWinError( st );
            print_winerr( L"LsaRemoveAccountRights failed", werr );
            LsaClose( policy );
            LocalFree( sid );
            return 1;
        }
        wprintf( L"Successfully REMOVED %ls.\n", kRight );
    }
    else
    {
        // MODE_LIST (no change)
        wprintf( L"(No changes; listing rights)\n" );
    }

    // Enumerate rights after (or without) change
    PLSA_UNICODE_STRING rights     = NULL;
    ULONG               rightCount = 0;
    st                             = LsaEnumerateAccountRights( policy, sid, &rights, &rightCount );
    if( st == 0 )
    {
        wprintf( L"Account rights (%lu):\n", rightCount );
        for( ULONG i = 0; i < rightCount; ++i ) { wprintf( L"  - %.*ls\n", rights[i].Length / 2, rights[i].Buffer ); }
        LsaFreeMemory( rights );
    }
    else
    {
        DWORD we = LsaNtStatusToWinError( st );
        if( we == ERROR_FILE_NOT_FOUND ) { wprintf( L"No rights currently assigned to this account.\n" ); }
        else { print_winerr( L"LsaEnumerateAccountRights warning", we ); }
    }

    LsaClose( policy );
    LocalFree( sid );

    if( mode != MODE_LIST )
    {
        wprintf( L"\nNEXT STEPS:\n" );
        wprintf( L"  - Sign out and sign back in (or reboot) so your logon token reflects the change.\n" );
        wprintf( L"  - Verify with:  whoami /priv  | findstr /I SeLockMemoryPrivilege\n" );
        wprintf( L"    (It will show as Disabled until your app enables it via AdjustTokenPrivileges.)\n" );
    }

    return 0;
}
