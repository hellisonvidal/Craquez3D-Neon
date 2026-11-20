// Copyright Craquez Studio. All Rights Reserved.

#include "FootballProceduralAnimComponent.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "MotionWarpingComponent.h"
#include "Engine/World.h"

UFootballProceduralAnimComponent::UFootballProceduralAnimComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UFootballProceduralAnimComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AActor* Owner = GetOwner())
	{
		PreviousYaw = Owner->GetActorRotation().Yaw;
	}
}

void UFootballProceduralAnimComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateFootPlacementIK(DeltaTime);
	UpdatePhysicsReactionRecovery(DeltaTime);
	UpdateProceduralLean(DeltaTime);

	if (KickIKFadeTimer > 0.0f)
	{
		KickIKFadeTimer = FMath::Max(0.0f, KickIKFadeTimer - DeltaTime);
		KickIKAlpha = FMath::Clamp(KickIKFadeTimer / 0.25f, 0.0f, 1.0f);
	}
}

void UFootballProceduralAnimComponent::PrepareKickMotionWarp(const FVector& BallWorldLocation, const FVector& KickDirection, EFootballKickStyle Style, bool bUseRightFoot)
{
	const FVector SafeDir = KickDirection.GetSafeNormal2D();
	// Offset slightly behind & to the side of the ball based on kick style (finesse vs power instep)
	const float SideSign = bUseRightFoot ? 1.0f : -1.0f;
	const FVector RightVec = FVector::CrossProduct(FVector::UpVector, SafeDir);
	const float LateralOffset = (Style == EFootballKickStyle::FinesseCurl) ? (14.0f * SideSign) : (8.0f * SideSign);

	KickFootTargetWorld = BallWorldLocation - (SafeDir * 12.0f) + (RightVec * LateralOffset);
	KickIKAlpha = 1.0f;
	KickIKFadeTimer = 0.25f;

	if (AActor* Owner = GetOwner())
	{
		if (UMotionWarpingComponent* MotionWarp = Owner->FindComponentByClass<UMotionWarpingComponent>())
		{
			const FVector PlantFootPos = BallWorldLocation - (SafeDir * 42.0f) - (RightVec * (24.0f * SideSign));
			const FRotator FacingRot = SafeDir.Rotation();
			MotionWarp->AddOrUpdateWarpTargetFromLocationAndRotation(TEXT("KickBallTarget"), PlantFootPos, FacingRot);
		}
	}
}

void UFootballProceduralAnimComponent::TriggerEuphoriaImpact(const FVector& WorldImpulse, FName ImpactBoneName, float BlendDuration)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar)
	{
		return;
	}

	PhysicsRecoveryTotalDuration = FMath::Max(BlendDuration, 0.15f);
	PhysicsRecoveryTimer = PhysicsRecoveryTotalDuration;
	CurrentPhysicsReactionBlend = 0.72f;

	if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
	{
		if (Mesh->GetSkeletalMeshAsset() && Mesh->GetBoneIndex(ImpactBoneName) != INDEX_NONE)
		{
			// Blend upper-body physics below spine while legs continue fighting for balance (Euphoria principle)
			Mesh->SetAllBodiesBelowSimulatePhysics(ImpactBoneName, true, false);
			Mesh->SetAllBodiesBelowPhysicsBlendWeight(ImpactBoneName, CurrentPhysicsReactionBlend, false, false);
			Mesh->AddImpulse(WorldImpulse, ImpactBoneName, true);
		}
	}
}

TArray<FFootballTrajectorySample> UFootballProceduralAnimComponent::ComputeMotionMatchingTrajectory(const FVector& DesiredInputDir, float TargetSpeedCmPerSec) const
{
	TArray<FFootballTrajectorySample> Samples;
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return Samples;
	}

	const float TimeHorizons[4] = { 0.15f, 0.35f, 0.60f, 0.90f };
	const FVector StartPos = Owner->GetActorLocation();
	const FVector CurVel = Owner->GetVelocity();
	const FVector DesiredVel = DesiredInputDir.GetSafeNormal2D() * TargetSpeedCmPerSec;

	for (float T : TimeHorizons)
	{
		FFootballTrajectorySample Sample;
		Sample.TimeOffsetSeconds = T;
		// Critically damped velocity blend predicts realistic footballer acceleration curve
		const float Blend = 1.0f - FMath::Exp(-5.5f * T);
		const FVector AvgVel = FMath::Lerp(CurVel, DesiredVel, Blend);
		Sample.WorldLocation = StartPos + AvgVel * T;
		Sample.WorldRotation = AvgVel.SizeSquared2D() > 100.0f ? AvgVel.Rotation() : Owner->GetActorRotation();
		Samples.Add(Sample);
	}
	return Samples;
}

