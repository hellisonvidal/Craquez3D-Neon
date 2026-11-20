// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FootballTypes.h"
#include "FootballTacticalTypes.generated.h"

UENUM(BlueprintType)
enum class EFootballTacticalState : uint8
{
	HoldFormation UMETA(DisplayName = "Manter Formacao"),
	SupportRun UMETA(DisplayName = "Apoio / Infiltracao"),
	Press UMETA(DisplayName = "Pressao no Portador (Press)"),
	Intercept UMETA(DisplayName = "Interceptar Passe/Bola (Intercept)"),
	DribbleAttack UMETA(DisplayName = "Conduzir ao Ataque"),
	PassToTeammate UMETA(DisplayName = "Passar para Companheiro"),
	ShootAtGoal UMETA(DisplayName = "Finalizar no Gol"),
	GoalkeeperPosition UMETA(DisplayName = "Goleiro - Posicionamento"),
	GoalkeeperDive UMETA(DisplayName = "Goleiro - Defesa/Salto")
};

/**
 * Shared tactical telemetry computed every tick by FFootballTacticalEvaluator
 * and consumed by StateTree transitions, GOAP planner, and RL observations.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCOREAI_API FFootballTacticalContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	FVector BallLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	FVector BallVelocity = FVector::ZeroVector;

	/** Predicted Magnus-curved intercept point computed ahead of ball motion */
	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	FVector PredictedInterceptPoint = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	float DistanceToBallCm = 9999.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	float TimeToReachInterceptSec = 99.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	float BallTimeToInterceptSec = 99.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	bool bMyTeamHasPossession = false;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	bool bIHavePossession = false;

	/** True when ball is passed/shot/loose — primary unlock gate for Press -> Intercept transition */
	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	bool bBallIsLooseOrInFlight = false;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	bool bAmClosestDefendingTeammate = false;

	/** Hysteresis timer preventing Press <-> Intercept rapid oscillation */
	UPROPERTY(BlueprintReadOnly, Category = "Football|AI")
	float TimeInCurrentStateSec = 0.0f;
};

/**
 * Reinforcement Learning (RL) compact normalized observation vector
 * ready for ONNX / UE5 Learning Agents integration.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCOREAI_API FFootballRLObservation
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI|RL")
	FVector2D NormalizedPlayerPos = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI|RL")
	FVector2D NormalizedBallPos = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI|RL")
	FVector2D NormalizedBallVel = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI|RL")
	float DistanceToAttackingGoalNorm = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI|RL")
	float StaminaNorm = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|AI|RL")
	float PossessionFlag = 0.0f; // +1 mine, 0 loose, -1 opponent
};
