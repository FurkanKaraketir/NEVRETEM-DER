@echo off
REM Windows Deployment Script for NEVRETEM-DER MBS
REM This script packages the application with all required Qt libraries

echo ========================================
echo NEVRETEM-DER MBS Windows Deployment
echo ========================================

REM Check if build directory exists
if not exist "build\bin\StudentManager.exe" (
    echo ERROR: StudentManager.exe not found in build\bin\
    echo Please build the project first using build.bat
    pause
    exit /b 1
)

REM Create deployment directory
set DEPLOY_DIR=deploy_windows
if exist %DEPLOY_DIR% (
    echo Removing existing deployment directory...
    rmdir /s /q %DEPLOY_DIR%
)

echo Creating deployment directory: %DEPLOY_DIR%
mkdir %DEPLOY_DIR%

REM Copy the executable
echo Copying executable...
copy "build\bin\StudentManager.exe" "%DEPLOY_DIR%\"

REM Copy configuration files
echo Copying configuration files...
if exist "config.ini" copy "config.ini" "%DEPLOY_DIR%\"
copy "config.example.ini" "%DEPLOY_DIR%\"

REM Copy universities.json
if exist "build\bin\universities.json" copy "build\bin\universities.json" "%DEPLOY_DIR%\"

REM Copy logo
if exist "src\logo.jpg" copy "src\logo.jpg" "%DEPLOY_DIR%\"

REM Find Qt installation path
echo Detecting Qt installation...
set QT_DIR=
for /f "tokens=*" %%i in ('where qmake 2^>nul') do set QT_DIR=%%~dpi
if "%QT_DIR%"=="" (
    echo WARNING: Qt not found in PATH. Trying common locations...
    if exist "C:\Qt\6.7.0\msvc2022_64\bin\windeployqt.exe" (
        set QT_DIR=C:\Qt\6.7.0\msvc2022_64\bin\
    ) else if exist "C:\Qt\6.6.0\msvc2022_64\bin\windeployqt.exe" (
        set QT_DIR=C:\Qt\6.6.0\msvc2022_64\bin\
    ) else if exist "C:\Qt\6.5.0\msvc2022_64\bin\windeployqt.exe" (
        set QT_DIR=C:\Qt\6.5.0\msvc2022_64\bin\
    ) else (
        echo ERROR: Could not find Qt installation
        echo Please ensure Qt is installed and qmake is in your PATH
        echo Or manually set QT_DIR variable in this script
        pause
        exit /b 1
    )
)

echo Qt found at: %QT_DIR%

REM Run windeployqt to copy Qt libraries
echo Running windeployqt...
"%QT_DIR%windeployqt.exe" --verbose 2 --dir "%DEPLOY_DIR%" --libdir "%DEPLOY_DIR%" --plugindir "%DEPLOY_DIR%\plugins" --no-translations --no-system-d3d-compiler --no-opengl-sw --no-compiler-runtime "%DEPLOY_DIR%\StudentManager.exe"

if %ERRORLEVEL% neq 0 (
    echo ERROR: windeployqt failed
    pause
    exit /b 1
)

REM Copy MinGW runtime libraries (if using MinGW)
echo Checking for MinGW runtime libraries...
set MINGW_FOUND=false

REM Check if Qt was built with MinGW
"%QT_DIR%qmake.exe" -query QT_VERSION > temp_qt_info.txt 2>nul
if exist temp_qt_info.txt (
    REM Look for MinGW libraries in Qt bin directory
    if exist "%QT_DIR%libgcc_s_seh-1.dll" (
        echo Found MinGW runtime libraries in Qt directory
        copy "%QT_DIR%libgcc_s_seh-1.dll" "%DEPLOY_DIR%\" 2>nul
        copy "%QT_DIR%libstdc++-6.dll" "%DEPLOY_DIR%\" 2>nul
        copy "%QT_DIR%libwinpthread-1.dll" "%DEPLOY_DIR%\" 2>nul
        set MINGW_FOUND=true
    )
    del temp_qt_info.txt 2>nul
)

REM Check common MinGW installation paths
if "%MINGW_FOUND%"=="false" (
    echo Searching for MinGW in common locations...
    set MINGW_PATHS=C:\Qt\Tools\mingw1120_64\bin C:\Qt\Tools\mingw900_64\bin C:\Qt\Tools\mingw810_64\bin C:\mingw64\bin C:\msys64\mingw64\bin
    
    for %%p in (%MINGW_PATHS%) do (
        if exist "%%p\libgcc_s_seh-1.dll" (
            echo Found MinGW runtime libraries at: %%p
            copy "%%p\libgcc_s_seh-1.dll" "%DEPLOY_DIR%\" 2>nul
            copy "%%p\libstdc++-6.dll" "%DEPLOY_DIR%\" 2>nul
            copy "%%p\libwinpthread-1.dll" "%DEPLOY_DIR%\" 2>nul
            set MINGW_FOUND=true
            goto :mingw_found
        )
    )
)
:mingw_found

