// Copyright Craquez Studio. All Rights Reserved.

#include "FootballGOAPPlanner.h"
#include "FootballCharacter.h"

EFootballTacticalState UFootballGOAPPlanner::EvaluateBestTacticalState(
	const AFootballCharacter* Player,
	const FFootballTacticalContext& Context,
	EFootballTacticalState CurrentState)
{
	if (!Player)
	{
		return EFootballTacticalState::HoldFormation;
	}

	// Dedicated Goalkeeper tactical pipeline
	if (Player->PlayerRole == EFootballPlayerRole::Goalkeeper)
	{
		const FVector DefGoal = Player->GetDefendingGoalLocation();
		const FVector BallToGoal = (DefGoal - Context.BallLocation).GetSafeNormal2D();
		const float BallSpeed = Context.BallVelocity.Size2D();
		const float HeadingTowardGoal = FVector::DotProduct(Context.BallVelocity.GetSafeNormal2D(), BallToGoal);
		const float DistBallToGoal = FVector::Dist2D(Context.BallLocation, DefGoal);

		if (BallSpeed > 900.0f && HeadingTowardGoal > 0.65f && DistBallToGoal < 2800.0f)
		{
			return EFootballTacticalState::GoalkeeperDive;
		}
		return EFootballTacticalState::GoalkeeperPosition;
	}

	// 1. In Possession: Shoot, Pass, or Dribble toward goal
	if (Context.bIHavePossession)
	{
		const float DistToOppGoal = FVector::Dist2D(Player->GetActorLocation(), Player->GetAttackingGoalLocation());
		if (DistToOppGoal < 2300.0f)
		{
			return EFootballTacticalState::ShootAtGoal;
		}
		if (Context.TimeInCurrentStateSec > 1.35f && DistToOppGoal < 5500.0f)
		{
			return EFootballTacticalState::PassToTeammate;
		}
		return EFootballTacticalState::DribbleAttack;
	}

	// 2. Teammate Has Possession: Make intelligent attacking support runs
	if (Context.bMyTeamHasPossession)
	{
		return EFootballTacticalState::SupportRun;
	}

	// 3. Defending / Loose Ball: Clean Press -> Intercept transition with hysteresis!
	if (Context.bBallIsLooseOrInFlight &&
		Context.DistanceToBallCm < 1950.0f &&
		Context.TimeToReachInterceptSec <= (Context.BallTimeToInterceptSec + 0.25f))
	{
		return EFootballTacticalState::Intercept;
	}

	// Hysteresis: If already intercepting a fast loose ball, stay committed briefly (0.35s)
	if (CurrentState == EFootballTacticalState::Intercept &&
		Context.TimeInCurrentStateSec < 0.35f &&
		Context.DistanceToBallCm < 1600.0f)
	{
		return EFootballTacticalState::Intercept;
	}

	// Closest defender presses opponent ball carrier
	if (Context.bAmClosestDefendingTeammate || Context.DistanceToBallCm < 1350.0f)
	{
		return EFootballTacticalState::Press;
	}

	return EFootballTacticalState::HoldFormation;
}

FFootballRLObservation UFootballGOAPPlanner::BuildRLObservation(const AFootballCharacter* Player, const FFootballTacticalContext& Context)
{
	FFootballRLObservation Obs;
	if (!Player)
	{
		return Obs;
	}

	const FVector P = Player->GetActorLocation();
	Obs.NormalizedPlayerPos = FVector2D(FMath::Clamp(P.X / 5250.0f, -1.f, 1.f), FMath::Clamp(P.Y / 3400.0f, -1.f, 1.f));
	Obs.NormalizedBallPos = FVector2D(FMath::Clamp(Context.BallLocation.X / 5250.0f, -1.f, 1.f), FMath::Clamp(Context.BallLocation.Y / 3400.0f, -1.f, 1.f));
	Obs.NormalizedBallVel = FVector2D(FMath::Clamp(Context.BallVelocity.X / 3000.0f, -1.f, 1.f), FMath::Clamp(Context.BallVelocity.Y / 3000.0f, -1.f, 1.f));
	Obs.DistanceToAttackingGoalNorm = FMath::Clamp(FVector::Dist2D(P, Player->GetAttackingGoalLocation()) / 10500.0f, 0.0f, 1.0f);
	Obs.StaminaNorm = Player->StaminaNormalized;
	Obs.PossessionFlag = Context.bMyTeamHasPossession ? 1.0f : (Context.bBallIsLooseOrInFlight ? 0.0f : -1.0f);
	return Obs;
}
