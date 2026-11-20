// Copyright Craquez Studio. All Rights Reserved.

#include "FootballCoreAIModule.h"
#include "FootballAIController.h"
#include "FootballMatchGameMode.h"

DEFINE_LOG_CATEGORY(LogFootballAI);

void FFootballCoreAIModule::StartupModule()
{
	AFootballMatchGameMode::RegisterDefaultFootballAIControllerClass(AFootballAIController::StaticClass());
	UE_LOG(LogFootballAI, Log, TEXT("FootballCoreAI module started (StateTree Tactical AI, Press->Intercept Fix, GOAP & GK Brain registered)."));
}

void FFootballCoreAIModule::ShutdownModule()
{
	UE_LOG(LogFootballAI, Log, TEXT("FootballCoreAI module shut down."));
}

IMPLEMENT_MODULE(FFootballCoreAIModule, FootballCoreAI)
