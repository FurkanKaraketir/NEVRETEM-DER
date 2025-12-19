@echo off
echo Building Student Manager with MinGW...

REM Add Qt's MinGW to PATH (64-bit version that matches Qt build)
set QT_DIR=C:/Qt
set PATH=%QT_DIR%/Tools/mingw1310_64/bin;%QT_DIR%/6.9.1/mingw_64/bin;%PATH%

REM Create build directory if it doesn't exist
if not exist "build" mkdir build
cd build

REM Configure with CMake using MinGW Makefiles
echo Configuring with CMake...
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=C:/Qt/6.9.1/mingw_64

REM Check if configuration was successful
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    pause
    exit /b 1
)

REM Build the project
echo Building project...
mingw32-make -j%NUMBER_OF_PROCESSORS%

REM Check if build was successful
if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build completed successfully!
echo Executable location: build\bin\StudentManager.exe
pause
