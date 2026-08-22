@echo off
:: ps14 Anti-Tamper Game Engine - Batch Build Script
:: Author: Mistral Vibe
:: Version: 1.0

SETLOCAL EnableDelayedExpansion

:: ============================================================================
:: COLOR DEFINITIONS
:: ============================================================================

FOR /F "tokens=1,2 delims=#" %%a IN ('"prompt #$H#$E# & echo on & for %%b in (1) do rem/"') do (
    SET "DEL=%%a"
)

SET RED=^%DEL%[31m
SET GREEN=^%DEL%[32m
SET YELLOW=^%DEL%[33m
SET BLUE=^%DEL%[34m
SET CYAN=^%DEL%[36m
SET RESET=^%DEL%[0m

:: ============================================================================
:: DEFAULT PARAMETERS
:: ============================================================================

SET CONFIGURATION=Release
SET GENERATOR=Visual Studio 17 2022
SET CLEAN=0
SET VERBOSE=0
SET BUILD_TESTS=1
SET BUILD_KERNEL=0

:: ============================================================================
:: PARSE COMMAND LINE ARGUMENTS
:: ============================================================================

:ParseArgs
IF "%~1"=="" GOTO ArgsDone

IF /I "%~1"=="debug" (
    SET CONFIGURATION=Debug
    SHIFT
    GOTO ParseArgs
)

IF /I "%~1"=="release" (
    SET CONFIGURATION=Release
    SHIFT
    GOTO ParseArgs
)

IF /I "%~1"=="clean" (
    SET CLEAN=1
    SHIFT
    GOTO ParseArgs
)

IF /I "%~1"=="verbose" (
    SET VERBOSE=1
    SHIFT
    GOTO ParseArgs
)

IF /I "%~1"=="notests" (
    SET BUILD_TESTS=0
    SHIFT
    GOTO ParseArgs
)

IF /I "%~1"=="kernel" (
    SET BUILD_KERNEL=1
    SHIFT
    GOTO ParseArgs
)

IF /I "%~1"=="help" (
    CALL :ShowHelp
    EXIT /B 0
)

:ShowHelp
echo.
echo ps14 Build Script - Usage:
echo.
echo   build.bat [options]
echo.
echo Options:
echo   debug        Build in Debug mode (default: Release)
echo   release      Build in Release mode
echo   clean        Clean build directory before building
echo   verbose      Show verbose build output
echo   notests      Disable test compilation
echo   kernel       Enable kernel driver build
echo   help         Show this help message
echo.
echo Examples:
echo   build.bat                    - Build in Release mode
echo   build.bat debug              - Build in Debug mode
echo   build.bat clean debug        - Clean and build in Debug
echo   build.bat kernel notests      - Build with kernel driver, no tests
echo.
GOTO :EOF

:ArgsDone

:: ============================================================================
:: DISPLAY HEADER
:: ============================================================================

echo.
echo %BLUE%=============================================================%RESET%
echo %BLUE%  ps14 Anti-Tamper Game Engine - Build Script%RESET%
echo %BLUE%=============================================================%RESET%
echo.

:: ============================================================================
:: CHECK PREREQUISITES
:: ============================================================================

:: Check if we're in the right directory
IF NOT EXIST "..\CMakeLists.txt" (
    echo %RED%Error: CMakeLists.txt not found in parent directory%RESET%
    echo %RED%Please run this script from the tools/ directory%RESET%
    pause
    EXIT /B 1
)

:: Check for CMake
echo %CYAN%[INFO] Checking for CMake...%RESET%
WHERE cmake > NUL 2>&1
IF %ERRORLEVEL% NEQ 0 (
    echo %RED%Error: CMake is not installed or not in PATH%RESET%
    echo %RED%Please install CMake 3.20+ from https://cmake.org/download/%RESET%
    pause
    EXIT /B 1
)

FOR /F "delims=" %%i IN ('cmake --version') DO SET CMAKE_VERSION=%%i
echo %GREEN%[SUCCESS] CMake version: %CMAKE_VERSION% %RESET%

:: Check for Visual Studio
IF "%GENERATOR:Visual Studio=%" == "%GENERATOR%" (
    echo %CYAN%[INFO] Checking for Visual Studio...%RESET%
    WHERE msbuild > NUL 2>&1
    IF %ERRORLEVEL% NEQ 0 (
        echo %RED%Error: Visual Studio 2022 is not installed%RESET%
        echo %RED%Please install Visual Studio 2022 with Desktop C++ workload%RESET%
        pause
        EXIT /B 1
    )
    echo %GREEN%[SUCCESS] Visual Studio found%RESET%
)

:: ============================================================================
:: SETUP BUILD DIRECTORY
:: ============================================================================

SET BUILD_DIR=..\build
IF NOT EXIST "%BUILD_DIR%" (
    MKDIR "%BUILD_DIR%"
)

CD "%BUILD_DIR%"

:: ============================================================================
:: CLEAN IF REQUESTED
:: ============================================================================

IF %CLEAN% EQU 1 (
    echo %YELLOW%[INFO] Cleaning build directory...%RESET%
    RMDIR /S /Q . > NUL 2>&1
    MKDIR . > NUL 2>&1
)

:: ============================================================================
:: CONFIGURE WITH CMAKE
:: ============================================================================

echo.
echo %BLUE%=============================================================%RESET%
echo %BLUE%  CONFIGURING PROJECT%RESET%
echo %BLUE%=============================================================%RESET%
echo.

echo %CYAN%[INFO] Configuration: %CONFIGURATION% %RESET%
echo %CYAN%[INFO] Generator: %GENERATOR% %RESET%
echo %CYAN%[INFO] Tests: %BUILD_TESTS% %RESET%
echo %CYAN%[INFO] Kernel Driver: %BUILD_KERNEL% %RESET%

SET CMAKE_ARGS=-G "%GENERATOR%" -DCMAKE_BUILD_TYPE=%CONFIGURATION%

IF %BUILD_TESTS% EQU 1 (
    SET CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_TESTS=ON
) ELSE (
    SET CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_TESTS=OFF
)

IF %BUILD_KERNEL% EQU 1 (
    SET CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_KERNEL_DRIVER=ON
) ELSE (
    SET CMAKE_ARGS=%CMAKE_ARGS% -DBUILD_KERNEL_DRIVER=OFF
)

IF %VERBOSE% EQU 1 (
    echo %CYAN%[INFO] CMake arguments: %CMAKE_ARGS% %RESET%
)

echo %CYAN%[INFO] Running CMake configuration...%RESET%
cmake .. %CMAKE_ARGS%
IF %ERRORLEVEL% NEQ 0 (
    echo %RED%[ERROR] CMake configuration failed!%RESET%
    pause
    EXIT /B 1
)

echo %GREEN%[SUCCESS] CMake configuration completed%RESET%

:: ============================================================================
:: BUILD PROJECT
:: ============================================================================

echo.
echo %BLUE%=============================================================%RESET%
echo %BLUE%  BUILDING PROJECT%RESET%
echo %BLUE%=============================================================%RESET%
echo.

echo %CYAN%[INFO] Starting build (%CONFIGURATION%)...%RESET%

IF %VERBOSE% EQU 1 (
    cmake --build . --config %CONFIGURATION% --verbose
) ELSE (
    cmake --build . --config %CONFIGURATION%
)

IF %ERRORLEVEL% NEQ 0 (
    echo %RED%[ERROR] Build failed!%RESET%
    pause
    EXIT /B 1
)

echo %GREEN%[SUCCESS] Build completed!%RESET%

:: ============================================================================
:: BUILD SUMMARY
:: ============================================================================

echo.
echo %BLUE%=============================================================%RESET%
echo %BLUE%  BUILD SUMMARY%RESET%
echo %BLUE%=============================================================%RESET%
echo.
echo %CYAN%Configuration:%RESET% %CONFIGURATION%
echo %CYAN%Generator:%RESET% %GENERATOR%
echo %CYAN%Tests:%RESET% %BUILD_TESTS%
echo %CYAN%Kernel Driver:%RESET% %BUILD_KERNEL%
echo %CYAN%Output Directory:%RESET% ..\build\bin
echo.

:: List output files
echo %CYAN%Build outputs:%RESET%
SET OUTPUT_COUNT=0
FOR /R ..\build\bin %%F IN (*.exe *.dll *.lib *.sys) DO (
    SET /A OUTPUT_COUNT+=1
    IF !OUTPUT_COUNT! LSS 21 (
        echo %GREEN%  - %%F%RESET%
    )
)

IF %OUTPUT_COUNT% GTR 20 (
    echo %CYAN%  ... and %OUTPUT_COUNT% more files%RESET%
)

:: Run tests if enabled and in Debug mode
IF %BUILD_TESTS% EQU 1 IF %CONFIGURATION% == Debug (
    echo.
    echo %BLUE%=============================================================%RESET%
    echo %BLUE%  RUNNING TESTS%RESET%
    echo %BLUE%=============================================================%RESET%
    echo.
    echo %CYAN%[INFO] Running unit tests...%RESET%
    ctest --output-on-failure -C %CONFIGURATION%
    IF %ERRORLEVEL% EQU 0 (
        echo %GREEN%[SUCCESS] All tests pass
