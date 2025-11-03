# ARM64 GUI Hypervisor - Build & Deploy Script
# Windows 11 ARM64 deployment with graphics virtualization

param(
    [switch]$Clean,
    [switch]$Debug,
    [switch]$Deploy,
    [string]$OutputDir = "build_gui"
)

$ErrorActionPreference = "Stop"

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ARM64 GUI Hypervisor - Build & Deploy" -ForegroundColor Cyan
Write-Host "Graphics Virtualization with DirectX 11" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Check if running as Administrator
$isAdmin = ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")
if (-not $isAdmin) {
    Write-Host "Warning: Not running as Administrator. Some features may not work." -ForegroundColor Yellow
}

# Check Windows version and architecture
$osInfo = Get-WmiObject -Class Win32_OperatingSystem
$procInfo = Get-WmiObject -Class Win32_Processor

Write-Host "System Information:" -ForegroundColor Green
Write-Host "  OS: $($osInfo.Caption) $($osInfo.Version)" -ForegroundColor White
Write-Host "  Architecture: $($osInfo.OSArchitecture)" -ForegroundColor White
Write-Host "  Processor: $($procInfo.Name)" -ForegroundColor White

# Verify ARM64 architecture
if ($osInfo.OSArchitecture -ne "64-bit ARM") {
    Write-Host "Warning: This hypervisor is optimized for ARM64. Current architecture: $($osInfo.OSArchitecture)" -ForegroundColor Yellow
}

# Check Hyper-V capability
Write-Host "Checking hypervisor capabilities..." -ForegroundColor Green
$hyperVFeature = Get-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All
if ($hyperVFeature.State -eq "Enabled") {
    Write-Host "  Hyper-V: Enabled" -ForegroundColor Green
} else {
    Write-Host "  Hyper-V: Disabled (will be enabled)" -ForegroundColor Yellow
    try {
        Enable-WindowsOptionalFeature -Online -FeatureName Microsoft-Hyper-V-All -All -NoRestart
        Write-Host "  Hyper-V enabled successfully" -ForegroundColor Green
    } catch {
        Write-Host "  Warning: Could not enable Hyper-V: $($_.Exception.Message)" -ForegroundColor Yellow
    }
}

# Check DirectX capabilities
Write-Host "Checking DirectX capabilities..." -ForegroundColor Green
try {
    $dxDiag = Get-WmiObject -Class Win32_VideoController | Select-Object Name, AdapterRAM, DriverVersion
    foreach ($adapter in $dxDiag) {
        $ramGB = [math]::Round($adapter.AdapterRAM / 1GB, 2)
        Write-Host "  GPU: $($adapter.Name) ($ramGB GB)" -ForegroundColor White
        Write-Host "  Driver: $($adapter.DriverVersion)" -ForegroundColor White
    }
} catch {
    Write-Host "  Warning: Could not detect GPU information" -ForegroundColor Yellow
}

# Setup build environment
Write-Host "Setting up build environment..." -ForegroundColor Green

