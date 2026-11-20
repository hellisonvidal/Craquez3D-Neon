// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "FootballHUD.generated.h"

class UFootballHUDWidget;

/**
 * Craquez Broadcast & Mobile HUD.
 * Automatically renders a crisp, attractive Neon-Sports overlay (scoreboard, 2D live mini-radar
 * with all 22 players + ball, Magnus shot power gauge, stamina bar, goal banner, and controls hint)
 * immediately upon pressing Play, AND optionally spawns a custom UMG Blueprint widget.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREUI_API AFootballHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

	/** Optional UMG Blueprint subclass (e.g., WBP_CraquezMobileHUD) spawned on BeginPlay */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|UI")
	TSubclassOf<UFootballHUDWidget> CustomHUDWidgetClass;

	/** Draw real-time tactical StateTree labels above AI players for debugging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|UI|Debug")
	bool bShowTacticalAIDebug = true;

protected:
	void DrawBroadcastScoreboard();
	void DrawLivePitchRadar();
	void DrawPlayerGaugesAndControls();
	void DrawGoalBanner();

	UPROPERTY()
	TObjectPtr<UFootballHUDWidget> ActiveHUDWidget;
};
