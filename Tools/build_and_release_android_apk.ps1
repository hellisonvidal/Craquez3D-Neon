<#
.SYNOPSIS
    Empacota o Craquez (UE 5.8.4) como instalador Android (.apk ARM64 ASTC)
    e opcionalmente publica uma Release no GitHub (hellisonvidal/Craquez3D-Neon).

.EXAMPLE
    .\Tools\build_and_release_android_apk.ps1
    .\Tools\build_and_release_android_apk.ps1 -ReleaseTag "v0.1.0-android-pilot" -PublishRelease
#>
param(
    [string]$EngineRoot = "C:\Program Files\Epic Games\UE_5.8",
    [string]$AndroidSdk = "",
    [string]$NdkRoot    = "",
    [string]$JavaHome   = "C:\Program Files\Android\openjdk\jdk-21.0.8",
    [string]$ReleaseTag = "ue5-v0.1.0-android",
    [switch]$PublishRelease
)

$ErrorActionPreference = "Stop"
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$UProject    = Join-Path $ProjectRoot "Craquez.uproject"
$ArchiveDir  = Join-Path $ProjectRoot "Build\Android_ASTC"

Write-Host "=== CRAQUEZ 3D (UE 5.8.4) — BUILD INSTALADOR ANDROID NATIVO (.APK) ===" -ForegroundColor Cyan

# Auto-detect Android SDK (Visual Studio / Program Files x86 or LocalAppData)
if ([string]::IsNullOrWhiteSpace($AndroidSdk)) {
    if (Test-Path "C:\Program Files (x86)\Android\android-sdk\platforms\android-36") {
        $AndroidSdk = "C:\Program Files (x86)\Android\android-sdk"
    } else {
        $AndroidSdk = "$env:LOCALAPPDATA\Android\Sdk"
    }
}

# 1. Validar Target Platform Android na UE 5.8.4
$AndroidBinaries = Join-Path $EngineRoot "Engine\Binaries\Android"
if (!(Test-Path $AndroidBinaries)) {
    Write-Error @"
[FALTA COMPONENTE UE 5.8.4] A plataforma Android (Engine\Binaries\Android) nao esta instalada!
Como ativar (leva ~5 min):
  1. Abra o Epic Games Launcher -> Unreal Engine -> Biblioteca
  2. Na UE 5.8.4, clique na seta ao lado de 'Iniciar' -> Opcoes (Options)
  3. Marque: Plataformas de Destino -> Android
  4. Clique em Aplicar e rode este script novamente!
"@
}

# 2. Auto-detect Android NDK r27c (Visual Studio Program Files x86 or SDK\ndk)
if ([string]::IsNullOrWhiteSpace($NdkRoot)) {
    if (Test-Path "C:\Program Files (x86)\Android\AndroidNDK\android-ndk-r27c") {
        $NdkRoot = "C:\Program Files (x86)\Android\AndroidNDK\android-ndk-r27c"
    } else {
        $NdkDir = Get-ChildItem "$AndroidSdk\ndk" -Directory -ErrorAction SilentlyContinue | Sort-Object Name -Descending | Select-Object -First 1
        if ($NdkDir) { $NdkRoot = $NdkDir.FullName }
    }
}

if ([string]::IsNullOrWhiteSpace($NdkRoot) -or !(Test-Path $NdkRoot)) {
    Write-Error "Android NDK r27c nao encontrado! Verifique C:\Program Files (x86)\Android\AndroidNDK\android-ndk-r27c ou $AndroidSdk\ndk."
}

$env:ANDROID_HOME     = $AndroidSdk
$env:ANDROID_SDK_ROOT = $AndroidSdk
$env:NDKROOT          = $NdkRoot
$env:ANDROID_NDK_ROOT = $NdkRoot
if (Test-Path $JavaHome) {
    $env:JAVA_HOME = $JavaHome
}

Write-Host "ANDROID_HOME = $env:ANDROID_HOME" -ForegroundColor Green
Write-Host "NDKROOT      = $env:NDKROOT" -ForegroundColor Green
Write-Host "JAVA_HOME    = $env:JAVA_HOME" -ForegroundColor Green

New-Item -ItemType Directory -Force -Path $ArchiveDir | Out-Null

# 3. Rodar RunUAT BuildCookRun para Android ARM64 (ASTC)
$RunUAT = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"
& $RunUAT BuildCookRun `
    "-project=$UProject" `
    -noP4 `
    -platform=Android `
    -cookflavor=ASTC `
    -clientconfig=Development `
    -build -cook -allmaps -stage -pak -archive `
    "-archivedirectory=$ArchiveDir"

$ApkFile = Get-ChildItem $ArchiveDir -Recurse -Filter "*.apk" | Select-Object -First 1
if (!$ApkFile) {
    Write-Error "Build concluiu mas nenhum arquivo .apk foi encontrado em $ArchiveDir"
}

Write-Host "`n[SUCESSO] APK gerado em: $($ApkFile.FullName)" -ForegroundColor Green

# 4. Publicar Release no GitHub com o instalador .apk anexado
if ($PublishRelease) {
    Write-Host "Publicando Release $ReleaseTag no GitHub..." -ForegroundColor Cyan
    gh release create $ReleaseTag $ApkFile.FullName `
        --repo hellisonvidal/Craquez3D-Neon `
        --title "Craquez $ReleaseTag (Instalador Android APK)" `
        --notes "Piloto jogavel Android ARM64 (Vulkan/ASTC) compilado na UE 5.8.4 com 11vs11 Jogo-Treino, 1vs1 Atacante x Goleiro, Fisica Magnus, IA StateTree (Press->Intercept corrigido) e controles Touch nativos."
    Write-Host "[SUCESSO] Release publicada em https://github.com/hellisonvidal/Craquez3D-Neon/releases/tag/$ReleaseTag" -ForegroundColor Green
}
