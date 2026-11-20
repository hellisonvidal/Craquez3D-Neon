// Copyright Craquez Studio. All Rights Reserved.

#include "FootballCoreGameplayModule.h"

DEFINE_LOG_CATEGORY(LogFootballGameplay);

void FFootballCoreGameplayModule::StartupModule()
{
	UE_LOG(LogFootballGameplay, Log, TEXT("FootballCoreGameplay module started (Chaos Ball Magnus Physics, Modular Cloth Kits, Procedural IK & Stadium)."));
}

void FFootballCoreGameplayModule::ShutdownModule()
{
	UE_LOG(LogFootballGameplay, Log, TEXT("FootballCoreGameplay module shut down."));
}

IMPLEMENT_MODULE(FFootballCoreGameplayModule, FootballCoreGameplay)
