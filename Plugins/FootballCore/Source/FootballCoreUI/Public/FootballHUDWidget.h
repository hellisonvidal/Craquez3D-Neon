// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FootballTypes.h"
#include "FootballHUDWidget.generated.h"

/**
 * Mobile-First UMG Base Widget for Craquez (Android Touch Controls, Broadcast Scoreboard & Main Menu).
 * Bind UMG buttons in Blueprint directly to these BlueprintCallable methods!
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREUI_API UFootballHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Football|UI|Touch")
	void TriggerShootPressed();

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Touch")
	void TriggerShootReleased();

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Touch")
	void TriggerPass();

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Touch")
	void TriggerLobPass();

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Touch")
	void TriggerSprint(bool bSprinting);

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Touch")
	void TriggerSwitchPlayer();

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Match")
	void SelectMatchMode(EFootballMatchMode NewMode);

	UFUNCTION(BlueprintCallable, Category = "Football|UI|Weather")
	void CycleStadiumWeather();

	UFUNCTION(BlueprintPure, Category = "Football|UI|Match")
	void GetScoreboardSnapshot(int32& OutHomeScore, int32& OutAwayScore, FString& OutClock, FString& OutModeLabel) const;
};
