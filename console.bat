@echo off
setlocal enabledelayedexpansion

set curr_dir=%~dp0
set curr_dir=!curr_dir:~0,-1!

for /f "tokens=1,2 delims==> " %%a in ('subst') do if /I "%%b"=="%curr_dir%" set "drive=%%a" && set "drive=!drive:~0,-2!" && goto new_console

set "drive=B:"
@for %%a in (Z Y X W V U T S R Q P O N M L K J I H G F E D C) do if not exist %%a:\ set "drive=%%a:" && goto run_subst

:run_subst
subst -cur_console %drive% "%curr_dir%"
if %errorlevel% neq 0 goto subst_failed

:new_console
%comspec% -new_console:d:%drive%\src /k %drive%\tools\env.bat
goto end

:subst_failed
echo ERROR: Failed to create %drive% drive.

:end
