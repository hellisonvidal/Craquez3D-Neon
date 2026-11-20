# Arquitetura Avançada UE 5.8.4 — Craquez

Este documento explica como os 5 pilares modernos da Unreal Engine 5.8.4 foram pesquisados, projetados e integrados ao código do plugin **`FootballCore`** para escalar de **Android (60 FPS)** até **PC / Consoles Ultra**.

---

## 1. Niagara + Compute Shaders (Geometria & Partículas na GPU)
- **Onde está no projeto**:
  - `Plugins/FootballCore/Shaders/FootballGPUGrass.usf` (Compute Shader de grama reativa ao vento e às chuteiras dos 22 jogadores + rastro da bola).
  - `Plugins/FootballCore/Shaders/FootballGPUCrowdVAT.usf` (Shader GPU de torcida animada sem ossos — suporta 20.000+ torcedores pulando conforme `CrowdExcitement` a 60 FPS no Android!).
  - `Plugins/FootballCore/Shaders/FootballBallDeformationWPO.usf` (Deformação elástica da bola preservando volume via *World Position Offset*).
  - `AFootballBallActor::MagnusTrailNiagara` & `AFootballWeatherAndAudioManager::RainNiagaraComponent`.
- **Como funciona**: Em vez de simular milhares de lâminas de grama ou torcedores na CPU, o Niagara roda em **GPU Simulation Stages** usando `Grid2D` / `StructuredBuffer`, lendo a posição da bola e dos jogadores enviada pelo C++.

---

## 2. Control Rig / Animation Blueprint (IK Procedural, Motion Warping, Motion Matching & Euphoria)
- **Onde está no projeto**: `UFootballProceduralAnimComponent` (`FootballProceduralAnimComponent.h/.cpp`).
- **4 camadas combinadas**:
  1. **Foot Placement IK (Control Rig)**: Raycasts duplos nos pés (`LeftFootIKLocation`, `RightFootIKLocation`, `PelvisZOffset`) garantem pés cravados na grama sem flutuar.
  2. **Motion Warping no Chute**: `PrepareKickMotionWarp()` registra o alvo `KickBallTarget` no `UMotionWarpingComponent`, ajustando dinamicamente o passo do pé de apoio e o contato do pé de chute com a bola em movimento (chute colocado, peito do pé ou trivela).
  3. **Motion Matching (`PoseSearch`)**: `ComputeMotionMatchingTrajectory()` gera amostras futuras (`0.15s`, `0.35s`, `0.60s`, `0.90s`) prontas para o nó `Motion Matching` da UE 5.8 escolher automaticamente a animação mais natural de drible, corte e arrancada.
  4. **Animação Baseada em Física (Euphoria-Style)**: `TriggerEuphoriaImpact()` ativa física parcial (`SetAllBodiesBelowSimulatePhysics` / `PhysicsControl`) na coluna e braços durante divididas de ombro e saltos do goleiro, recuperando o equilíbrio organicamente!

---

## 3. Chaos Physics (Bola Aerodinâmica, Cloth de Camisas & Rede do Gol)
- **Onde está no projeto**: `AFootballBallActor`, `UFootballUniformComponent`, `Config/DefaultEngine.ini`.
- **Configuração Chaos Async Fixed-Step** (`120 Hz` / `0.008333s` + `bEnableEnhancedDeterminism=True`):
  - **Efeito Magnus**: $\vec{F}_M = \frac{1}{2} C_L \rho A r (\vec{\omega} \times \vec{v})$ gerando curvas reais (batida colocada, folha seca, trivela).
  - **Arrasto Quadrático + Vento Dinâmico**: $\vec{F}_D = -\frac{1}{2} C_D \rho A |\vec{v}_{rel}| \vec{v}_{rel}$.
  - **Atrito de Grama Molhada & Transferência de Spin no Quique**: Em dias de chuva (`LightRain` / `StormRain`), a bola desliza mais rápido e o topspin acelera ao tocar o gramado.
  - **CCD (`bUseCCD = true`)**: Chutes a 120 km/h nunca atravessam a trave ou a luva do goleiro.
  - **Chaos Cloth**: Camisas e calções simulam vento e corrida, com auto-LOD a 22m da câmera para o piloto Android.

---

## 4. MetaHuman (Framework Procedural de Atletas)
- **Plugin nativo na UE 5.8.4**: `MetaHumanCharacter` & `MetaHumanCrowd` já estão habilitados na sua instalação da UE 5.8!
- **Pipeline para Android + Multiplataforma**:
  - **Subsurface Scattering (Pele)**: Perfil `Subsurface Profile` ativo em `DefaultEngine.ini` (`r.SubsurfaceScattering=1`) com escalabilidade automática (`r.SubsurfaceQuality=1` no Android Balanced, `2` no PC).
  - **Cabelo (Groom vs Hair Cards)**: Para Android, exporte o MetaHuman em **LOD 2/3 (Hair Cards)** para máximo desempenho; no futuro port para PC/Console, ative **LOD 0 (Groom Strands)** no mesmo personagem!
  - **Encaixe de Uniformes**: O `UFootballUniformComponent` vincula a camisa/calção/meião diretamente ao esqueleto do MetaHuman via `SetLeaderPoseComponent`.

---

## 5. PCG (Procedural Content Generation — Estádio, Grama & Multidão)
- **Onde está no projeto**: `AFootballStadiumGenerator` + plugin `PCG` habilitado no `Craquez.uproject`.
- **Como expandir o estádio low-poly usando PCG na UE 5.8.4**:
  1. Crie um **PCG Graph** (`PCG_CraquezStadiumCrowd`): use `Surface Sampler` nos degraus das arquibancadas geradas pelo `AFootballStadiumGenerator` + `Static Mesh Spawner` apontando para o torcedor low-poly com o material `FootballGPUCrowdVAT.usf` → **15.000 torcedores em 1 draw call!**
  2. Crie um **PCG Graph** (`PCG_CraquezGrass`): espalhe tufos de grama 3D apenas nas bordas/close-ups (com cull distance de 15m no Android) combinados com o shader de listras do gramado.

---

## 6. Áudio Posicional & Sistema de Clima
- **Reverb de Estádio**: Use um `AudioGameplayVolume` cobrindo o estádio (`120m x 85m x 40m`) vinculado a um `Submix Reverb` (Decay Time ~`2.4s`, Wet Level `0.35`) para eco autêntico de arena aberta.
- **Impactos Dinâmicos**: `AFootballWeatherAndAudioManager` escala volume e pitch de chutes, traves metálicas (`PostClangSound`), rede (`NetSwishSound`) e quiques na grama conforme a velocidade real do impacto em cm/s.
- **Torcida Reativa**: Emitters 3D nas arquibancadas Norte e Sul aumentam intensidade e tom automaticamente quando a bola acerta a trave ou sai gol!
