@echo off
REM Quick fix for missing MinGW DLL error
REM Run this script to manually copy MinGW runtime libraries

echo ========================================
echo MinGW DLL Fix for NEVRETEM-DER MBS
echo ========================================

set TARGET_DIR=%~dp0
if "%1" neq "" set TARGET_DIR=%1

echo Target directory: %TARGET_DIR%

REM Check if the target directory exists
if not exist "%TARGET_DIR%" (
    echo ERROR: Target directory does not exist: %TARGET_DIR%
    pause
    exit /b 1
)

echo Searching for MinGW runtime libraries...

REM Common MinGW paths
set MINGW_PATHS=C:\Qt\Tools\mingw1120_64\bin C:\Qt\Tools\mingw900_64\bin C:\Qt\Tools\mingw810_64\bin C:\mingw64\bin C:\msys64\mingw64\bin

REM Also check Qt installation directories
for /f "tokens=*" %%i in ('where qmake 2^>nul') do (
    set QT_BIN_DIR=%%~dpi
    set MINGW_PATHS=!MINGW_PATHS! !QT_BIN_DIR!
)

set MINGW_FOUND=false

for %%p in (%MINGW_PATHS%) do (
    if exist "%%p\libgcc_s_seh-1.dll" (
        echo Found MinGW runtime libraries at: %%p
        
        echo Copying libgcc_s_seh-1.dll...
        copy "%%p\libgcc_s_seh-1.dll" "%TARGET_DIR%" >nul 2>&1
        if %ERRORLEVEL% equ 0 echo   ✓ libgcc_s_seh-1.dll copied successfully
        
        echo Copying libstdc++-6.dll...
        copy "%%p\libstdc++-6.dll" "%TARGET_DIR%" >nul 2>&1
        if %ERRORLEVEL% equ 0 echo   ✓ libstdc++-6.dll copied successfully
        
        echo Copying libwinpthread-1.dll...
        copy "%%p\libwinpthread-1.dll" "%TARGET_DIR%" >nul 2>&1
        if %ERRORLEVEL% equ 0 echo   ✓ libwinpthread-1.dll copied successfully
        
        set MINGW_FOUND=true
        goto :found_mingw
    )
)

:found_mingw

if "%MINGW_FOUND%"=="true" (
    echo.
    echo ========================================
    echo MinGW DLLs copied successfully!
    echo ========================================
    echo.
    echo The following files were copied to %TARGET_DIR%:
    echo - libgcc_s_seh-1.dll
    echo - libstdc++-6.dll  
    echo - libwinpthread-1.dll
    echo.
    echo Your application should now run without the DLL error.
) else (
    echo.
    echo ========================================
    echo ERROR: MinGW runtime libraries not found
    echo ========================================
    echo.
    echo Could not find MinGW runtime libraries in common locations.
    echo.
    echo Manual solutions:
    echo 1. Install Qt with MinGW compiler
    echo 2. Download MinGW DLLs manually from:
    echo    - libgcc_s_seh-1.dll
    echo    - libstdc++-6.dll
    echo    - libwinpthread-1.dll
    echo.
    echo 3. Or rebuild the application with MSVC compiler
)

echo.
echo Press any key to exit...
pause >nul
