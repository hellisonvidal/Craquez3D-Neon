// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "FootballTypes.h"
#include "FootballMatchGameState.generated.h"

/**
 * Replicated & UI-readable match state for Craquez (11v11 Jogo-Treino & 1v1 Duelo).
 */
UCLASS(BlueprintType)
class FOOTBALLCOREGAMEPLAY_API AFootballMatchGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AFootballMatchGameState();

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	EFootballMatchMode ActiveMode = EFootballMatchMode::Training11v11;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	int32 HomeScore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	int32 AwayScore = 0;

	/** Match clock in simulated match minutes (0..90) */
	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	float MatchElapsedSeconds = 0.0f;

	/** Round counter for 1vs1 Striker vs Goalkeeper mode */
	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	int32 DuelRoundIndex = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	FString StatusBannerText = TEXT("JOGO-TREINO 11v11 — CRAQUEZ");

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	float BannerDisplayTimer = 0.0f;

	UFUNCTION(BlueprintPure, Category = "Football|Match")
	FString GetFormattedClock() const;
};
