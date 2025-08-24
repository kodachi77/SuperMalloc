@echo off
setlocal enabledelayedexpansion

set curr_dir=%~dp0
set curr_dir=!curr_dir:~0,-1!

set DOCKER_BUILDKIT=1

for /f "usebackq delims=" %%i in (`docker system info -f "{{.OSType}}"`) do set HOST_OS=%%i
echo HOST_OS is %HOST_OS%

docker buildx bake --progress=plain %*

if %errorlevel% neq 0 (
    echo "Docker buildx bake failed."
    exit /b %errorlevel%
)

if "%HOST_OS%"=="windows" (
    echo "Running tests on Windows host"
    docker run --rm -v "%curr_dir%":C:\src -w C:\src toolchain:msvc-clang-cl cmd /S /C "C:\BuildTools\VC\Auxiliary\Build\vcvarsall.bat x86_amd64 && where clang-cl"
) else (
    echo "Running tests on Linux host"
    docker run --rm -v "%curr_dir%":/src -w /src toolchain:gcc9 bash -lc "tests/docker-test.sh --gcc 9"
    docker run --rm -v "%curr_dir%":/src -w /src toolchain:clang11 bash -lc "tests/docker-test.sh --clang 11"
    docker run --rm -v "%curr_dir%":/src -w /src toolchain:gcc-modern bash -lc "tests/docker-test.sh --gcc 14"
    docker run --rm -v "%curr_dir%":/src -w /src toolchain:clang-modern bash -lc "tests/docker-test.sh --clang 21"
)
