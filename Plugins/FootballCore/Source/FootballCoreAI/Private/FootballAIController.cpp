// Copyright Craquez Studio. All Rights Reserved.

#include "FootballAIController.h"
#include "FootballGOAPPlanner.h"
#include "FootballCharacter.h"
#include "FootballBallActor.h"
#include "Components/StateTreeAIComponent.h"
#include "EngineUtils.h"

AFootballAIController::AFootballAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	TacticalStateTreeComponent = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("TacticalStateTreeComponent"));
}

void AFootballAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (bUseExternalStateTreeAsset && TacticalStateTreeComponent)
	{
		TacticalStateTreeComponent->StartLogic();
	}
}

void AFootballAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bUseExternalStateTreeAsset)
	{
		return;
	}

	AFootballCharacter* Player = Cast<AFootballCharacter>(GetPawn());
	if (!Player)
	{
		return;
	}

	AFootballBallActor* Ball = Player->GetMatchBall();
	if (!Ball)
	{
		return;
	}

	if (ActionCooldown > 0.0f)
	{
		ActionCooldown = FMath::Max(0.0f, ActionCooldown - DeltaTime);
	}

	RefreshTacticalContext(Player, Ball, DeltaTime);

	// Re-evaluate tactical state cleanly every tick (matches StateTree OnTick transition fix!)
	const EFootballTacticalState DesiredState = UFootballGOAPPlanner::EvaluateBestTacticalState(Player, LiveContext, ActiveTacticalState);
	if (DesiredState != ActiveTacticalState)
	{
		ActiveTacticalState = DesiredState;
		LiveContext.TimeInCurrentStateSec = 0.0f;
	}

	ExecuteTacticalState(Player, Ball, DeltaTime);
}

void AFootballAIController::RefreshTacticalContext(AFootballCharacter* Player, AFootballBallActor* Ball, float DeltaTime)
{
	LiveContext.TimeInCurrentStateSec += DeltaTime;
	LiveContext.BallLocation = Ball->GetActorLocation();
	LiveContext.BallVelocity = Ball->GetBallVelocity();
	LiveContext.DistanceToBallCm = FVector::Dist2D(Player->GetActorLocation(), LiveContext.BallLocation);

	AFootballCharacter* Owner = Ball->GetCurrentOwnerPlayer();
	LiveContext.bIHavePossession = (Owner == Player);
	LiveContext.bMyTeamHasPossession = (Owner != nullptr && Owner->TeamSide == Player->TeamSide);
	LiveContext.bBallIsLooseOrInFlight = Ball->IsBallLooseOrInFlight();

	const float SprintSpeed = 680.0f;
	const float LookaheadSec = FMath::Clamp(LiveContext.DistanceToBallCm / SprintSpeed, 0.20f, 1.0f);
	LiveContext.PredictedInterceptPoint = Ball->PredictPositionAtTime(LookaheadSec, 10);
	LiveContext.BallTimeToInterceptSec = LookaheadSec;
	LiveContext.TimeToReachInterceptSec = FVector::Dist2D(Player->GetActorLocation(), LiveContext.PredictedInterceptPoint) / SprintSpeed;
	LiveContext.bAmClosestDefendingTeammate = IsClosestTeammateToBall(Player, LiveContext.BallLocation);
}

