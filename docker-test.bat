@echo off
setlocal enabledelayedexpansion

set curr_dir=%~dp0
set curr_dir=!curr_dir:~0,-1!

set DOCKER_BUILDKIT=1
docker buildx bake --progress=plain %*

if %errorlevel% neq 0 (
    echo "Docker build failed."
    exit /b %errorlevel%
)

docker run --rm -v "%curr_dir%":/src -w /src toolchain:gcc9 bash -lc "tests/docker-test.sh --gcc 9"
docker run --rm -v "%curr_dir%":/src -w /src toolchain:clang11 bash -lc "tests/docker-test.sh --clang 11"
docker run --rm -v "%curr_dir%":/src -w /src toolchain:gcc-modern bash -lc "tests/docker-test.sh --gcc 14"
docker run --rm -v "%curr_dir%":/src -w /src toolchain:clang-modern bash -lc "tests/docker-test.sh --clang 21"