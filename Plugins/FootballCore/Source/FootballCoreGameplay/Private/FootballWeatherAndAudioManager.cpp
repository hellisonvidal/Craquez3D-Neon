// Copyright Craquez Studio. All Rights Reserved.

#include "FootballWeatherAndAudioManager.h"
#include "FootballBallActor.h"
#include "FootballCharacter.h"
#include "FootballUniformComponent.h"
#include "FootballStadiumGenerator.h"
#include "NiagaraComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

AFootballWeatherAndAudioManager::AFootballWeatherAndAudioManager()
{
	PrimaryActorTick.bCanEverTick = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	RainNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("RainNiagaraComponent"));
	RainNiagaraComponent->SetupAttachment(RootComponent);
	RainNiagaraComponent->bAutoActivate = false;

	NorthStandChantAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("NorthStandChantAudio"));
	NorthStandChantAudio->SetupAttachment(RootComponent);
	NorthStandChantAudio->SetRelativeLocation(FVector(0.0f, -4200.0f, 600.0f));
	NorthStandChantAudio->bAutoActivate = false;

	SouthStandChantAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("SouthStandChantAudio"));
	SouthStandChantAudio->SetupAttachment(RootComponent);
	SouthStandChantAudio->SetRelativeLocation(FVector(0.0f, 4200.0f, 600.0f));
	SouthStandChantAudio->bAutoActivate = false;
}

void AFootballWeatherAndAudioManager::BeginPlay()
{
	Super::BeginPlay();

	// Bind to ball impact events for positional 3D sound playback
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFootballBallActor> It(World); It; ++It)
		{
			It->OnBallImpact.AddDynamic(this, &AFootballWeatherAndAudioManager::HandleBallImpactEvent);
		}
	}

	ApplyWeatherPreset(CurrentWeather);
}

void AFootballWeatherAndAudioManager::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Smoothly decay crowd excitement back to ambient stadium hum (0.22)
	CrowdExcitement = FMath::FInterpTo(CrowdExcitement, 0.22f, DeltaTime, 0.18f);

	const float CrowdVol = FMath::Lerp(0.35f, 1.0f, CrowdExcitement);
	const float CrowdPitch = FMath::Lerp(0.96f, 1.08f, CrowdExcitement);

	if (NorthStandChantAudio && NorthStandChantAudio->IsPlaying())
	{
		NorthStandChantAudio->SetVolumeMultiplier(CrowdVol);
		NorthStandChantAudio->SetPitchMultiplier(CrowdPitch);
	}
	if (SouthStandChantAudio && SouthStandChantAudio->IsPlaying())
	{
		SouthStandChantAudio->SetVolumeMultiplier(CrowdVol);
		SouthStandChantAudio->SetPitchMultiplier(CrowdPitch);
	}
}

void AFootballWeatherAndAudioManager::ApplyWeatherPreset(EFootballWeatherType NewWeather)
{
	CurrentWeather = NewWeather;

	float FloodlightLevel = 0.0f;
	bool bEnableRain = false;

	switch (CurrentWeather)
	{
	case EFootballWeatherType::ClearDay:
		ActiveWindVelocityCmPerSec = FVector(80.0f, 40.0f, 0.0f);
		ActivePitchWetness = 0.0f;
		FloodlightLevel = 0.0f;
		break;

	case EFootballWeatherType::GoldenHour:
		ActiveWindVelocityCmPerSec = FVector(150.0f, -90.0f, 0.0f);
		ActivePitchWetness = 0.1f;
		FloodlightLevel = 0.55f;
		break;

	case EFootballWeatherType::NightFloodlights:
		ActiveWindVelocityCmPerSec = FVector(110.0f, 60.0f, 0.0f);
		ActivePitchWetness = 0.20f; // Evening dew
		FloodlightLevel = 1.0f;
		break;

	case EFootballWeatherType::LightRain:
		ActiveWindVelocityCmPerSec = FVector(280.0f, 190.0f, 0.0f);
		ActivePitchWetness = 0.65f;
		FloodlightLevel = 1.0f;
		bEnableRain = true;
		break;

	case EFootballWeatherType::StormRain:
		ActiveWindVelocityCmPerSec = FVector(520.0f, -340.0f, 0.0f);
		ActivePitchWetness = 1.0f;
		FloodlightLevel = 1.0f;
		bEnableRain = true;
		break;
	}

	if (RainNiagaraComponent)
	{
		if (bEnableRain && RainNiagaraComponent->GetAsset())
		{
			RainNiagaraComponent->Activate(true);
			RainNiagaraComponent->SetFloatParameter(TEXT("RainIntensity"), ActivePitchWetness);
			RainNiagaraComponent->SetVectorParameter(TEXT("WindVelocity"), ActiveWindVelocityCmPerSec);
		}
		else
		{
			RainNiagaraComponent->Deactivate();
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFootballStadiumGenerator> It(World); It; ++It)
		{
			It->SetFloodlightsIntensity(FloodlightLevel);
		}
	}

	PropagateEnvironmentToWorld();
}

void AFootballWeatherAndAudioManager::CycleNextWeather()
{
	const uint8 NextIdx = (static_cast<uint8>(CurrentWeather) + 1) % 5;
	ApplyWeatherPreset(static_cast<EFootballWeatherType>(NextIdx));
}

void AFootballWeatherAndAudioManager::AddCrowdExcitement(float DeltaExcitement)
{
	CrowdExcitement = FMath::Clamp(CrowdExcitement + DeltaExcitement, 0.0f, 1.0f);
	if (DeltaExcitement >= 0.7f && GoalRoarSound && GetWorld())
	{
		UGameplayStatics::PlaySound2D(GetWorld(), GoalRoarSound, 1.0f, 1.0f);
	}
}

void AFootballWeatherAndAudioManager::PlayPositionalBallImpact(EFootballBallSurface Surface, float ImpactSpeedCmPerSec, const FVector& WorldLocation)
{
	if (!GetWorld())
	{
		return;
	}

	USoundBase* ChosenSound = nullptr;
	switch (Surface)
	{
	case EFootballBallSurface::PlayerBoot:
	case EFootballBallSurface::KeeperGlove:
		ChosenSound = KickImpactSound;
		break;
	case EFootballBallSurface::Post:
		ChosenSound = PostClangSound;
		AddCrowdExcitement(0.45f); // Dramatic crossbar hit!
		break;
	case EFootballBallSurface::Net:
		ChosenSound = NetSwishSound;
		break;
	default:
		ChosenSound = TurfBounceSound;
		break;
	}

	if (ChosenSound)
	{
		const float Vol = FMath::Clamp(ImpactSpeedCmPerSec / 2600.0f, 0.15f, 1.0f);
		const float Pitch = FMath::Lerp(0.88f, 1.15f, Vol);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ChosenSound, WorldLocation, Vol, Pitch, 0.0f, StadiumSpatialAttenuation);
	}
}

void AFootballWeatherAndAudioManager::HandleBallImpactEvent(EFootballBallSurface Surface, float ImpactSpeed, FVector ImpactLocation)
{
	PlayPositionalBallImpact(Surface, ImpactSpeed, ImpactLocation);
}

void AFootballWeatherAndAudioManager::PropagateEnvironmentToWorld()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AFootballBallActor> It(World); It; ++It)
	{
		It->SetEnvironmentConditions(ActiveWindVelocityCmPerSec, ActivePitchWetness);
	}

	for (TActorIterator<AFootballCharacter> It(World); It; ++It)
	{
		if (It->UniformComponent)
		{
			It->UniformComponent->SetFabricWetness(ActivePitchWetness);
		}
	}
}
