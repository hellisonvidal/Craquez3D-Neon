# Guia Completo de Iniciante — Unreal Engine 5.8.4 + Android (Craquez)

Bem-vindo! Este guia foi feito especialmente para quem **nunca abriu a Unreal Engine 5**. Siga os passos com calma — todo o código pesado do **Craquez** já está pronto neste repositório!

---

## Pré-requisito Rápido no Windows (Visual Studio + SDK)
Como o **Craquez** usa um plugin profissional em C++ (`FootballCore`), o Windows precisa do **Windows SDK** instalado no Visual Studio para compilar na primeira vez:

1. Abra o **Visual Studio Installer** no Menu Iniciar do Windows.
2. Clique em **Modificar (Modify)** no seu Visual Studio Community.
3. Na aba **Cargas de Trabalho (Workloads)**, marque:
   - **Desenvolvimento para desktop com C++** (*Desktop development with C++*)
   - **Desenvolvimento de jogos com C++** (*Game development with C++*)
4. Na aba **Componentes Individuais (Individual Components)**, confirme que está marcado:
   - **SDK do Windows 10 (10.0.19041.0)** ou **Windows 11 SDK (10.0.22621+)**
   - **Ferramentas de build MSVC v143 / mais recente (x64/x86)**
5. Clique em **Modificar** e aguarde finalizar.

---

## Passo 1 — Abrindo o Projeto Craquez na UE 5.8.4

1. Vá até a pasta onde baixou este repositório (`Craquez3D-Neon`).
2. Dê **duplo clique** no arquivo **`Craquez.uproject`**.
3. A UE 5.8.4 vai mostrar uma janelinha perguntando: *"The following modules are missing or built with a different engine version... Would you like to rebuild them now?"*
   - Clique em **Sim (Yes)**.
   - Na primeira vez, a compilação leva de 2 a 5 minutos. Depois disso, abre em segundos!

---

## Passo 2 — Jogando Imediatamente (11v11 e 1v1!)

Assim que o Editor da Unreal Engine 5.8.4 abrir:

1. Na barra superior, clique no botão verde **▶ Play (Alt + P)**.
2. **Pronto!** O `AFootballMatchGameMode` gera automaticamente:
   - O **Estádio Low-Poly** com gramado listrado, linhas oficiais, traves, redes e refletores;
   - A **Bola Oficial FIFA** com **Efeito Magnus**, arrasto, deformação elástica e colisão precisa;
   - **22 jogadores** com uniformes 3D separados (camisa, calção, meião, chuteira) no modo **Jogo-Treino 11vs11**;
   - **Placar Neon**, **Mini-Radar ao vivo** e barras de Estamina/Chute!

### Controles no Teclado / Controle (para testar no PC)
| Tecla | Ação |
|---|---|
| `W / A / S / D` ou Setas | Movimentar o jogador |
| `Segurar ESPAÇO` e soltar | **Chute com Curva (Efeito Magnus)** — quanto mais segura, mais forte! |
| `E` | **Passe Rasteiro** (ou trocar jogador na defesa) |
| `Q` | **Lançamento / Cavadinha** (com backspin) |
| `Shift Esquerdo` | **Correr (Sprint)** |
| `TAB` | **Trocar** para o companheiro mais próximo da bola |
| `M` | **Alternar Modo de Jogo**: `Jogo-Treino 11vs11` ↔ `Duelo 1vs1 (Atacante x Goleiro)` |
| `C` | **Mudar Clima** (`Dia` → `Pôr do Sol` → `Noite Refletores` → `Chuva Leve` → `Tempestade`) |

---

## Passo 3 — Como Criar sua Tela Touch para Android (UMG)

O código dos botões touch já está pronto em C++ (`UFootballHUDWidget`)! Para montar sua interface visual:

1. No **Content Browser** (parte inferior), clique direito → **User Interface → Widget Blueprint**.
2. Em *All Classes*, procure e escolha **`FootballHUDWidget`**. Nomeie como `WBP_CraquezMobileHUD`.
3. Abra o widget, arraste botões (`Button`) para o canto inferior direito e nomeie: `CHUTE`, `PASSE`, `CAVADINHA`, `CORRER`, `TROCAR`.
4. No evento `OnPressed` / `OnReleased` de cada botão, chame as funções prontas do plugin:
   - `TriggerShootPressed` / `TriggerShootReleased`
   - `TriggerPass`, `TriggerLobPass`, `TriggerSprint`, `TriggerSwitchPlayer`
   - `SelectMatchMode` (botões para escolher `11v11` ou `1v1`!)
5. Crie um Blueprint filho de `AFootballHUD`, selecione `WBP_CraquezMobileHUD` em **Custom HUD Widget Class**, e pronto!

---

## Passo 4 — Preparando e Gerando o APK para Celular Android

1. Instale o **Android Studio** (gratuito) e abra **SDK Manager**:
   - Instale **Android SDK Platform 34** (Android 14)
   - Em *SDK Tools*, marque **Android SDK Build-Tools**, **NDK (Side by side) 25.x/26.x**, **Android SDK Command-line Tools** e **CMake**.
2. Execute o script oficial da Unreal Engine que configura tudo automaticamente:
   - `C:\Program Files\Epic Games\UE_5.8\Engine\Extras\Android\SetupAndroid.bat`
3. Reinicie o Editor da UE 5.8.4.
4. No menu superior: **Edit → Project Settings → Platforms → Android**:
   - Clique em **Configure Now** (botão vermelho → ficará verde!).
   - Clique em **Accept SDK License**.
   - As demais configurações (`ARM64`, `Vulkan`, `Landscape`, `MinSDK 28`, `TargetSDK 34`) já foram pré-configuradas em `Config/DefaultEngine.ini`!
5. Conecte seu celular Android via cabo USB com **Depuração USB** ativada, ou gere o instalador:
   - Menu **Platforms → Android → Package Project** → escolha uma pasta.
   - Será gerado um `.apk` + script `Install_Craquez-arm64.bat` para instalar direto no celular!

---

## Dicas de Ouro para Iniciantes na UE5
- **Mover câmera no Editor**: Segure o **Botão Direito do Mouse** e use `W, A, S, D` (use a rodinha do mouse para ajustar a velocidade da câmera).
- **Salvar sempre**: Pressione `Ctrl + Shift + S` para salvar todos os níveis e Blueprints.
- **Testar visual Mobile no PC**: No menu **Settings → Preview Rendering Level → Android Vulkan**, você vê exatamente como ficará no celular!
