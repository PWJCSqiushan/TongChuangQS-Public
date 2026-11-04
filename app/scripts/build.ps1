# Build the source archive on Windows 10/11 x64; no machine-specific paths.
param([string]$QtRoot='',[switch]$Test,[switch]$NoGui,[string]$Without='')
$ErrorActionPreference='Stop'
$app=Split-Path -Parent $PSScriptRoot
$root=Split-Path -Parent $app
function Invoke-QS([string]$Exe,[string[]]$Arguments) {
    & $Exe @Arguments
    if($LASTEXITCODE -ne 0){throw "$Exe failed (exit $LASTEXITCODE)"}
}
function Get-AsciiQS([string]$Path) {
    $full=[IO.Path]::GetFullPath($Path).TrimEnd('\','/')
    if($full -notmatch '[^\x00-\x7f]'){return $full}
    $base=Join-Path $env:TEMP 'TongChuangQS-build-links'
    if($base -match '[^\x00-\x7f]'){throw 'Use an ASCII TEMP directory for CMake/MSVC.'}
    New-Item -ItemType Directory -Path $base -Force | Out-Null
    $hash=[Security.Cryptography.SHA256]::Create()
    try{$tag=[BitConverter]::ToString($hash.ComputeHash([Text.Encoding]::UTF8.GetBytes($full.ToLowerInvariant()))).Replace('-','').Substring(0,16)}finally{$hash.Dispose()}
    $link=Join-Path $base $tag
    if(Test-Path -LiteralPath $link){
        $item=Get-Item -LiteralPath $link -Force
        if($item.LinkType -ne 'Junction' -or [IO.Path]::GetFullPath([string]$item.Target).TrimEnd('\','/') -ne $full){throw "Unexpected existing junction: $link"}
    }else{New-Item -ItemType Junction -Path $link -Target $full | Out-Null}
    return $link
}
$vswhere=Join-Path ([Environment]::GetFolderPath('ProgramFilesX86')) 'Microsoft Visual Studio\Installer\vswhere.exe'
if(!(Test-Path -LiteralPath $vswhere)){throw 'Install Visual Studio 2022 with Desktop development with C++.'}
$vs=& $vswhere -latest -version '[17.0,18.0)' -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if(!$vs){throw 'Visual Studio 2022 C++ x64 toolchain was not found.'}
$cmake=Join-Path $vs 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe'
$ctest=Join-Path (Split-Path -Parent $cmake) 'ctest.exe'
$alias=Get-AsciiQS $root
if(!$NoGui){
    if(!$QtRoot){throw 'Specify -QtRoot for a GUI build (Qt 6.8.x msvc2022_64).'}
    $qt=(Resolve-Path -LiteralPath $QtRoot).Path
    if(!(Test-Path -LiteralPath (Join-Path $qt 'lib\cmake\Qt6\Qt6Config.cmake'))){throw 'QtRoot must point to Qt 6.8.x msvc2022_64.'}
    $qtAlias=Get-AsciiQS $qt
}
$suffix=if($Without){'-without-'+$Without}else{''}
$build=Join-Path $alias ('build'+$suffix)
$gui=if($NoGui){'OFF'}else{'ON'}
$argsQS=@('-S',(Join-Path $alias 'app'),'-B',$build,'-G','Visual Studio 17 2022','-A','x64',"-DCMAKE_GENERATOR_INSTANCE=$vs","-DQS_BUILD_GUI=$gui",'-DQS_BUILD_TESTS=ON',"-DQS_DISABLED_SERVICES=$Without")
if(!$NoGui){$argsQS+="-DCMAKE_PREFIX_PATH=$qtAlias"}
Invoke-QS $cmake $argsQS
Invoke-QS $cmake @('--build',$build,'--config','Release','--parallel')
if($Test){Invoke-QS $ctest @('--test-dir',$build,'-C','Release','--output-on-failure')}
$runtime=Join-Path $root ('runtime'+$suffix)
New-Item -ItemType Directory -Path $runtime -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $build 'Release\SocialQS.exe') -Destination $runtime -Force
if(!$NoGui){
    Copy-Item -LiteralPath (Join-Path $build 'Release\TongChuangQS.exe') -Destination $runtime -Force
    Copy-Item -LiteralPath (Join-Path $app 'resources\TongChuangQS.png') -Destination $runtime -Force
    Invoke-QS (Join-Path $qtAlias 'bin\windeployqt.exe') @('--release','--no-patchqt','--no-translations','--no-compiler-runtime','--no-opengl-sw','--no-system-d3d-compiler','--no-system-dxc-compiler','--skip-plugin-types','generic,networkinformation,tls',(Join-Path $runtime 'TongChuangQS.exe'))
    "[Paths]`nPrefix=.`nPlugins=.`n" | Set-Content -LiteralPath (Join-Path $runtime 'qt.conf') -Encoding ASCII
}
$redist=Get-ChildItem -LiteralPath (Join-Path $vs 'VC\Redist\MSVC') -Directory | Where-Object {$_.Name -match '^\d+\.'} | Sort-Object Name -Descending | Select-Object -First 1
if(!$redist){throw 'The VS2022 x64 app-local C++ runtime was not found.'}
$crt=Join-Path $redist.FullName 'x64\Microsoft.VC143.CRT'
Get-ChildItem -LiteralPath $crt -Filter '*.dll' -File | Copy-Item -Destination $runtime -Force
Write-Output "Build complete: $runtime"
# No files are deleted automatically. If a stale cache is reported, move the
# build directory to the Recycle Bin yourself, then rerun this script.
