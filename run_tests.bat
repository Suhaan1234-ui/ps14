@echo off
echo ========================================================
echo   ps14 Anti-Tamper Engine - Unit Tests
echo ========================================================
echo.

:: Navigate to project
cd O:\pss14\ps14

:: Verify we're in the right place
if not exist CMakeLists.txt (
    echo ERROR: CMakeLists.txt not found!
    echo You are in:
    cd
    echo.
    echo Please run this from O:\pss14\ps14
    pause
    exit /b 1
)

echo [INFO] Found project at: %cd%
echo.

:: Clean previous build
if exist build (
    echo [INFO] Cleaning old build...
    rmdir /s /q build
)

:: Create build directory
mkdir build
cd build

:: Find CMake
set CMAKE_EXE=
where cmake >nul 2>&1 && set CMAKE_EXE=cmake

if "%CMAKE_EXE%"=="" (
    if exist "C:\Program Files\CMake\bin\cmake.exe" set CMAKE_EXE="C:\Program Files\CMake\bin\cmake.exe"
    if exist "C:\Program Files (x86)\CMake\bin\cmake.exe" set CMAKE_EXE="C:\Program Files (x86)\CMake\bin\cmake.exe"
)

if "%CMAKE_EXE%"=="" (
    echo ERROR: CMake not found!
    echo.
    echo Please install CMake from: https://cmake.org/download/
    echo.
    pause
    exit /b 1
)

echo [1/4] Configuring with CMake...
echo Generator: Visual Studio 17 2022
echo.

%CMAKE_EXE% .. -G "Visual Studio 17 2022" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: CMake configuration failed!
    echo.
    echo Trying with Ninja generator instead...
    %CMAKE_EXE% .. -G "Ninja" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
    if %ERRORLEVEL% NEQ 0 (
        echo.
        echo ========================================================
        echo FAILED: Could not configure project
        echo.
        echo Possible fixes:
        echo 1. Install Visual Studio 2022 with C++ workload
        echo 2. Install CMake from cmake.org
        echo 3. Make sure you're running from:
        echo    x64 Native Tools Command Prompt for VS 2022
        echo ========================================================
        pause
        exit /b 1
    )
)

echo.
echo [2/4] Building project...
%CMAKE_EXE% --build . --config Debug --parallel 4

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo [3/4] Running unit tests...
echo ========================================================
cd bin\Debug
ps14_tests_unit.exe
echo ========================================================

echo.
echo [4/4] Running integration tests...
ps14_tests_integration.exe

echo.
echo ========================================================
echo ✅ ALL TESTS COMPLETED SUCCESSFULLY!
echo ========================================================
pause