void AFootballAIController::ExecuteTacticalState(AFootballCharacter* Player, AFootballBallActor* Ball, float DeltaTime)
{
	const FVector AttackGoal = Player->GetAttackingGoalLocation();
	const FVector DefendGoal = Player->GetDefendingGoalLocation();

	switch (ActiveTacticalState)
	{
	case EFootballTacticalState::HoldFormation:
	{
		Player->CurrentTacticalStateLabel = TEXT("Formation");
		// Shift formation dynamically with ball X/Y position (compact tactical block!)
		const FVector BallShift(LiveContext.BallLocation.X * 0.28f, LiveContext.BallLocation.Y * 0.25f, 0.0f);
		FVector Target = Player->FormationHomeLocation + BallShift;
		Target.X = FMath::Clamp(Target.X, -4600.0f, 4600.0f);
		Target.Y = FMath::Clamp(Target.Y, -3100.0f, 3100.0f);
		SteerPlayerToward(Player, Target, false);
		break;
	}

	case EFootballTacticalState::SupportRun:
	{
		Player->CurrentTacticalStateLabel = TEXT("SupportRun");
		const float AttackDirX = (Player->TeamSide == EFootballTeamSide::Home) ? 1.0f : -1.0f;
		FVector Target = Player->FormationHomeLocation + FVector(AttackDirX * 1450.0f, LiveContext.BallLocation.Y * 0.30f, 0.0f);
		Target.X = FMath::Clamp(Target.X, -4500.0f, 4500.0f);
		Target.Y = FMath::Clamp(Target.Y, -3050.0f, 3050.0f);
		SteerPlayerToward(Player, Target, true);
		break;
	}

	case EFootballTacticalState::Press:
	{
		Player->CurrentTacticalStateLabel = TEXT("Press");
		SteerPlayerToward(Player, LiveContext.BallLocation, true);
		if (LiveContext.DistanceToBallCm < 140.0f && ActionCooldown <= 0.0f)
		{
			Player->PerformTackle();
			ActionCooldown = 0.85f;
		}
		break;
	}

	case EFootballTacticalState::Intercept:
	{
		Player->CurrentTacticalStateLabel = TEXT("Intercept");
		SteerPlayerToward(Player, LiveContext.PredictedInterceptPoint, true);
		if (LiveContext.DistanceToBallCm < 130.0f && ActionCooldown <= 0.0f)
		{
			Player->PerformTackle();
			ActionCooldown = 0.65f;
		}
		break;
	}

	case EFootballTacticalState::DribbleAttack:
	{
		Player->CurrentTacticalStateLabel = TEXT("Dribble");
		SteerPlayerToward(Player, AttackGoal, true);
		break;
	}

	case EFootballTacticalState::PassToTeammate:
	{
		Player->CurrentTacticalStateLabel = TEXT("Pass");
		if (ActionCooldown <= 0.0f)
		{
			Player->PerformPass(nullptr, false);
			ActionCooldown = 1.0f;
		}
		break;
	}

	case EFootballTacticalState::ShootAtGoal:
	{
		Player->CurrentTacticalStateLabel = TEXT("Shoot");
		if (ActionCooldown <= 0.0f)
		{
			// Aim for the far corner with realistic Magnus finesse curl!
			const float CornerY = (Player->GetActorLocation().Y >= 0.0f) ? -285.0f : 285.0f;
			const FVector CornerTarget(AttackGoal.X, CornerY, 130.0f);
			const FVector ShootDir = (CornerTarget - LiveContext.BallLocation).GetSafeNormal();
			const float CurveSign = (CornerY < 0.0f) ? 0.65f : -0.65f;
			Player->PerformShot(ShootDir, 0.82f, CurveSign);
			ActionCooldown = 1.2f;
		}
		break;
	}

	case EFootballTacticalState::GoalkeeperPosition:
	{
		Player->CurrentTacticalStateLabel = TEXT("GK_Guard");
		// Bisect the angle between the ball and center of goal line!
		const FVector GoalToBall = (LiveContext.BallLocation - DefendGoal).GetSafeNormal2D();
		const float OutFromLine = FMath::Clamp(FVector::Dist2D(LiveContext.BallLocation, DefendGoal) * 0.14f, 120.0f, 520.0f);
		FVector GuardPos = DefendGoal + GoalToBall * OutFromLine;
		GuardPos.Y = FMath::Clamp(GuardPos.Y, -320.0f, 320.0f);
		SteerPlayerToward(Player, GuardPos, true);

		// If keeper grabbed the ball, distribute quickly upfield!
		if (LiveContext.bIHavePossession && ActionCooldown <= 0.0f)
		{
			if (!Player->PerformPass(nullptr, true))
			{
				Player->PerformShot((AttackGoal - Player->GetActorLocation()).GetSafeNormal2D(), 0.75f, 0.0f);
			}
			ActionCooldown = 1.5f;
		}
		break;
	}

	case EFootballTacticalState::GoalkeeperDive:
	{
		Player->CurrentTacticalStateLabel = TEXT("GK_Dive");
		// Predict Magnus curved ball trajectory at 0.35s ahead and dive to intercept!
		const FVector ShotIntercept = Ball->PredictPositionAtTime(0.35f, 10);
		FVector ClampedSaveSpot = ShotIntercept;
		ClampedSaveSpot.X = DefendGoal.X + ((Player->TeamSide == EFootballTeamSide::Home) ? 160.0f : -160.0f);
		ClampedSaveSpot.Y = FMath::Clamp(ClampedSaveSpot.Y, -360.0f, 360.0f);

		SteerPlayerToward(Player, ClampedSaveSpot, true);
		if (FVector::Dist2D(Player->GetActorLocation(), LiveContext.BallLocation) < 950.0f)
		{
			Player->PerformGoalkeeperDive(ClampedSaveSpot);
		}
		break;
	}
	}
}

bool AFootballAIController::IsClosestTeammateToBall(const AFootballCharacter* Player, const FVector& BallLoc) const
{
	if (!Player || !GetWorld())
	{
		return false;
	}

	const float MyDistSq = FVector::DistSquared2D(Player->GetActorLocation(), BallLoc);
	for (TActorIterator<AFootballCharacter> It(GetWorld()); It; ++It)
	{
		const AFootballCharacter* Other = *It;
		if (!Other || Other == Player || Other->TeamSide != Player->TeamSide || Other->PlayerRole == EFootballPlayerRole::Goalkeeper)
		{
			continue;
		}
		if (FVector::DistSquared2D(Other->GetActorLocation(), BallLoc) < MyDistSq)
		{
			return false;
		}
	}
	return true;
}

void AFootballAIController::SteerPlayerToward(AFootballCharacter* Player, const FVector& TargetLocation, bool bSprint) const
{
	if (!Player)
	{
		return;
	}

	const FVector Delta = TargetLocation - Player->GetActorLocation();
	const float Dist2D = Delta.Size2D();
	if (Dist2D < 55.0f)
	{
		Player->Input_SprintStop();
		return;
	}

	if (bSprint && Dist2D > 180.0f)
	{
		Player->Input_SprintStart();
	}
	else
	{
		Player->Input_SprintStop();
	}

	Player->AddMovementInput(Delta.GetSafeNormal2D(), 1.0f);
}
