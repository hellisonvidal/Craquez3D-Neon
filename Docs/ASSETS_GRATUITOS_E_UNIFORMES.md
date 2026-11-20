# Guia de Assets 100% Gratuitos e Uniformes 3D Reais (Não Pintados no Corpo)

No **Craquez**, os uniformes **NÃO são pintados na textura da pele**! O componente `UFootballUniformComponent` foi arquitetado para usar **peças de roupa 3D separadas** (`JerseySkeletalMesh`, `ShortsSkeletalMesh`, `SocksSkeletalMesh`, `BootsSkeletalMesh`, `GlovesSkeletalMesh`) acopladas ao esqueleto do jogador via `SetLeaderPoseComponent` e simuladas com **Chaos Cloth** (com LOD automático por distância para manter 60 FPS no Android).

---

## 1. Uniformes 3D Prontos para Importar (Gerados por Nós!)

Incluímos o script **`Tools/blender_generate_kit_meshes.py`** que gera automaticamente as malhas 3D separadas em formato `.fbx` (camisa com mangas e subdivisão para Chaos Cloth, calção solto, meião, chuteira e bola FIFA 11 cm):
- `Art/GeneratedFBX/SM_Craquez_Jersey.fbx`
- `Art/GeneratedFBX/SM_Craquez_Shorts.fbx`
- `Art/GeneratedFBX/SM_Craquez_Socks.fbx`
- `Art/GeneratedFBX/SM_Craquez_Boots.fbx`
- `Art/GeneratedFBX/SM_Craquez_Ball.fbx`

### Como aplicar Chaos Cloth na Camisa dentro da UE 5.8.4
1. Importe `SM_Craquez_Jersey.fbx` vinculando ao esqueleto do seu jogador (UE5 Manny / MetaHuman / Mixamo).
2. Abra o Skeletal Mesh da camisa → Menu **Section Selection** → Clique direito na malha da camisa → **Create Clothing Data from Section**.
3. Clique em **Activate Cloth Paint** e pinte:
   - **0 cm (roxo/fixo)** nos ombros e gola (para a camisa não cair do corpo);
   - **3 cm a 8 cm (branco/livre)** na barra inferior da camisa e pontas das mangas (para balançar com o vento e corrida!).
4. Atribua a camisa ao slot `JerseySkeletalMesh` do `UFootballUniformComponent`. O código C++ desliga automaticamente a simulação de tecido em jogadores longe da câmera (`ClothLODDisableDistanceCm = 2200 cm`) para economizar bateria no Android!

---

## 2. Catálogo de Assets 100% Gratuitos na Internet (Licença Livre/Comercial)

| Categoria | Fonte Gratuita | O que Baixar | Como Usar no Craquez |
|---|---|---|---|
| **Jogadores Realistas** | **MetaHuman Creator** (gratuito dentro da UE 5.8 via plugin `MetaHumanCharacter`) | Crie atletas realistas (pele com **Subsurface Scattering**, cabelo groom/cards) | Exporte com **LOD 2/3 para Android** e oculte o torso interno sob a `JerseySkeletalMesh` para evitar penetração (*clipping*). |
| **Animações de Futebol** | **Mixamo.com** (100% gratuito com conta Adobe) | Pesquise *"Soccer"* / *"Football"*: `Soccer Idle`, `Soccer Run`, `Soccer Pass`, `Soccer Tackle`, `Goalkeeper Catch`, `Goalkeeper Dive`, `Bicycle Kick` | Baixe em `.fbx` (*Without Skin*), faça **IK Retargeting** no UE 5.8 para o esqueleto Manny/MetaHuman e alimente o **Motion Matching (`PoseSearch`)**! |
| **Grama, Concreto & Materiais PBR** | **Fab.com / Quixel Megascans** (gratuito para Unreal Engine!) | *"Sports Turf Grass"*, *"Wet Asphalt/Concrete"*, *"Fabric Jersey Cotton/Polyester"*, *"Leather Soccer Ball"* | Aplique na malha da pista, arquibancadas e no `M_Craquez_Jersey` (conectando o parâmetro `Wetness` do clima!). |
| **Iluminação / Céu HDRI** | **PolyHaven.com** (CC0 — Domínio Público) | HDRIs *"Stadium Night"*, *"Sunset Field"*, *"Overcast Sky"* | Use no `SkyLight` do estádio para reflexos PBR realistas na bola e nas chuteiras. |
| **Áudio Posicional & Torcida** | **Freesound.org** (filtrar por licença **CC0**) | *"soccer ball kick"*, *"stadium crowd cheer loop"*, *"goalpost metal hit"*, *"whistle referee"*, *"rain stadium"* | Arraste os arquivos `.wav` direto para as propriedades `KickImpactSound`, `PostClangSound`, `NetSwishSound`, `GoalRoarSound` e `North/SouthStandChantAudio` do `AFootballWeatherAndAudioManager`! |
| **Modelos 3D Gratuitos Extras** | **Sketchfab** (filtro *Downloadable + CC0/CC-BY*) & **Kenney.nl** | Troféus, refletores detalhados, placas de LED, bancos de reserva | Importe em `/Content/Craquez/Stadium/Props/` e posicione com **PCG**. |
