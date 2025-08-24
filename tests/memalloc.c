// cl /W4 /O2 memalloc.c
#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#pragma comment( lib, "Advapi32.lib" )

static DWORD
print_last_error( const char* msg )
{
    DWORD  err = GetLastError();
    LPVOID buf = NULL;
    FormatMessageA( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, err,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ), (LPSTR) &buf, 0, NULL );
    fprintf( stderr, "%s (err=%lu): %s\n", msg, err, buf ? (char*) buf : "(no message)" );
    if( buf ) LocalFree( buf );
    return err;
}

static BOOL
enable_lock_pages_privilege( void )
{
    HANDLE           hToken = NULL;
    TOKEN_PRIVILEGES tp     = { 0 };
    LUID             luid;

    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken ) )
    {
        print_last_error( "OpenProcessToken failed" );
        return FALSE;
    }
    if( !LookupPrivilegeValue( NULL, SE_LOCK_MEMORY_NAME, &luid ) )
    {
        print_last_error( "LookupPrivilegeValue(SE_LOCK_MEMORY_NAME) failed" );
        CloseHandle( hToken );
        return FALSE;
    }

    tp.PrivilegeCount           = 1;
    tp.Privileges[0].Luid       = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if( !AdjustTokenPrivileges( hToken, FALSE, &tp, sizeof( tp ), NULL, NULL ) )
    {
        DWORD err = print_last_error( "AdjustTokenPrivileges failed" );
        CloseHandle( hToken );
        if( err == ERROR_NOT_ALL_ASSIGNED )
        {
            fprintf( stderr, "This account does not have SeLockMemoryPrivilege assigned.\n"
                             "Grant 'Lock pages in memory' to this user, then sign out/in.\n" );
        }

        return FALSE;
    }

    // Per docs, need to check GetLastError() == ERROR_SUCCESS
    DWORD err = GetLastError();
    CloseHandle( hToken );
    if( err != ERROR_SUCCESS )
    {
        SetLastError( err );
        print_last_error( "AdjustTokenPrivileges reported partial/failed enable" );
        return FALSE;
    }
    return TRUE;
}

int
main( void )
{
    enable_lock_pages_privilege();
    SIZE_T large_min = GetLargePageMinimum();    // 0 if privilege missing/unsupported
    printf( "GetLargePageMinimum(): %llu bytes\n", (unsigned long long) large_min );

    // Load VirtualAlloc2 dynamically
    HMODULE h = GetModuleHandleW( L"KernelBase.dll" );
    if( !h ) h = GetModuleHandleW( L"Kernel32.dll" );
    typedef PVOID( WINAPI * PFN_VirtualAlloc2 )( HANDLE, PVOID, SIZE_T, ULONG, ULONG, MEM_EXTENDED_PARAMETER*, ULONG );
    PFN_VirtualAlloc2 pVirtualAlloc2 = (PFN_VirtualAlloc2) ( h ? GetProcAddress( h, "VirtualAlloc2" ) : NULL );

    if( !pVirtualAlloc2 )
    {
        fprintf( stderr, "VirtualAlloc2 not available on this OS.\n" );
        return 1;
    }

    // --- Try 1 GiB HUGE pages ---
    const SIZE_T           oneGiB = (SIZE_T) 1 << 30;
    MEM_EXTENDED_PARAMETER ep     = { 0 };
    ep.Type                       = MemExtendedParameterAttributeFlags;      // <- attribute flags
    ep.ULong64                    = MEM_EXTENDED_PARAMETER_NONPAGED_HUGE;    // <- HUGE

    PVOID p =
        pVirtualAlloc2( GetCurrentProcess(), NULL, oneGiB, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE, &ep, 1 );
    if( p )
    {
        printf( "Allocated 1 GiB HUGE page(s) at %p\n", p );
        VirtualFree( p, 0, MEM_RELEASE );
    }
    else { print_last_error( "1 GiB huge page allocation failed, falling back..." ); }

    if( !large_min )
    {
        print_last_error( "GetLargePageMinimum returned 0 (privilege or support issue)\n" );
        return 1;
    }
    ep.ULong64   = MEM_EXTENDED_PARAMETER_NONPAGED_LARGE;    // <- LARGE
    SIZE_T alloc = 4 * large_min;                            // e.g., 4 large pages
    p = pVirtualAlloc2( GetCurrentProcess(), NULL, alloc, MEM_RESERVE | MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE, &ep, 1 );
    if( !p )
    {
        print_last_error( "2 MiB large page allocation failed" );
        return 1;
    }

    printf( "Allocated LARGE pages at %p (size=%zu, page=%zu)\n", p, alloc, large_min );
    VirtualFree( p, 0, MEM_RELEASE );

    return 0;
}
