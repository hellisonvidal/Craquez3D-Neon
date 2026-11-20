// Copyright Craquez Studio. All Rights Reserved.

#include "FootballMatchGameMode.h"
#include "FootballMatchGameState.h"
#include "FootballBallActor.h"
#include "FootballCharacter.h"
#include "FootballStadiumGenerator.h"
#include "FootballWeatherAndAudioManager.h"
#include "FootballDeterministicSimSubsystem.h"
#include "AIController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

TSubclassOf<AAIController> AFootballMatchGameMode::RegisteredDefaultAIControllerClass = nullptr;
TSubclassOf<AHUD> AFootballMatchGameMode::RegisteredDefaultHUDClass = nullptr;

AFootballMatchGameMode::AFootballMatchGameMode()
{
	PrimaryActorTick.bCanEverTick = true;
	GameStateClass = AFootballMatchGameState::StaticClass();
	PlayerCharacterClass = AFootballCharacter::StaticClass();
	BallActorClass = AFootballBallActor::StaticClass();
	DefaultPawnClass = nullptr; // Spawned and possessed explicitly by MatchMode
}

void AFootballMatchGameMode::RegisterDefaultFootballAIControllerClass(TSubclassOf<AAIController> InAIClass)
{
	RegisteredDefaultAIControllerClass = InAIClass;
}

void AFootballMatchGameMode::RegisterDefaultFootballHUDClass(TSubclassOf<AHUD> InHUDClass)
{
	RegisteredDefaultHUDClass = InHUDClass;
}

void AFootballMatchGameMode::BeginPlay()
{
	if (!HUDClass && RegisteredDefaultHUDClass)
	{
		HUDClass = RegisteredDefaultHUDClass;
	}
	if (!FootballAIControllerClass && RegisteredDefaultAIControllerClass)
	{
		FootballAIControllerClass = RegisteredDefaultAIControllerClass;
	}

	Super::BeginPlay();

	EnsureWorldActorsSpawned();
	StartMatchMode(CurrentMatchMode);
}

void AFootballMatchGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	AFootballMatchGameState* GS = GetGameState<AFootballMatchGameState>();
	if (GS)
	{
		GS->MatchElapsedSeconds += DeltaSeconds;
		if (GS->BannerDisplayTimer > 0.0f)
		{
			GS->BannerDisplayTimer = FMath::Max(0.0f, GS->BannerDisplayTimer - DeltaSeconds);
		}
	}

	if (PendingResetTimer > 0.0f)
	{
		PendingResetTimer -= DeltaSeconds;
		if (PendingResetTimer <= 0.0f)
		{
			ResetToKickoffPositions();
		}
	}

	// Keep ball inside stadium perimeter (out-of-bounds auto-return for smooth pilot flow)
	if (MatchBall)
	{
		const FVector BallLoc = MatchBall->GetActorLocation();
		if (FMath::Abs(BallLoc.X) > 5650.0f || FMath::Abs(BallLoc.Y) > 3750.0f || BallLoc.Z < -150.0f)
		{
			if (CurrentMatchMode == EFootballMatchMode::Duel1v1)
			{
				if (GS)
				{
					GS->DuelRoundIndex++;
					GS->StatusBannerText = FString::Printf(TEXT("RODADA %d — ATACANTE x GOLEIRO"), GS->DuelRoundIndex);
					GS->BannerDisplayTimer = 2.0f;
				}
				ResetToKickoffPositions();
			}
			else
			{
				const FVector Clamped(
					FMath::Clamp(BallLoc.X, -5050.0f, 5050.0f),
					FMath::Clamp(BallLoc.Y, -3200.0f, 3200.0f),
					20.0f);
				MatchBall->ResetBallToLocation(Clamped);
			}
		}

		// Record deterministic frame snapshot
		if (UFootballDeterministicSimSubsystem* SimSub = GetWorld()->GetSubsystem<UFootballDeterministicSimSubsystem>())
		{
			const int32 H = GS ? GS->HomeScore : 0;
			const int32 A = GS ? GS->AwayScore : 0;
			SimSub->RecordSnapshot(MatchBall->CaptureDeterministicState(), H, A);
		}
	}
}

