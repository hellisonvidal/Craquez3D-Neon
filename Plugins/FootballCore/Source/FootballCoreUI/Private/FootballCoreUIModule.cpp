// Copyright Craquez Studio. All Rights Reserved.

#include "FootballCoreUIModule.h"
#include "FootballHUD.h"
#include "FootballMatchGameMode.h"

DEFINE_LOG_CATEGORY(LogFootballUI);

void FFootballCoreUIModule::StartupModule()
{
	AFootballMatchGameMode::RegisterDefaultFootballHUDClass(AFootballHUD::StaticClass());
	UE_LOG(LogFootballUI, Log, TEXT("FootballCoreUI module started (Craquez Neon Broadcast HUD & Mobile UMG Touch API ready)."));
}

void FFootballCoreUIModule::ShutdownModule()
{
	UE_LOG(LogFootballUI, Log, TEXT("FootballCoreUI module shut down."));
}

IMPLEMENT_MODULE(FFootballCoreUIModule, FootballCoreUI)
