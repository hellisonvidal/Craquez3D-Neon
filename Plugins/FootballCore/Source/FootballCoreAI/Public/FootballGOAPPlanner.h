// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "FootballTacticalTypes.h"
#include "FootballGOAPPlanner.generated.h"

class AFootballCharacter;

/**
 * Lightweight Goal-Oriented Action Planner (GOAP) & Reinforcement Learning (RL) Observation Bridge.
 * Evaluates utility costs for tactical actions (Press, Intercept, SupportRun, Shoot, Pass, HoldFormation)
 * using the exact hysteresis rules that prevent Press <-> Intercept deadlocks.
 */
UCLASS(BlueprintType)
class FOOTBALLCOREAI_API UFootballGOAPPlanner : public UObject
{
	GENERATED_BODY()

public:
	/** Select highest-value tactical state via GOAP utility scoring */
	UFUNCTION(BlueprintCallable, Category = "Football|AI|GOAP")
	static EFootballTacticalState EvaluateBestTacticalState(
		const AFootballCharacter* Player,
		const FFootballTacticalContext& Context,
		EFootballTacticalState CurrentState);

	/** Build normalized RL observation struct for ONNX / UE5 Learning Agents inference */
	UFUNCTION(BlueprintPure, Category = "Football|AI|RL")
	static FFootballRLObservation BuildRLObservation(const AFootballCharacter* Player, const FFootballTacticalContext& Context);
};