void AFootballMatchGameMode::EnsureWorldActorsSpawned()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 1. Stadium
	for (TActorIterator<AFootballStadiumGenerator> It(World); It; ++It)
	{
		StadiumActor = *It;
		break;
	}
	if (!StadiumActor)
	{
		StadiumActor = World->SpawnActor<AFootballStadiumGenerator>(AFootballStadiumGenerator::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
	if (StadiumActor)
	{
		StadiumActor->OnGoalScored.RemoveDynamic(this, &AFootballMatchGameMode::HandleGoalScored);
		StadiumActor->OnGoalScored.AddDynamic(this, &AFootballMatchGameMode::HandleGoalScored);
	}

	// 2. Ball
	for (TActorIterator<AFootballBallActor> It(World); It; ++It)
	{
		MatchBall = *It;
		break;
	}
	if (!MatchBall)
	{
		UClass* SpawnBallClass = BallActorClass ? BallActorClass.Get() : AFootballBallActor::StaticClass();
		MatchBall = World->SpawnActor<AFootballBallActor>(SpawnBallClass, FVector(0.0f, 0.0f, 25.0f), FRotator::ZeroRotator);
	}

	// 3. Weather & Positional Audio Manager
	for (TActorIterator<AFootballWeatherAndAudioManager> It(World); It; ++It)
	{
		WeatherAudioManager = *It;
		break;
	}
	if (!WeatherAudioManager)
	{
		WeatherAudioManager = World->SpawnActor<AFootballWeatherAndAudioManager>(AFootballWeatherAndAudioManager::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	}
}

void AFootballMatchGameMode::StartMatchMode(EFootballMatchMode NewMode)
{
	CurrentMatchMode = NewMode;
	PendingResetTimer = -1.0f;

	if (AFootballMatchGameState* GS = GetGameState<AFootballMatchGameState>())
	{
		GS->ActiveMode = CurrentMatchMode;
		GS->HomeScore = 0;
		GS->AwayScore = 0;
		GS->MatchElapsedSeconds = 0.0f;
		GS->DuelRoundIndex = 1;
		GS->StatusBannerText = (CurrentMatchMode == EFootballMatchMode::Training11v11)
			? TEXT("JOGO-TREINO 11v11 — CRAQUEZ FC vs RIVAL FC")
			: TEXT("DUELO 1v1 — ATACANTE x GOLEIRO");
		GS->BannerDisplayTimer = 3.5f;
	}

	if (UFootballDeterministicSimSubsystem* SimSub = GetWorld()->GetSubsystem<UFootballDeterministicSimSubsystem>())
	{
		SimSub->ResetSimulation(20261120);
	}

	ClearAllPlayers();

	if (CurrentMatchMode == EFootballMatchMode::Training11v11)
	{
		SpawnTraining11v11Teams();
	}
	else
	{
		SpawnDuel1v1Actors();
	}

	ResetToKickoffPositions();
}

void AFootballMatchGameMode::ClearAllPlayers()
{
	for (AFootballCharacter* Player : AllPlayers)
	{
		if (IsValid(Player))
		{
			if (AController* Ctrl = Player->GetController())
			{
				if (Cast<AAIController>(Ctrl))
				{
					Ctrl->Destroy();
				}
			}
			Player->Destroy();
		}
	}
	AllPlayers.Empty();
}

void AFootballMatchGameMode::SpawnTraining11v11Teams()
{
	// Home Team (Craquez FC) — Tactical 4-3-3 attacking toward +X
	struct FFormationSlot { EFootballPlayerRole Role; FVector Pos; int32 Num; };
	const TArray<FFormationSlot> HomeSlots = {
		{ EFootballPlayerRole::Goalkeeper,   FVector(-4950,     0, 92), 1  },
		{ EFootballPlayerRole::LeftBack,     FVector(-3400, -1850, 92), 6  },
		{ EFootballPlayerRole::CenterBack,   FVector(-3650,  -620, 92), 4  },
		{ EFootballPlayerRole::CenterBack,   FVector(-3650,   620, 92), 3  },
		{ EFootballPlayerRole::RightBack,    FVector(-3400,  1850, 92), 2  },
		{ EFootballPlayerRole::DefensiveMid, FVector(-2150,     0, 92), 5  },
		{ EFootballPlayerRole::CentralMid,   FVector(-1450, -1100, 92), 8  },
		{ EFootballPlayerRole::AttackingMid, FVector(-1200,  1100, 92), 10 },
		{ EFootballPlayerRole::LeftWing,     FVector( -480, -1950, 92), 11 },
		{ EFootballPlayerRole::RightWing,    FVector( -480,  1950, 92), 7  },
		{ EFootballPlayerRole::Striker,      FVector( -220,     0, 92), 9  } // Human starts here!
	};

	// Away Team (Rival FC) — Tactical 4-4-2 attacking toward -X
	const TArray<FFormationSlot> AwaySlots = {
		{ EFootballPlayerRole::Goalkeeper,   FVector( 4950,     0, 92), 1  },
		{ EFootballPlayerRole::LeftBack,     FVector( 3450,  1850, 92), 6  },
		{ EFootballPlayerRole::CenterBack,   FVector( 3700,   640, 92), 4  },
		{ EFootballPlayerRole::CenterBack,   FVector( 3700,  -640, 92), 3  },
		{ EFootballPlayerRole::RightBack,    FVector( 3450, -1850, 92), 2  },
		{ EFootballPlayerRole::LeftWing,     FVector( 1750,  1950, 92), 11 },
		{ EFootballPlayerRole::CentralMid,   FVector( 1850,   620, 92), 8  },
		{ EFootballPlayerRole::DefensiveMid, FVector( 1850,  -620, 92), 5  },
		{ EFootballPlayerRole::RightWing,    FVector( 1750, -1950, 92), 7  },
		{ EFootballPlayerRole::Striker,      FVector(  580,   420, 92), 9  },
		{ EFootballPlayerRole::Striker,      FVector(  580,  -420, 92), 10 }
	};

	int32 NextId = 1;
	for (int32 i = 0; i < HomeSlots.Num(); ++i)
	{
		const bool bIsHuman = (HomeSlots[i].Role == EFootballPlayerRole::Striker);
		SpawnSinglePlayer(NextId++, EFootballTeamSide::Home, HomeSlots[i].Role, HomeSlots[i].Pos,
			MakeHomeKit(HomeSlots[i].Num, HomeSlots[i].Role == EFootballPlayerRole::Goalkeeper), bIsHuman);
	}

	for (int32 i = 0; i < AwaySlots.Num(); ++i)
	{
		SpawnSinglePlayer(NextId++, EFootballTeamSide::Away, AwaySlots[i].Role, AwaySlots[i].Pos,
			MakeAwayKit(AwaySlots[i].Num, AwaySlots[i].Role == EFootballPlayerRole::Goalkeeper), false);
	}
}

void AFootballMatchGameMode::SpawnDuel1v1Actors()
{
	// 1vs1 Mode: Human Striker on the edge of the Away box vs AI Goalkeeper guarding +X goal
	SpawnSinglePlayer(1, EFootballTeamSide::Home, EFootballPlayerRole::Striker, FVector(2850.0f, 0.0f, 92.0f), MakeHomeKit(10, false), true);
	SpawnSinglePlayer(2, EFootballTeamSide::Away, EFootballPlayerRole::Goalkeeper, FVector(5020.0f, 0.0f, 92.0f), MakeAwayKit(1, true), false);
}

AFootballCharacter* AFootballMatchGameMode::SpawnSinglePlayer(
	int32 PlayerId,
	EFootballTeamSide Side,
	EFootballPlayerRole Role,
	const FVector& SpawnLoc,
	const FFootballKitConfig& Kit,
	bool bHumanControlled)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FRotator Facing = (Side == EFootballTeamSide::Home) ? FRotator(0.0f, 0.0f, 0.0f) : FRotator(0.0f, 180.0f, 0.0f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	UClass* CharClass = PlayerCharacterClass ? PlayerCharacterClass.Get() : AFootballCharacter::StaticClass();
	AFootballCharacter* Player = World->SpawnActor<AFootballCharacter>(CharClass, SpawnLoc, Facing, Params);
	if (!Player)
	{
		return nullptr;
	}

	Player->ConfigurePlayer(PlayerId, Side, Role, SpawnLoc, Kit);
	AllPlayers.Add(Player);

	if (bHumanControlled)
	{
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->Possess(Player);
		}
	}
	else
	{
		UClass* AIClass = FootballAIControllerClass ? FootballAIControllerClass.Get() : AAIController::StaticClass();
		if (AAIController* AICtrl = World->SpawnActor<AAIController>(AIClass, SpawnLoc, Facing))
		{
			AICtrl->Possess(Player);
		}
	}

	return Player;
}

void AFootballMatchGameMode::ResetToKickoffPositions()
{
	for (AFootballCharacter* Player : AllPlayers)
	{
		if (IsValid(Player))
		{
			const FRotator Facing = (Player->TeamSide == EFootballTeamSide::Home) ? FRotator(0, 0, 0) : FRotator(0, 180, 0);
			Player->SetActorLocationAndRotation(Player->FormationHomeLocation, Facing, false, nullptr, ETeleportType::TeleportPhysics);
			Player->GetCharacterMovement()->StopMovementImmediately();
		}
	}

	if (MatchBall)
	{
		const FVector BallSpot = (CurrentMatchMode == EFootballMatchMode::Duel1v1)
			? FVector(3050.0f, 0.0f, 14.0f)
			: FVector(0.0f, 0.0f, 14.0f);
		MatchBall->ResetBallToLocation(BallSpot);
	}
}

void AFootballMatchGameMode::SwitchHumanControlToClosestTeammate()
{
	if (!MatchBall || !GetWorld())
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	AFootballCharacter* CurrentHuman = Cast<AFootballCharacter>(PC->GetPawn());
	AFootballCharacter* BestCandidate = nullptr;
	float BestDist = FLT_MAX;

	for (AFootballCharacter* Player : AllPlayers)
	{
		if (!IsValid(Player) || Player == CurrentHuman || Player->TeamSide != EFootballTeamSide::Home || Player->PlayerRole == EFootballPlayerRole::Goalkeeper)
		{
			continue;
		}
		const float Dist = FVector::Dist2D(Player->GetActorLocation(), MatchBall->GetActorLocation());
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestCandidate = Player;
		}
	}

	if (BestCandidate)
	{
		// Hand old human back to AI and possess new closest teammate
		AController* OldBestCtrl = BestCandidate->GetController();
		if (OldBestCtrl && Cast<AAIController>(OldBestCtrl))
		{
			OldBestCtrl->UnPossess();
			if (CurrentHuman)
			{
				OldBestCtrl->Possess(CurrentHuman);
			}
		}
		PC->Possess(BestCandidate);
	}
}

