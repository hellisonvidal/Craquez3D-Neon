// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FootballTacticalTypes.h"
#include "FootballAIController.generated.h"

class UStateTreeAIComponent;
class AFootballCharacter;
class AFootballBallActor;

/**
 * Tactical AI Controller for Craquez (11v11 Outfielders & Goalkeepers + 1v1 Goalkeeper).
 * Supports attaching a UE 5.8 StateTree asset via StateTreeAIComponent AND includes a
 * complete, immediately-playable C++ GOAP/StateTree brain so 11v11 and 1v1 AI play
 * realistic football right away without requiring manual asset setup first.
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREAI_API AFootballAIController : public AAIController
{
	GENERATED_BODY()

public:
	AFootballAIController();

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|AI")
	TObjectPtr<UStateTreeAIComponent> TacticalStateTreeComponent;

	/** Set true if you assign a custom UE5 StateTree asset to TacticalStateTreeComponent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|AI")
	bool bUseExternalStateTreeAsset = false;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	EFootballTacticalState ActiveTacticalState = EFootballTacticalState::HoldFormation;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	FFootballTacticalContext LiveContext;

protected:
	void RefreshTacticalContext(AFootballCharacter* Player, AFootballBallActor* Ball, float DeltaTime);
	void ExecuteTacticalState(AFootballCharacter* Player, AFootballBallActor* Ball, float DeltaTime);
	bool IsClosestTeammateToBall(const AFootballCharacter* Player, const FVector& BallLoc) const;
	void SteerPlayerToward(AFootballCharacter* Player, const FVector& TargetLocation, bool bSprint) const;

	float DecisionCooldown = 0.0f;
	float ActionCooldown = 0.0f;
};
