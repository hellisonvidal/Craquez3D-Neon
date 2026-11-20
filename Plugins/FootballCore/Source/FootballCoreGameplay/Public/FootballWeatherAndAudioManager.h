// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballTypes.h"
#include "FootballWeatherAndAudioManager.generated.h"

class UNiagaraComponent;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;

/**
 * Dynamic Stadium Weather & Positional 3D Audio Controller for Craquez:
 * - Weather presets (ClearDay, GoldenHour, NightFloodlights, LightRain, StormRain)
 * - Drives ambient Wind vector into Ball Magnus/Drag & Jersey Chaos Cloth
 * - Drives Pitch Wetness into Turf Friction & Kit Material Darkening
 * - Positional Audio: Stadium Reverb scaling, velocity-mapped ball impact sounds (kick/post/net/turf),
 *   and 3D spatialized crowd chants driven dynamically by match excitement [0..1]
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREGAMEPLAY_API AFootballWeatherAndAudioManager : public AActor
{
	GENERATED_BODY()

public:
	AFootballWeatherAndAudioManager();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	UFUNCTION(BlueprintCallable, Category = "Football|Weather")
	void ApplyWeatherPreset(EFootballWeatherType NewWeather);

	UFUNCTION(BlueprintCallable, Category = "Football|Weather")
	void CycleNextWeather();

	/** Spike crowd excitement (e.g., +0.35 on shot on target, +1.0 on GOAL!) */
	UFUNCTION(BlueprintCallable, Category = "Football|Audio")
	void AddCrowdExcitement(float DeltaExcitement);

	/** Trigger velocity-scaled 3D positional ball impact sound with stadium reverb send */
	UFUNCTION(BlueprintCallable, Category = "Football|Audio")
	void PlayPositionalBallImpact(EFootballBallSurface Surface, float ImpactSpeedCmPerSec, const FVector& WorldLocation);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Weather")
	EFootballWeatherType CurrentWeather = EFootballWeatherType::NightFloodlights;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Weather")
	FVector ActiveWindVelocityCmPerSec = FVector(120.0f, 65.0f, 0.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Football|Weather")
	float ActivePitchWetness = 0.15f;

	/** Reactive crowd excitement [0..1] driving stadium chant volume & pitch */
	UPROPERTY(BlueprintReadOnly, Category = "Football|Audio")
	float CrowdExcitement = 0.25f;

	/** Niagara GPU rain / stadium mist emitter */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Weather")
	TObjectPtr<UNiagaraComponent> RainNiagaraComponent;

	/** 3D spatialized crowd chant emitters positioned around North/South stands */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Audio")
	TObjectPtr<UAudioComponent> NorthStandChantAudio;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Audio")
	TObjectPtr<UAudioComponent> SouthStandChantAudio;

	// Optional free SoundBase assets assignables in Blueprint (Freesound CC0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Audio|Assets")
	TObjectPtr<USoundBase> KickImpactSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Audio|Assets")
	TObjectPtr<USoundBase> PostClangSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Audio|Assets")
	TObjectPtr<USoundBase> NetSwishSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Audio|Assets")
	TObjectPtr<USoundBase> TurfBounceSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Audio|Assets")
	TObjectPtr<USoundBase> GoalRoarSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Audio|Assets")
	TObjectPtr<USoundAttenuation> StadiumSpatialAttenuation;

protected:
	UFUNCTION()
	void HandleBallImpactEvent(EFootballBallSurface Surface, float ImpactSpeed, FVector ImpactLocation);

	void PropagateEnvironmentToWorld();
};