void AFootballMatchGameMode::HandleGoalScored(EFootballTeamSide ScoringTeam)
{
	if (AFootballMatchGameState* GS = GetGameState<AFootballMatchGameState>())
	{
		if (ScoringTeam == EFootballTeamSide::Home)
		{
			GS->HomeScore++;
			GS->StatusBannerText = TEXT("GOOOL DO CRAQUEZ FC!");
		}
		else
		{
			GS->AwayScore++;
			GS->StatusBannerText = TEXT("GOL DO RIVAL FC!");
		}
		if (CurrentMatchMode == EFootballMatchMode::Duel1v1)
		{
			GS->DuelRoundIndex++;
		}
		GS->BannerDisplayTimer = 2.8f;
	}

	if (WeatherAudioManager)
	{
		WeatherAudioManager->AddCrowdExcitement(0.85f);
	}

	PendingResetTimer = 2.5f;
}

FFootballKitConfig AFootballMatchGameMode::MakeHomeKit(int32 Number, bool bIsGK) const
{
	FFootballKitConfig Kit;
	Kit.TeamName = TEXT("CRAQUEZ FC");
	Kit.SquadNumber = Number;
	if (bIsGK)
	{
		Kit.JerseyPrimaryColor = FLinearColor(0.95f, 0.72f, 0.04f, 1.0f);
		Kit.JerseySecondaryColor = FLinearColor(0.08f, 0.08f, 0.12f, 1.0f);
		Kit.ShortsColor = FLinearColor(0.08f, 0.08f, 0.12f, 1.0f);
		Kit.SocksColor = FLinearColor(0.95f, 0.72f, 0.04f, 1.0f);
	}
	else
	{
		Kit.JerseyPrimaryColor = FLinearColor(0.02f, 0.72f, 0.35f, 1.0f); // Emerald
		Kit.JerseySecondaryColor = FLinearColor(0.0f, 0.92f, 0.96f, 1.0f); // Neon Cyan
		Kit.ShortsColor = FLinearColor(0.05f, 0.08f, 0.14f, 1.0f);
		Kit.SocksColor = FLinearColor(0.02f, 0.72f, 0.35f, 1.0f);
		Kit.BootsColor = FLinearColor(0.96f, 0.98f, 0.12f, 1.0f);
	}
	return Kit;
}

