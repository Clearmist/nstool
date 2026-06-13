$pf86 = [Environment]::GetEnvironmentVariable('ProgramFiles(x86)')
$vswhere = Join-Path $pf86 'Microsoft Visual Studio\Installer\vswhere.exe'

if (-not (Test-Path $vswhere)) {
    Write-Error "vswhere.exe not found at '$vswhere'"
    exit 1
}

Write-Host "Using vswhere at: $vswhere"

# Try to find MSBuild.exe via vswhere
$msbuildCandidates = & $vswhere `
    -latest `
    -products * `
    -find 'MSBuild\**\MSBuild.exe'

Write-Host "vswhere MSBuild candidates:"
$msbuildCandidates | ForEach-Object { Write-Host "  $_" }

$msbuild = $msbuildCandidates | Select-Object -First 1

# Fallback to the path you know works (from your original command)
$defaultMsBuild = 'C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\MSBuild\Current\Bin\MSBuild.exe'

if (-not $msbuild -and (Test-Path $defaultMsBuild)) {
    Write-Host "vswhere did not return MSBuild. Falling back to known path:"
    Write-Host "  $defaultMsBuild"
    $msbuild = $defaultMsBuild
}

if (-not $msbuild) {
    Write-Error "MSBuild.exe not found via vswhere or fallback path"
    exit 1
}

Write-Host "Using MSBuild at: $msbuild"

# Locate the solution relative to this script
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$solution = Join-Path $scriptDir '..\build\visualstudio\nstool.sln'

Write-Host "Building solution: $solution"

& $msbuild $solution /t:Clean
