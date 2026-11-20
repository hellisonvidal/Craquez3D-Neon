// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "FootballTypes.h"
#include "FootballMatchGameMode.generated.h"

class AFootballBallActor;
class AFootballCharacter;
class AFootballStadiumGenerator;
class AFootballWeatherAndAudioManager;
class AAIController;

/**
 * Core GameMode orchestrating Craquez Offline Single-Player Modes:
 * 1. Jogo-Treino 11vs11 (Full 22-player tactical match: 4-3-3 Home vs 4-4-2 Away)
 * 2. Duelo 1vs1 (Atacante x Goleiro finishing & Magnus curve challenge)
 * Automatically spawns the procedural low-poly stadium, ball, weather/audio manager, and teams
 * so pressing Play in UE5 works out of the box even on an empty level!
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREGAMEPLAY_API AFootballMatchGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AFootballMatchGameMode();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	/** Start or switch between 11vs11 Training and 1vs1 Striker vs Goalkeeper */
	UFUNCTION(BlueprintCallable, Category = "Football|Match")
	void StartMatchMode(EFootballMatchMode NewMode);

	/** Reset players & ball to Kickoff (11v11) or Penalty Arc Breakaway (1v1) */
	UFUNCTION(BlueprintCallable, Category = "Football|Match")
	void ResetToKickoffPositions();

	/** Switch human PlayerController to the Home teammate closest to the ball */
	UFUNCTION(BlueprintCallable, Category = "Football|Match")
	void SwitchHumanControlToClosestTeammate();

	/** Register custom AI controller class from FootballCoreAI (avoids circular dependency) */
	static void RegisterDefaultFootballAIControllerClass(TSubclassOf<AAIController> InAIClass);

	/** Register custom HUD class from FootballCoreUI (avoids circular dependency) */
	static void RegisterDefaultFootballHUDClass(TSubclassOf<AHUD> InHUDClass);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Match")
	EFootballMatchMode CurrentMatchMode = EFootballMatchMode::Training11v11;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Match|Classes")
	TSubclassOf<AFootballCharacter> PlayerCharacterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Match|Classes")
	TSubclassOf<AFootballBallActor> BallActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Match|Classes")
	TSubclassOf<AAIController> FootballAIControllerClass;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	TObjectPtr<AFootballBallActor> MatchBall;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	TObjectPtr<AFootballStadiumGenerator> StadiumActor;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	TObjectPtr<AFootballWeatherAndAudioManager> WeatherAudioManager;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Match")
	TArray<TObjectPtr<AFootballCharacter>> AllPlayers;

protected:
	UFUNCTION()
	void HandleGoalScored(EFootballTeamSide ScoringTeam);

	void EnsureWorldActorsSpawned();
	void ClearAllPlayers();
	void SpawnTraining11v11Teams();
	void SpawnDuel1v1Actors();
	AFootballCharacter* SpawnSinglePlayer(
		int32 PlayerId,
		EFootballTeamSide Side,
		EFootballPlayerRole Role,
		const FVector& SpawnLoc,
		const FFootballKitConfig& Kit,
		bool bHumanControlled);

	FFootballKitConfig MakeHomeKit(int32 Number, bool bIsGK) const;
	FFootballKitConfig MakeAwayKit(int32 Number, bool bIsGK) const;

	float PendingResetTimer = -1.0f;

	static TSubclassOf<AAIController> RegisteredDefaultAIControllerClass;
	static TSubclassOf<AHUD> RegisteredDefaultHUDClass;
};
