// Copyright Craquez Studio. All Rights Reserved.

#include "FootballCoreNetworkingModule.h"

DEFINE_LOG_CATEGORY(LogFootballNetworking);

void FFootballCoreNetworkingModule::StartupModule()
{
	UE_LOG(LogFootballNetworking, Log, TEXT("FootballCoreNetworking module started (Deterministic Fixed-Step & Quantized Replication ready)."));
}

void FFootballCoreNetworkingModule::ShutdownModule()
{
	UE_LOG(LogFootballNetworking, Log, TEXT("FootballCoreNetworking module shut down."));
}

IMPLEMENT_MODULE(FFootballCoreNetworkingModule, FootballCoreNetworking)