if "%MINGW_FOUND%"=="false" (
    echo WARNING: MinGW runtime libraries not found
    echo If your application was built with MinGW, you may need to:
    echo 1. Copy libgcc_s_seh-1.dll, libstdc++-6.dll, libwinpthread-1.dll manually
    echo 2. Or install MinGW runtime on target systems
)

REM Copy Visual C++ Redistributables (if available and not MinGW)
if "%MINGW_FOUND%"=="false" (
    echo Checking for Visual C++ Redistributables...
    set VCREDIST_PATH=
    for /d %%i in ("C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\*") do (
        if exist "%%i\x64\Microsoft.VC143.CRT\msvcp140.dll" (
            set VCREDIST_PATH=%%i\x64\Microsoft.VC143.CRT\
        )
    )

    if not "%VCREDIST_PATH%"=="" (
        echo Copying Visual C++ Runtime libraries...
        copy "%VCREDIST_PATH%msvcp140.dll" "%DEPLOY_DIR%\" 2>nul
        copy "%VCREDIST_PATH%vcruntime140.dll" "%DEPLOY_DIR%\" 2>nul
        copy "%VCREDIST_PATH%vcruntime140_1.dll" "%DEPLOY_DIR%\" 2>nul
    ) else (
        echo WARNING: Visual C++ Redistributables not found
        echo Users may need to install Visual C++ Redistributable 2022
    )
)

REM Create a simple launcher script
echo Creating launcher script...
echo @echo off > "%DEPLOY_DIR%\run.bat"
echo cd /d "%%~dp0" >> "%DEPLOY_DIR%\run.bat"
echo start StudentManager.exe >> "%DEPLOY_DIR%\run.bat"

REM Create README for deployment
echo Creating deployment README...
echo NEVRETEM-DER MBS - Windows Distribution > "%DEPLOY_DIR%\README.txt"
echo ======================================= >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo Version: 1.0.0 >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo This package contains the NEVRETEM-DER MBS application >> "%DEPLOY_DIR%\README.txt"
echo with all required Qt libraries for Windows. >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo To run the application: >> "%DEPLOY_DIR%\README.txt"
echo 1. Double-click StudentManager.exe >> "%DEPLOY_DIR%\README.txt"
echo    OR >> "%DEPLOY_DIR%\README.txt"
echo 2. Double-click run.bat >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo Configuration: >> "%DEPLOY_DIR%\README.txt"
echo - Copy config.example.ini to config.ini >> "%DEPLOY_DIR%\README.txt"
echo - Edit config.ini with your Firebase credentials >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo Updates: >> "%DEPLOY_DIR%\README.txt"
echo - The application can check for updates automatically >> "%DEPLOY_DIR%\README.txt"
echo - Go to Help -^> Check for Updates to get the latest version >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo System Requirements: >> "%DEPLOY_DIR%\README.txt"
echo - Windows 10 or later >> "%DEPLOY_DIR%\README.txt"
echo - Visual C++ Redistributable 2022 (if not already installed) >> "%DEPLOY_DIR%\README.txt"
echo. >> "%DEPLOY_DIR%\README.txt"
echo For support, contact: FURKAN KARAKETIR +90 551 145 09 68 >> "%DEPLOY_DIR%\README.txt"

echo.
echo ========================================
echo Deployment completed successfully!
echo ========================================
echo Deployment directory: %DEPLOY_DIR%
echo.
echo The application is ready for distribution.
echo You can zip the '%DEPLOY_DIR%' folder and distribute it.
echo.
echo To test the deployment:
echo 1. Navigate to %DEPLOY_DIR%
echo 2. Run StudentManager.exe
echo.

REM Offer to create ZIP archive
set /p CREATE_ZIP="Create ZIP archive for distribution? (y/n): "
if /i "%CREATE_ZIP%"=="y" (
    echo Creating ZIP archive...
    powershell -NoProfile -ExecutionPolicy Bypass -Command "$src='%DEPLOY_DIR%'; Compress-Archive -Path $src -DestinationPath 'NEVRETEM-DER_MBS_Windows.zip' -Force"
    if exist "NEVRETEM-DER_MBS_Windows.zip" (
        echo ZIP archive created: NEVRETEM-DER_MBS_Windows.zip
    ) else (
        echo Failed to create ZIP archive
    )
)

echo.
echo Press any key to exit...
pause >nul
