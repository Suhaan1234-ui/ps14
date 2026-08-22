<#
.SYNOPSIS
    Build script for ps14 Anti-Tamper Game Engine
.DESCRIPTION
    This PowerShell script builds the ps14 project using CMake
    and Visual Studio.
.NOTES
    Author: Mistral Vibe
    Version: 1.0
#>

param(
    [string]$Configuration = "Release",
    [string]$Generator = "Visual Studio 17 2022",
    [switch]$Clean = $false,
    [switch]$Verbose = $false,
    [switch]$BuildTests = $true,
    [switch]$BuildKernel = $false
)

# Colors for output
$Reset = "$([char]27)[0m"
$Red = "$([char]27)[31m"
$Green = "$([char]27)[32m"
$Yellow = "$([char]27)[33m"
$Blue = "$([char]27)[34m"
$Cyan = "$([char]27)[36m"

function Write-Status {
    param([string]$Message, [string]$Color = "White")
    $colorCode = switch($Color) {
        "Red" { $Red }
        "Green" { $Green }
        "Yellow" { $Yellow }
        "Blue" { $Blue }
        "Cyan" { $Cyan }
        default { "" }
    }
    Write-Host "[$(Get-Date -Format 'HH:mm:ss')] $colorCode$Message$Reset"
}

function Write-Header {
    param([string]$Message)
    Write-Host "`n$Blue$Message$Reset"
    Write-Host "$(('=' * $Message.Length))$Blue$('=' * $Message.Length)$Reset`n"
}

# Check if we're in the right directory
$scriptPath = $PSScriptRoot
$projectRoot = Resolve-Path "$scriptPath/.."
$cmakeListsPath = Join-Path $projectRoot "CMakeLists.txt"

if (-not (Test-Path $cmakeListsPath)) {
    Write-Status "Error: CMakeLists.txt not found in $projectRoot" "Red"
    Write-Status "Please run this script from the tools/ directory or project root" "Red"
    exit 1
}

# Create build directory
$buildDir = Join-Path $projectRoot "build"
if (-not (Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

# Change to build directory
Set-Location $buildDir

# Clean if requested
if ($Clean) {
    Write-Status "Cleaning build directory..." "Yellow"
    if (Test-Path "$buildDir/*") {
        Remove-Item "$buildDir/*" -Recurse -Force
    }
}

# Check for CMake
Write-Status "Checking for CMake..." "Cyan"
try {
    $cmakeVersion = cmake --version
    if ($LASTEXITCODE -ne 0) {
        throw "CMake not found"
    }
    Write-Status "CMake version: $cmakeVersion" "Green"
} catch {
    Write-Status "Error: CMake is not installed or not in PATH" "Red"
    Write-Status "Please install CMake 3.20+ from https://cmake.org/download/" "Red"
    exit 1
}

# Check for Visual Studio
if ($Generator -like "*Visual Studio*") {
    Write-Status "Checking for Visual Studio..." "Cyan"
    try {
        $vsWhere = & "$env:ProgramFiles(x86)\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.Component.MSBuild
        if ($LASTEXITCODE -ne 0) {
            throw "Visual Studio not found"
        }
        Write-Status "Visual Studio found: $vsWhere" "Green"
    } catch {
        Write-Status "Error: Visual Studio 2022 is not installed" "Red"
        Write-Status "Please install Visual Studio 2022 with Desktop C++ workload" "Red"
        exit 1
    }
}

# Configure CMake
Write-Header "Configuring Project"

$cmakeArgs = @(
    "-G", $Generator,
    "-DCMAKE_BUILD_TYPE=$Configuration"
)

if ($BuildTests) {
    $cmakeArgs += "-DBUILD_TESTS=ON"
} else {
    $cmakeArgs += "-DBUILD_TESTS=OFF"
}

if ($BuildKernel) {
    $cmakeArgs += "-DBUILD_KERNEL_DRIVER=ON"
} else {
    $cmakeArgs += "-DBUILD_KERNEL_DRIVER=OFF"
}

if ($Verbose) {
    Write-Status "CMake arguments: $($cmakeArgs -join ' ')" "Cyan"
}

Write-Status "Configuring with CMake..." "Cyan"
$cmakeOutput = cmake .. @cmakeArgs 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Status "CMake configuration failed!" "Red"
    Write-Status $cmakeOutput "Red"
    exit 1
}

Write-Status "CMake configuration completed successfully" "Green"

# Build the project
Write-Header "Building Project"

Write-Status "Starting build ($Configuration)..." "Cyan"

if ($Verbose) {
    $buildCmd = "cmake --build . --config $Configuration --verbose"
} else {
    $buildCmd = "cmake --build . --config $Configuration"
}

$buildOutput = & $buildCmd 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Status "Build failed!" "Red"
    Write-Status $buildOutput "Red"
    exit 1
}

Write-Status "Build completed successfully!" "Green"

# Summary
Write-Header "Build Summary"
Write-Status "Configuration: $Configuration" "Cyan"
Write-Status "Generator: $Generator" "Cyan"
Write-Status "Tests: $(if($BuildTests) {'Enabled'} else {'Disabled'})" "Cyan"
Write-Status "Kernel Driver: $(if($BuildKernel) {'Enabled'} else {'Disabled'})" "Cyan"
Write-Status "`nOutput directory: $(Join-Path $buildDir 'bin')" "Cyan"

# List output files
Write-Status "`nBuild outputs:" "Cyan"
$outputFiles = Get-ChildItem "$(Join-Path $buildDir 'bin')" -File -Recurse -ErrorAction SilentlyContinue
if ($outputFiles) {
    foreach ($file in $outputFiles | Select-Object -First 20) {
        Write-Status "  - $($file.FullName.Replace($projectRoot, ''))")" "Green"
    }
    if ($outputFiles.Count -gt 20) {
        Write-Status "  ... and $($outputFiles.Count - 20) more files" "Cyan"
    }
} else {
    Write-Status "  No output files found in bin/ directory" "Yellow"
}

# Test run (if enabled)
if ($BuildTests -and $Configuration -eq "Debug") {
    Write-Header "Running Tests"
    Write-Status "Running unit tests..." "Cyan"
    $testOutput = ctest --output-on-failure -C $Configuration 2>&1
    Write-Status $testOutput
    if ($LASTEXITCODE -eq 0) {
        Write-Status "All tests passed!" "Green"
    } else {
        Write-Status "Some tests failed" "Yellow"
    }
}

Write-Status "`n$Green Build process completed!$Reset" "Green"
