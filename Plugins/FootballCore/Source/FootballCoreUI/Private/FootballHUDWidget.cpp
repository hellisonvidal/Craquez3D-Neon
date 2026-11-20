// Copyright Craquez Studio. All Rights Reserved.

#include "FootballHUDWidget.h"
#include "FootballCharacter.h"
#include "FootballMatchGameMode.h"
#include "FootballMatchGameState.h"
#include "FootballWeatherAndAudioManager.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

namespace
{
	AFootballCharacter* GetControlledFootballer(const UUserWidget* Widget)
	{
		if (APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr)
		{
			return Cast<AFootballCharacter>(PC->GetPawn());
		}
		return nullptr;
	}
}

void UFootballHUDWidget::TriggerShootPressed()
{
	if (AFootballCharacter* Player = GetControlledFootballer(this))
	{
		Player->Input_ShootPressed();
	}
}

void UFootballHUDWidget::TriggerShootReleased()
{
	if (AFootballCharacter* Player = GetControlledFootballer(this))
	{
		Player->Input_ShootReleased();
	}
}

void UFootballHUDWidget::TriggerPass()
{
	if (AFootballCharacter* Player = GetControlledFootballer(this))
	{
		Player->Input_PassPressed();
	}
}

void UFootballHUDWidget::TriggerLobPass()
{
	if (AFootballCharacter* Player = GetControlledFootballer(this))
	{
		Player->Input_LobPassPressed();
	}
}

void UFootballHUDWidget::TriggerSprint(bool bSprinting)
{
	if (AFootballCharacter* Player = GetControlledFootballer(this))
	{
		if (bSprinting)
		{
			Player->Input_SprintStart();
		}
		else
		{
			Player->Input_SprintStop();
		}
	}
}

void UFootballHUDWidget::TriggerSwitchPlayer()
{
	if (AFootballCharacter* Player = GetControlledFootballer(this))
	{
		Player->Input_SwitchPlayer();
	}
}

void UFootballHUDWidget::SelectMatchMode(EFootballMatchMode NewMode)
{
	if (UWorld* World = GetWorld())
	{
		if (AFootballMatchGameMode* GM = World->GetAuthGameMode<AFootballMatchGameMode>())
		{
			GM->StartMatchMode(NewMode);
		}
	}
}

void UFootballHUDWidget::CycleStadiumWeather()
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFootballWeatherAndAudioManager> It(World); It; ++It)
		{
			It->CycleNextWeather();
			break;
		}
	}
}

void UFootballHUDWidget::GetScoreboardSnapshot(int32& OutHomeScore, int32& OutAwayScore, FString& OutClock, FString& OutModeLabel) const
{
	OutHomeScore = 0;
	OutAwayScore = 0;
	OutClock = TEXT("00:00");
	OutModeLabel = TEXT("JOGO-TREINO 11v11");

	if (UWorld* World = GetWorld())
	{
		if (AFootballMatchGameState* GS = World->GetGameState<AFootballMatchGameState>())
		{
			OutHomeScore = GS->HomeScore;
			OutAwayScore = GS->AwayScore;
			OutClock = GS->GetFormattedClock();
			OutModeLabel = (GS->ActiveMode == EFootballMatchMode::Training11v11)
				? TEXT("JOGO-TREINO 11v11")
				: FString::Printf(TEXT("DUELO 1v1 | R%d"), GS->DuelRoundIndex);
		}
	}
}
