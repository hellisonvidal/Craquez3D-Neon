// Copyright Craquez Studio. All Rights Reserved.

#include "FootballTacticalStateTree.h"
#include "FootballCharacter.h"
#include "FootballBallActor.h"
#include "FootballCoreAIModule.h"
#include "AIController.h"

// ============================================================================
// FFootballTacticalEvaluator
// ============================================================================

void FFootballTacticalEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
	RefreshContext(Context);
}

void FFootballTacticalEvaluator::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.TacticalContext.TimeInCurrentStateSec += DeltaTime;
	RefreshContext(Context);
}

void FFootballTacticalEvaluator::RefreshContext(FStateTreeExecutionContext& Context) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	AFootballCharacter* Player = Data.FootballPlayer;
	if (!Player)
	{
		return;
	}

	AFootballBallActor* Ball = Player->GetMatchBall();
	if (!Ball)
	{
		return;
	}

	FFootballTacticalContext& Ctx = Data.TacticalContext;
	Ctx.BallLocation = Ball->GetActorLocation();
	Ctx.BallVelocity = Ball->GetBallVelocity();
	Ctx.DistanceToBallCm = FVector::Dist2D(Player->GetActorLocation(), Ctx.BallLocation);

	AFootballCharacter* Owner = Ball->GetCurrentOwnerPlayer();
	Ctx.bIHavePossession = (Owner == Player);
	Ctx.bMyTeamHasPossession = (Owner != nullptr && Owner->TeamSide == Player->TeamSide);
	Ctx.bBallIsLooseOrInFlight = Ball->IsBallLooseOrInFlight();

	// Compute Magnus-curved intercept point over a 0.3s..1.1s lookahead horizon
	const float PlayerSprintSpeed = 680.0f;
	const float LookaheadSec = FMath::Clamp(Ctx.DistanceToBallCm / PlayerSprintSpeed, 0.25f, 1.10f);
	Ctx.PredictedInterceptPoint = Ball->PredictPositionAtTime(LookaheadSec, 10);
	Ctx.BallTimeToInterceptSec = LookaheadSec;
	Ctx.TimeToReachInterceptSec = FVector::Dist2D(Player->GetActorLocation(), Ctx.PredictedInterceptPoint) / PlayerSprintSpeed;
}

// ============================================================================
// FFootballCond_CanInterceptBall
// ============================================================================

bool FFootballCond_CanInterceptBall::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	const FFootballTacticalContext& Ctx = Data.TacticalContext;

	// Never intercept if our own team already controls the ball
	if (Ctx.bMyTeamHasPossession)
	{
		return false;
	}

	// Gate 1: Ball must actually be loose, passed, or in-flight (breaks Press lock cleanly!)
	if (!Ctx.bBallIsLooseOrInFlight)
	{
		return false;
	}

	// Gate 2: Within tactical interception envelope
	if (Ctx.DistanceToBallCm > Data.MaxInterceptRadiusCm)
	{
		return false;
	}

	// Gate 3: Feasibility check with hysteresis margin so player commits decisively
	return Ctx.TimeToReachInterceptSec <= (Ctx.BallTimeToInterceptSec + Data.InterceptTimeMarginSec);
}

// ============================================================================
// FFootballTask_PressCarrier
// ============================================================================

EStateTreeRunStatus FFootballTask_PressCarrier::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedPressSec = 0.0f;
	if (Data.FootballPlayer)
	{
		Data.FootballPlayer->CurrentTacticalStateLabel = TEXT("Press");
		Data.FootballPlayer->Input_SprintStart();
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FFootballTask_PressCarrier::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedPressSec += DeltaTime;

	AFootballCharacter* Player = Data.FootballPlayer;
	if (!Player)
	{
		return EStateTreeRunStatus::Failed;
	}

	// CRITICAL FIX: Yield cleanly as soon as ball is released/loose so StateTree transitions to 'Intercept'
	// even if the asset transition was mistakenly authored with OnStateCompleted instead of OnTick!
	if (Data.TacticalContext.bBallIsLooseOrInFlight)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// Possession won or press timed out -> allow tactical re-evaluation
	if (Data.TacticalContext.bMyTeamHasPossession || Data.ElapsedPressSec >= Data.MaxPressDurationSec)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// Close down ball carrier
	const FVector ToBall = (Data.TacticalContext.BallLocation - Player->GetActorLocation()).GetSafeNormal2D();
	Player->AddMovementInput(ToBall, 1.0f);

	if (Data.TacticalContext.DistanceToBallCm < 145.0f)
	{
		Player->PerformTackle();
	}

	return EStateTreeRunStatus::Running;
}

void FFootballTask_PressCarrier::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	const FInstanceDataType& Data = Context.GetInstanceData(*this);
	if (Data.FootballPlayer)
	{
		Data.FootballPlayer->Input_SprintStop();
	}
}

// ============================================================================
// FFootballTask_InterceptBall
// ============================================================================

EStateTreeRunStatus FFootballTask_InterceptBall::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedInterceptSec = 0.0f;
	if (Data.FootballPlayer)
	{
		Data.FootballPlayer->CurrentTacticalStateLabel = TEXT("Intercept");
		Data.FootballPlayer->Input_SprintStart();
	}
	return EStateTreeRunStatus::Running;
}

EStateTreeRunStatus FFootballTask_InterceptBall::Tick(FStateTreeExecutionContext& Context, const float DeltaTime) const
{
	FInstanceDataType& Data = Context.GetInstanceData(*this);
	Data.ElapsedInterceptSec += DeltaTime;

	AFootballCharacter* Player = Data.FootballPlayer;
	if (!Player)
	{
		return EStateTreeRunStatus::Failed;
	}

	if (Data.TacticalContext.bMyTeamHasPossession)
	{
		return EStateTreeRunStatus::Succeeded;
	}

	// Abort intercept if ball was captured cleanly by opponent out of reach or timed out
	if ((!Data.TacticalContext.bBallIsLooseOrInFlight && Data.ElapsedInterceptSec > 0.35f) || Data.ElapsedInterceptSec > 2.6f)
	{
		return EStateTreeRunStatus::Failed;
	}

	// Sprint directly toward the Magnus-predicted intercept spot
	const FVector Target = Data.TacticalContext.PredictedInterceptPoint;
	const FVector Dir = (Target - Player->GetActorLocation()).GetSafeNormal2D();
	Player->AddMovementInput(Dir, 1.0f);

	if (FVector::Dist2D(Player->GetActorLocation(), Data.TacticalContext.BallLocation) < 130.0f)
	{
		Player->PerformTackle();
		return EStateTreeRunStatus::Succeeded;
	}

	return EStateTreeRunStatus::Running;
}
