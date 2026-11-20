// Copyright Craquez Studio. All Rights Reserved.

#include "FootballMatchGameState.h"

AFootballMatchGameState::AFootballMatchGameState()
{
}

FString AFootballMatchGameState::GetFormattedClock() const
{
	const int32 TotalSecs = FMath::FloorToInt32(MatchElapsedSeconds);
	const int32 Mins = TotalSecs / 60;
	const int32 Secs = TotalSecs % 60;
	return FString::Printf(TEXT("%02d:%02d"), Mins, Secs);
}
