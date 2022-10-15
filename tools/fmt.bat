@echo off

if "%1"=="" goto no_args

%~dp0clang-format -i %*
goto end

:no_args
for /f "delims=" %%a in ('dir /b /s *.cpp *.hpp *.c *.h') do echo formatting %%a... && %~dp0clang-format -i %%a
goto end

:no_clang_format
echo ERROR: clang-format executable not found

:end
