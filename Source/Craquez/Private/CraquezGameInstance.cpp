// Copyright Craquez Studio. All Rights Reserved.

#include "CraquezGameInstance.h"
#include "GameFramework/GameUserSettings.h"

void UCraquezGameInstance::Init()
{
	Super::Init();

#if PLATFORM_ANDROID
	ApplyGraphicsPreset(1); // Android Balanced 60 FPS target by default
#else
	ApplyGraphicsPreset(3); // Desktop High/Epic preview
#endif
}

void UCraquezGameInstance::ApplyGraphicsPreset(int32 QualityLevel)
{
	if (UGameUserSettings* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr)
	{
		const int32 Clamped = FMath::Clamp(QualityLevel, 0, 3);
		Settings->SetOverallScalabilityLevel(Clamped);
		Settings->SetFrameRateLimit(PLATFORM_ANDROID ? 60.0f : 120.0f);
		Settings->ApplySettings(false);
	}
}
