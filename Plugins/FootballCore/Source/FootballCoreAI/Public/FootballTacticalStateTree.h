// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreeConditionBase.h"
#include "StateTreeTaskBase.h"
#include "StateTreeExecutionContext.h"
#include "FootballTacticalTypes.h"
#include "FootballTacticalStateTree.generated.h"

class AFootballCharacter;
class AFootballBallActor;

// ============================================================================
// 1. TACTICAL EVALUATOR (Fixes stale property bindings during Press -> Intercept)
// ============================================================================

USTRUCT()
struct FOOTBALLCOREAI_API FFootballTacticalEvaluatorInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AFootballCharacter> FootballPlayer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Output")
	FFootballTacticalContext TacticalContext;
};

/**
 * StateTree Evaluator ticking every frame to refresh ball velocity, Magnus intercept point,
 * and loose-ball flag. Without a ticking Evaluator, Press -> Intercept conditions read stale
 * ball velocity and get permanently stuck in Press!
 */
USTRUCT(meta = (DisplayName = "Craquez Tactical Evaluator"))
struct FOOTBALLCOREAI_API FFootballTacticalEvaluator : public FStateTreeEvaluatorCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FFootballTacticalEvaluatorInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
	virtual void Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;

private:
	void RefreshContext(FStateTreeExecutionContext& Context) const;
};

// ============================================================================
// 2. CONDITION: CAN INTERCEPT BALL (Fixes Press -> Intercept Hysteresis & Gate)
// ============================================================================

USTRUCT()
struct FOOTBALLCOREAI_API FFootballCond_CanInterceptBallInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Input")
	FFootballTacticalContext TacticalContext;

	/** Maximum reach radius to break off Press and sprint for an interception */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxInterceptRadiusCm = 1900.0f;

	/** Lead margin (seconds) ensuring player can actually beat the ball to the intercept point */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float InterceptTimeMarginSec = 0.20f;
};

/**
 * Evaluated on OnTick transitions out of 'Press'.
 * Returns TRUE cleanly when:
 * 1. The ball is loose or in-flight (e.g. carrier just released a pass/heavy touch), AND
 * 2. Player is within MaxInterceptRadiusCm and can reach PredictedInterceptPoint within margin.
 */
USTRUCT(meta = (DisplayName = "Craquez: Can Intercept Ball (Press->Intercept Fix)"))
struct FOOTBALLCOREAI_API FFootballCond_CanInterceptBall : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FFootballCond_CanInterceptBallInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;
};

// ============================================================================
// 3. TASK: PRESS CARRIER (Fixes OnStateCompleted Lock by Yielding on Release)
// ============================================================================

USTRUCT()
struct FOOTBALLCOREAI_API FFootballTask_PressCarrierInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AFootballCharacter> FootballPlayer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	FFootballTacticalContext TacticalContext;

	/** Safety timeout so Press never permanently deadlocks even if transition triggers fail */
	UPROPERTY(EditAnywhere, Category = "Parameter")
	float MaxPressDurationSec = 4.5f;

	float ElapsedPressSec = 0.0f;
};

/**
 * Press Task: closes down opponent ball carrier and attempts standing tackle.
 * CRITICAL FIX for stuck 'Press' -> 'Intercept' transitions:
 * - Explicitly sets bShouldCallTick = true and bShouldCopyBoundPropertiesOnTick = true.
 * - Immediately returns EStateTreeRunStatus::Succeeded when the ball becomes loose/passed
 *   OR when MaxPressDurationSec expires, guaranteeing transition even if the designer wired
 *   OnStateCompleted instead of OnTick!
 */
USTRUCT(meta = (DisplayName = "Craquez: Press Ball Carrier"))
struct FOOTBALLCOREAI_API FFootballTask_PressCarrier : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FFootballTask_PressCarrier()
	{
		bShouldCallTick = true;
		bShouldCopyBoundPropertiesOnTick = true;
	}

	using FInstanceDataType = FFootballTask_PressCarrierInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
};

// ============================================================================
// 4. TASK: INTERCEPT BALL (Sprints to Magnus-Predicted Intercept Point)
// ============================================================================

USTRUCT()
struct FOOTBALLCOREAI_API FFootballTask_InterceptBallInstanceData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category = "Context")
	TObjectPtr<AFootballCharacter> FootballPlayer = nullptr;

	UPROPERTY(EditAnywhere, Category = "Input")
	FFootballTacticalContext TacticalContext;

	float ElapsedInterceptSec = 0.0f;
};

USTRUCT(meta = (DisplayName = "Craquez: Intercept Predicted Ball"))
struct FOOTBALLCOREAI_API FFootballTask_InterceptBall : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	FFootballTask_InterceptBall()
	{
		bShouldCallTick = true;
		bShouldCopyBoundPropertiesOnTick = true;
	}

	using FInstanceDataType = FFootballTask_InterceptBallInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const override;
};
