@echo off

set "INCLUDE="
set "LIB="
set Path=%SystemRoot%\System32;%SystemRoot%;%SystemRoot%\System32\Wbem

set "VSPath="
for /f "delims=" %%a in ('%~dp0vswhere -nologo -version ^[15^,16^) -property installationPath') do set "VSPath=%%a"
if "%VSPath%"=="" goto no_vs

call "%VSPath%\VC\Auxiliary\Build\vcvarsall.bat" x64

if not exist %~d0\bin mkdir %~d0\bin
if not exist %~d0\bin\x64 mkdir %~d0\bin\x64

set Path=%~d0\tools;%~d0\bin\x64;%Path%
goto end

:no_vs
echo *** ERROR! *** Compatible Visual Studio not found. vswhere returned %errorlevel%
exit /B %errorlevel%

:end