void UFootballProceduralAnimComponent::UpdateFootPlacementIK(float DeltaTime)
{
	ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
	if (!OwnerChar || !GetWorld())
	{
		return;
	}

	const FVector ActorLoc = OwnerChar->GetActorLocation();
	const FVector RightVec = OwnerChar->GetActorRightVector();

	auto TraceFoot = [this, OwnerChar](const FVector& Origin, FVector& OutHitLoc) -> float
	{
		FHitResult Hit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(OwnerChar);
		const FVector Start = Origin + FVector(0.0f, 0.0f, 30.0f);
		const FVector End = Origin - FVector(0.0f, 0.0f, FootTraceDistance);
		if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			OutHitLoc = Hit.ImpactPoint;
			return Hit.ImpactPoint.Z - (Origin.Z - 88.0f);
		}
		OutHitLoc = Origin - FVector(0.0f, 0.0f, 88.0f);
		return 0.0f;
	};

	FVector TargetLeft, TargetRight;
	const float LeftOffset = TraceFoot(ActorLoc - RightVec * 14.0f, TargetLeft);
	const float RightOffset = TraceFoot(ActorLoc + RightVec * 14.0f, TargetRight);

	LeftFootIKLocation = FMath::VInterpTo(LeftFootIKLocation, TargetLeft, DeltaTime, IKInterpSpeed);
	RightFootIKLocation = FMath::VInterpTo(RightFootIKLocation, TargetRight, DeltaTime, IKInterpSpeed);

	const float DesiredPelvis = FMath::Min(LeftOffset, RightOffset);
	PelvisZOffset = FMath::FInterpTo(PelvisZOffset, FMath::Clamp(DesiredPelvis, -22.0f, 10.0f), DeltaTime, IKInterpSpeed);
}

void UFootballProceduralAnimComponent::UpdatePhysicsReactionRecovery(float DeltaTime)
{
	if (PhysicsRecoveryTimer <= 0.0f)
	{
		return;
	}

	PhysicsRecoveryTimer = FMath::Max(0.0f, PhysicsRecoveryTimer - DeltaTime);
	const float Alpha = PhysicsRecoveryTimer / FMath::Max(PhysicsRecoveryTotalDuration, 0.01f);
	CurrentPhysicsReactionBlend = Alpha * 0.72f;

	if (ACharacter* OwnerChar = Cast<ACharacter>(GetOwner()))
	{
		if (USkeletalMeshComponent* Mesh = OwnerChar->GetMesh())
		{
			if (Mesh->GetSkeletalMeshAsset() && Mesh->GetBoneIndex(TEXT("spine_02")) != INDEX_NONE)
			{
				if (PhysicsRecoveryTimer <= KINDA_SMALL_NUMBER)
				{
					Mesh->SetAllBodiesBelowSimulatePhysics(TEXT("spine_02"), false, false);
				}
				else
				{
					Mesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("spine_02"), CurrentPhysicsReactionBlend, false, false);
				}
			}
		}
	}
}

void UFootballProceduralAnimComponent::UpdateProceduralLean(float DeltaTime)
{
	AActor* Owner = GetOwner();
	if (!Owner || DeltaTime <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float CurrentYaw = Owner->GetActorRotation().Yaw;
	const float YawDelta = FMath::FindDeltaAngleDegrees(PreviousYaw, CurrentYaw);
	PreviousYaw = CurrentYaw;

	const float YawRate = YawDelta / DeltaTime;
	const float SpeedRatio = FMath::Clamp(Owner->GetVelocity().Size2D() / 680.0f, 0.0f, 1.0f);
	const float TargetLean = FMath::Clamp(YawRate * 0.08f * SpeedRatio, -20.0f, 20.0f);
	ProceduralBodyLeanDegrees = FMath::FInterpTo(ProceduralBodyLeanDegrees, TargetLean, DeltaTime, 8.0f);
}
