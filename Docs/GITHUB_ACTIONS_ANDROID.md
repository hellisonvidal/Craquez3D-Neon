# GitHub Actions — Build Automático e Release do Instalador Android (`.apk`)

Este repositório conta com **2 workflows do GitHub Actions** prontos em `.github/workflows/`:

1. **`ci-validate-repo.yml`** *(roda na nuvem gratuita do GitHub a cada push)*:
   - Valida o `.uproject`, os 4 módulos do plugin `FootballCore`, regras da **UE 5.8.4**, shaders `.usf`, malhas `.fbx` dos uniformes, tipos seguros para Blueprint e configurações Android ARM64/Vulkan.
2. **`android-apk-release.yml`** *(compila o `Craquez-Android-ARM64.apk` e publica automaticamente em **GitHub Releases**)*:
   - Como a Unreal Engine 5.8.4 (~40 GB) não vem pré-instalada nos servidores gratuitos do GitHub, este workflow usa um **Self-Hosted Runner gratuito rodando no seu próprio PC Windows** (onde a UE 5.8.4 já está instalada!).

---

## Como conectar seu PC ao GitHub Actions (1 minuto, 100% gratuito)

1. Primeiro, garanta os 2 componentes Android instalados no seu PC:
   - **Epic Games Launcher** → UE 5.8.4 → **Opções** → marcar **Android** → **Aplicar**.
   - **Android Studio → SDK Manager** → instalar **Android API 36**, **NDK `27.2.12479018`**, **CMake `3.22.1`** e **Command-line Tools**.
2. No navegador, abra o repositório:
   **https://github.com/hellisonvidal/Craquez3D-Neon/settings/actions/runners/new**
3. Selecione **Windows** e copie os 3 comandos que o GitHub exibe no seu PowerShell (`mkdir actions-runner`, `./config.cmd`, `./run.cmd`).
4. Pronto! Agora basta ir na aba **Actions** do GitHub:
   - Clique em **`Craquez Android APK Build & Release (UE 5.8.4)`** → **`Run workflow`**!
   - *(Ou simplesmente crie uma tag `git tag v0.1.0-android && git push origin v0.1.0-android`)*
5. O GitHub Actions compila o **Craquez**, gera o **`Craquez-Android-ARM64.apk`**, sobe como artefato e cria automaticamente a **Release oficial com link direto de download para o celular** em:
   **https://github.com/hellisonvidal/Craquez3D-Neon/releases**!
