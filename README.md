
Supported platforms:
 - x86_64
 - ARM64

Supported OS's
 - Linux
 - Windows

Supported compilers:
 - MSVC 14.38+
 - GCC 5+
 - clang 5+

### Building on Windows

    .\tools\premake5 vs2022
    msbuild SuperMalloc.sln /p:Configuration=Release /p:Platform=x64
    msbuild SuperMalloc.sln /p:Configuration=Debug /p:Platform=x64

### Building on Linux

    ./tools/premake5 gmake2
    make config=Release platform=x64 -j$(nproc)
    make config=Release platform=ARM64 -j$(nproc)

### Links

https://gcc.gnu.org/projects/c-status.html#c11
https://clang.llvm.org/c_status.html#c11


### Other

debug w/ cache no prefetch: 73.536790ns/small_malloc
debug w/o cache and prefetch: 77.387280ns/small_malloc

release w/o cache and prefetch (windows baseline): 24.578000ns/small_malloc


ENABLE_LOG_CHECKING
ENABLE_STATS
ENABLE_FAILED_COUNTS