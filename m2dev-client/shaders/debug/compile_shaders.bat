@echo off
setlocal

set "FXC="

for /f "delims=" %%D in ('dir /b /ad /o-n "C:\Program Files (x86)\Windows Kits\10\bin" 2^>nul') do (
    if exist "C:\Program Files (x86)\Windows Kits\10\bin\%%D\x64\fxc.exe" (
        set "FXC=C:\Program Files (x86)\Windows Kits\10\bin\%%D\x64\fxc.exe"
        goto :fxc_found
    )
)

if exist "C:\Program Files (x86)\Windows Kits\10\bin\x64\fxc.exe" (
    set "FXC=C:\Program Files (x86)\Windows Kits\10\bin\x64\fxc.exe"
    goto :fxc_found
)

echo Nu am gasit fxc.exe
pause
exit /b 1

:fxc_found
echo FXC: "%FXC%"

set "INPUT_DIR=."
set "OUTPUT_DIR=.\cso"

if not exist "%OUTPUT_DIR%" mkdir "%OUTPUT_DIR%"

for %%F in ("%INPUT_DIR%\*Vs.hlsl") do (
    echo Compiling VS: %%~nxF
    "%FXC%" /nologo /T vs_5_0 /E main /O3 /Fo "%OUTPUT_DIR%\%%~nF.cso" "%%F"
)

for %%F in ("%INPUT_DIR%\*Ps.hlsl") do (
    echo Compiling PS: %%~nxF
    "%FXC%" /nologo /T ps_5_0 /E main /O3 /Fo "%OUTPUT_DIR%\%%~nF.cso" "%%F"
)

echo Done.
pause