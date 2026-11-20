// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FootballTypes.h"
#include "FootballBlueprintInterface.generated.h"

/**
 * Blueprint-accessible gameplay contract for Craquez actors (Players, Ball, Stadium, UI).
 * Allows designers to hook Blueprint VFX, Sound, Animation Montages, and Camera Shakes
 * directly into C++ football simulation events without touching C++.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UFootballBlueprintInterface : public UInterface
{
	GENERATED_BODY()
};

class FOOTBALLCOREGAMEPLAY_API IFootballBlueprintInterface
{
	GENERATED_BODY()

public:
	/** Triggered when a player or keeper strikes the ball */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|BlueprintAPI")
	void OnFootballKickExecuted(const FFootballKickRequest& KickRequest, FVector ContactWorldLocation);

	/** Triggered when a player requests a tactical pass to a teammate */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|BlueprintAPI")
	bool RequestFootballPass(AActor* TargetTeammate, bool bLobbed);

	/** Triggered when the ball impacts turf, goalposts, net, or gloves */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|BlueprintAPI")
	void OnFootballBallImpact(EFootballBallSurface Surface, float ImpactSpeedCmPerSec, FVector ImpactPoint);

	/** Query team affiliation from any Blueprint actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|BlueprintAPI")
	EFootballTeamSide GetFootballTeamSide() const;

	/** Query tactical role (GK, CB, ST, etc.) from any Blueprint actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|BlueprintAPI")
	EFootballPlayerRole GetFootballPlayerRole() const;

	/** Apply updated uniform kit colors/cloth settings dynamically in Blueprints */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|BlueprintAPI")
	void ApplyFootballKit(const FFootballKitConfig& NewKit);
};
