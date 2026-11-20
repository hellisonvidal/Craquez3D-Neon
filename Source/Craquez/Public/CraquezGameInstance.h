// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "FootballTypes.h"
#include "CraquezGameInstance.generated.h"

/**
 * Persistent GameInstance for Craquez managing selected Match Mode,
 * Weather preference, and Android/Desktop scalability presets.
 */
UCLASS(Blueprintable, BlueprintType)
class CRAQUEZ_API UCraquezGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	/** Apply mobile-tuned graphics preset (0=Android Performance, 1=Android Balanced, 2=High, 3=Ultra/PC) */
	UFUNCTION(BlueprintCallable, Category = "Craquez|Settings")
	void ApplyGraphicsPreset(int32 QualityLevel);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craquez|Settings")
	EFootballMatchMode PreferredMatchMode = EFootballMatchMode::Training11v11;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Craquez|Settings")
	EFootballWeatherType PreferredWeather = EFootballWeatherType::NightFloodlights;
};
