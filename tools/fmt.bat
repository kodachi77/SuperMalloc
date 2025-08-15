@echo off

if "%1"=="" goto no_args

%~dp0clang-format -i %*
goto end

:no_args
for /f "delims=" %%a in ('dir /b /s *.c *.h') do echo formatting %%a... && %~dp0clang-format -i %%a && %~dp0dos2unix -q %%a
goto end

:no_clang_format
echo ERROR: clang-format executable not found

:end