FFootballKitConfig AFootballMatchGameMode::MakeAwayKit(int32 Number, bool bIsGK) const
{
	FFootballKitConfig Kit;
	Kit.TeamName = TEXT("RIVAL FC");
	Kit.SquadNumber = Number;
	if (bIsGK)
	{
		Kit.JerseyPrimaryColor = FLinearColor(0.58f, 0.14f, 0.92f, 1.0f);
		Kit.JerseySecondaryColor = FLinearColor(0.95f, 0.95f, 0.98f, 1.0f);
		Kit.ShortsColor = FLinearColor(0.10f, 0.08f, 0.16f, 1.0f);
		Kit.SocksColor = FLinearColor(0.58f, 0.14f, 0.92f, 1.0f);
	}
	else
	{
		Kit.JerseyPrimaryColor = FLinearColor(0.88f, 0.12f, 0.18f, 1.0f); // Crimson
		Kit.JerseySecondaryColor = FLinearColor(0.98f, 0.98f, 1.0f, 1.0f);
		Kit.ShortsColor = FLinearColor(0.95f, 0.96f, 0.98f, 1.0f);
		Kit.SocksColor = FLinearColor(0.88f, 0.12f, 0.18f, 1.0f);
		Kit.BootsColor = FLinearColor(0.06f, 0.85f, 0.95f, 1.0f);
	}
	return Kit;
}
