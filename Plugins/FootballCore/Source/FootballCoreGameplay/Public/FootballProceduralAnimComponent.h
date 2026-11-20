// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootballTypes.h"
#include "FootballProceduralAnimComponent.generated.h"

class USkeletalMeshComponent;
class UMotionWarpingComponent;

/**
 * Predicted future locomotion sample consumed by UE5 PoseSearch (Motion Matching).
 */
USTRUCT(BlueprintType)
struct FOOTBALLCOREGAMEPLAY_API FFootballTrajectorySample
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation")
	float TimeOffsetSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation")
	FRotator WorldRotation = FRotator::ZeroRotator;
};

/**
 * Procedural Animation Controller combining:
 * 1. Dual-Foot Ground IK (Control Rig / AnimBP targets + Pelvis offset)
 * 2. Kick Target IK & Motion Warping (aligns striking foot to moving/spinning ball)
 * 3. Euphoria-Style Physics Impact Blending (smooth shoulder-charge/tackle balance recovery)
 * 4. Motion Matching (PoseSearch) future trajectory prediction buffer
 */
UCLASS(ClassGroup = (Football), meta = (BlueprintSpawnableComponent))
class FOOTBALLCOREGAMEPLAY_API UFootballProceduralAnimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootballProceduralAnimComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Configure Control Rig / Motion Warping kick warp target for exact boot-to-ball contact */
	UFUNCTION(BlueprintCallable, Category = "Football|Animation|IK")
	void PrepareKickMotionWarp(const FVector& BallWorldLocation, const FVector& KickDirection, EFootballKickStyle Style, bool bUseRightFoot = true);

	/** Trigger Euphoria-style physics-driven reaction on tackle/shoulder collision, recovering smoothly */
	UFUNCTION(BlueprintCallable, Category = "Football|Animation|Physics")
	void TriggerEuphoriaImpact(const FVector& WorldImpulse, FName ImpactBoneName = TEXT("spine_02"), float BlendDuration = 0.55f);

	/** Compute predicted future path samples for UE5 PoseSearch (Motion Matching) */
	UFUNCTION(BlueprintPure, Category = "Football|Animation|MotionMatching")
	TArray<FFootballTrajectorySample> ComputeMotionMatchingTrajectory(const FVector& DesiredInputDir, float TargetSpeedCmPerSec) const;

	// --- AnimBP / Control Rig Readable Outputs ---
	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|IK")
	FVector LeftFootIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|IK")
	FVector RightFootIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|IK")
	float PelvisZOffset = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|IK")
	FVector KickFootTargetWorld = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|IK")
	float KickIKAlpha = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|Physics")
	float CurrentPhysicsReactionBlend = 0.0f;

	/** Lean angle driven by angular velocity for realistic cutting & sprinting */
	UPROPERTY(BlueprintReadOnly, Category = "Football|Animation|Procedural")
	float ProceduralBodyLeanDegrees = 0.0f;

protected:
	void UpdateFootPlacementIK(float DeltaTime);
	void UpdatePhysicsReactionRecovery(float DeltaTime);
	void UpdateProceduralLean(float DeltaTime);

	UPROPERTY(EditAnywhere, Category = "Football|Animation|IK")
	float FootTraceDistance = 55.0f;

	UPROPERTY(EditAnywhere, Category = "Football|Animation|IK")
	float IKInterpSpeed = 16.0f;

	float PhysicsRecoveryTimer = 0.0f;
	float PhysicsRecoveryTotalDuration = 0.5f;
	float KickIKFadeTimer = 0.0f;
	float PreviousYaw = 0.0f;
};