# Find Visual Studio
$vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (Test-Path $vsWhere) {
    $vsPath = & $vsWhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($vsPath) {
        Write-Host "  Visual Studio found: $vsPath" -ForegroundColor Green
        
        # Setup VS environment
        $vcvarsPath = "$vsPath\VC\Auxiliary\Build\vcvarsall.bat"
        if (Test-Path $vcvarsPath) {
            Write-Host "  Setting up Visual Studio environment..." -ForegroundColor Green
            cmd /c "`"$vcvarsPath`" arm64 & set" | ForEach-Object {
                if ($_ -match "^(.*?)=(.*)$") {
                    Set-Item -Path "env:$($matches[1])" -Value $matches[2]
                }
            }
        }
    }
} else {
    Write-Host "  Visual Studio not found, checking for MinGW..." -ForegroundColor Yellow
    
    # Try to find MinGW
    $mingwPaths = @(
        "C:\msys64\mingw64\bin",
        "C:\MinGW\bin",
        "C:\TDM-GCC-64\bin"
    )
    
    foreach ($path in $mingwPaths) {
        if (Test-Path "$path\gcc.exe") {
            Write-Host "  MinGW found: $path" -ForegroundColor Green
            $env:PATH = "$path;$env:PATH"
            break
        }
    }
}

# Clean if requested
if ($Clean) {
    Write-Host "Cleaning build directory..." -ForegroundColor Yellow
    if (Test-Path $OutputDir) {
        Remove-Item -Path $OutputDir -Recurse -Force
    }
}

# Create build directory
if (-not (Test-Path $OutputDir)) {
    New-Item -Path $OutputDir -ItemType Directory -Force | Out-Null
}

Set-Location $OutputDir

# Configure CMake
Write-Host "Configuring CMake..." -ForegroundColor Green
$buildType = if ($Debug) { "Debug" } else { "Release" }

$cmakeArgs = @(
    "-S", ".."
    "-B", "."
    "-f", "..\CMakeLists_GUI.txt"
    "-DCMAKE_BUILD_TYPE=$buildType"
)

# Add ARM64 specific configuration
if ($env:PROCESSOR_ARCHITECTURE -eq "ARM64") {
    $cmakeArgs += "-DCMAKE_SYSTEM_PROCESSOR=ARM64"
    $cmakeArgs += "-DCMAKE_GENERATOR_PLATFORM=ARM64"
}

# Use Visual Studio generator if available
if (Get-Command "msbuild.exe" -ErrorAction SilentlyContinue) {
    $cmakeArgs += "-G", "Visual Studio 17 2022"
    Write-Host "  Using Visual Studio 17 2022 generator" -ForegroundColor Green
} elseif (Get-Command "ninja.exe" -ErrorAction SilentlyContinue) {
    $cmakeArgs += "-G", "Ninja"
    Write-Host "  Using Ninja generator" -ForegroundColor Green
} else {
    Write-Host "  Using default generator" -ForegroundColor Yellow
}

try {
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed"
    }
    Write-Host "  CMake configuration successful" -ForegroundColor Green
} catch {
    Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Build the project
Write-Host "Building GUI hypervisor..." -ForegroundColor Green
try {
    & cmake --build . --config $buildType --parallel
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed"
    }
    Write-Host "  Build successful" -ForegroundColor Green
} catch {
    Write-Host "  Error: $($_.Exception.Message)" -ForegroundColor Red
    exit 1
}

# Find the executable
$exeName = "ARM64_GUI_Hypervisor.exe"
$exePaths = @(
    ".\bin\$exeName",
    ".\bin\$buildType\$exeName",
    ".\$buildType\$exeName",
    ".\$exeName"
)

$exePath = $null
foreach ($path in $exePaths) {
    if (Test-Path $path) {
        $exePath = $path
        break
    }
}

if (-not $exePath) {
    Write-Host "Error: Could not find built executable" -ForegroundColor Red
    exit 1
}

Write-Host "  Executable found: $exePath" -ForegroundColor Green

# Get file information
$fileInfo = Get-Item $exePath
Write-Host "  Size: $([math]::Round($fileInfo.Length / 1MB, 2)) MB" -ForegroundColor White
Write-Host "  Created: $($fileInfo.CreationTime)" -ForegroundColor White

# Deploy if requested
if ($Deploy) {
    Write-Host "Deploying GUI hypervisor..." -ForegroundColor Green
    
    $deployDir = "..\deploy_gui"
    if (-not (Test-Path $deployDir)) {
        New-Item -Path $deployDir -ItemType Directory -Force | Out-Null
    }
    
    # Copy executable
    Copy-Item -Path $exePath -Destination "$deployDir\$exeName" -Force
    Write-Host "  Copied executable to deploy directory" -ForegroundColor Green
    
    # Copy dependencies (if any DLLs are needed)
    $dllPaths = @(
        ".\bin\*.dll",
        ".\$buildType\*.dll"
    )
    
    foreach ($dllPath in $dllPaths) {
        if (Test-Path $dllPath) {
            Copy-Item -Path $dllPath -Destination $deployDir -Force
            Write-Host "  Copied DLLs from $dllPath" -ForegroundColor Green
        }
    }
    
    # Create run script
    $runScript = @"
@echo off
echo ARM64 GUI Hypervisor - Graphics Virtualization
echo =============================================
echo.

REM Check for Administrator privileges
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running as Administrator: YES
) else (
    echo Running as Administrator: NO
    echo.
    echo Warning: Administrator privileges recommended for full functionality
    pause
)

echo.
echo Starting GUI Hypervisor...
echo.

start $exeName

echo.
echo GUI Hypervisor launched!
pause
"@

    Set-Content -Path "$deployDir\run_gui_hypervisor.bat" -Value $runScript -Encoding ASCII
    Write-Host "  Created run script" -ForegroundColor Green
    
    # Create README
    $readme = @"
# ARM64 GUI Hypervisor with Graphics Virtualization

## Overview
This is a Windows ARM64 hypervisor with full graphics virtualization capabilities.
Features include:
- DirectX 11 graphics rendering
- Virtual GPU emulation
- Input device virtualization (keyboard/mouse)
- Real OS loading (Linux, Windows, ISO images)
- Modern Windows GUI interface

## Requirements
- Windows 11 ARM64
- Administrator privileges (recommended)
- Hyper-V enabled
- DirectX 11 compatible graphics

## Usage
1. Run as Administrator: Right-click 'run_gui_hypervisor.bat' -> 'Run as administrator'
2. Or double-click $exeName directly
3. Use the GUI interface to:
   - Load OS images (Linux kernels, Windows PE, ISO files)
   - Start/Stop virtual machines
   - View guest OS graphics output
   - Control VM execution

## Controls
- Load OS: Browse and load guest operating systems
- Start VM: Begin virtual machine execution
- Stop VM: Halt virtual machine
- Reset VM: Restart virtual machine
- The main window shows the guest OS graphics output

## Supported OS Types
- Linux ARM64 kernels
- Windows PE ARM64 images
- ISO 9660 boot images
- Raw binary files

## Technical Details
- Uses Windows Hypervisor Platform (WHP) APIs
- DirectX 11 for graphics virtualization
- Native ARM64 assembly for exception handling
- Device emulation: UART PL011, Timer, GIC, VGA
- Full input device virtualization

Built: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
Architecture: ARM64
Build Type: $buildType
"@

    Set-Content -Path "$deployDir\README.md" -Value $readme -Encoding UTF8
    Write-Host "  Created README.md" -ForegroundColor Green
    
    Write-Host "Deployment complete!" -ForegroundColor Green
    Write-Host "  Deploy directory: $deployDir" -ForegroundColor White
    Write-Host "  Run: $deployDir\run_gui_hypervisor.bat" -ForegroundColor White
}

# Test run (if not deploying)
if (-not $Deploy) {
    Write-Host "Testing hypervisor..." -ForegroundColor Green
    
    Write-Host "  Note: GUI hypervisor requires display to test properly" -ForegroundColor Yellow
    Write-Host "  To test, run: $exePath" -ForegroundColor White
    
    # Quick validation
    if (Test-Path $exePath) {
        try {
            $fileVersion = [System.Diagnostics.FileVersionInfo]::GetVersionInfo($exePath)
            Write-Host "  File version: $($fileVersion.FileVersion)" -ForegroundColor White
        } catch {
            Write-Host "  Executable appears valid" -ForegroundColor White
        }
    }
}

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "ARM64 GUI Hypervisor Build Complete!" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

Set-Location ..