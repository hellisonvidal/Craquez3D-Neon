// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballTypes.h"
#include "FootballBlueprintInterface.h"
#include "FootballDeterministicTypes.h"
#include "FootballBallActor.generated.h"

class USphereComponent;
class UProceduralMeshComponent;
class UStaticMeshComponent;
class UNiagaraComponent;
class AFootballCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnFootballBallImpactSignature, EFootballBallSurface, Surface, float, ImpactSpeed, FVector, ImpactLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFootballBallKickedSignature, const FFootballKickRequest&, Request, AFootballCharacter*, Kicker);

/**
 * Physically accurate FIFA Size-5 Football powered by Chaos Physics with:
 * - Magnus Effect (lateral/vertical curve from spin w x v)
 * - Quadratic aerodynamic drag & dynamic weather wind
 * - Wet/dry turf rolling friction & spin-to-velocity bounce transfer
 * - Impact deformation spring (squash/stretch exposed to materials & procedural scale)
 * - Swept CCD collision so fast shots never tunnel through posts or keeper hands
 * - Deterministic snapshot serialization & Magnus trajectory prediction for AI
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREGAMEPLAY_API AFootballBallActor : public AActor, public IFootballDeterministicReplica, public IFootballBlueprintInterface
{
	GENERATED_BODY()

public:
	AFootballBallActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

	/** Execute a kick with exact exit speed & 3D Magnus spin vector */
	UFUNCTION(BlueprintCallable, Category = "Football|BallPhysics")
	void ExecuteKick(const FFootballKickRequest& Request, AFootballCharacter* Kicker = nullptr);

	/** Convenience helper for finesse curl shots (calculates Magnus spin automatically) */
	UFUNCTION(BlueprintCallable, Category = "Football|BallPhysics")
	void ExecuteCurvedShot(FVector TargetDirection, float PowerNormalized, float CurveRightLeftNormalized, AFootballCharacter* Kicker = nullptr);

	/** Reset ball cleanly to kickoff or 1v1 spot */
	UFUNCTION(BlueprintCallable, Category = "Football|BallPhysics")
	void ResetBallToLocation(const FVector& NewWorldLocation);

	/** Set ambient weather wind (cm/s) and pitch wetness [0..1] affecting Magnus, drag & turf friction */
	UFUNCTION(BlueprintCallable, Category = "Football|BallPhysics")
	void SetEnvironmentConditions(const FVector& InWindVelocityCmPerSec, float InPitchWetness);

	/**
	 * Predict future ball world location integrating Gravity + Drag + Magnus curve.
	 * Used by Goalkeeper Dive AI and Defender Intercept AI (StateTree).
	 */
	UFUNCTION(BlueprintPure, Category = "Football|BallPhysics")
	FVector PredictPositionAtTime(float SecondsAhead, int32 Substeps = 12) const;

	/** Get linear velocity in cm/s */
	UFUNCTION(BlueprintPure, Category = "Football|BallPhysics")
	FVector GetBallVelocity() const;

	/** Get angular velocity (spin) in rad/s */
	UFUNCTION(BlueprintPure, Category = "Football|BallPhysics")
	FVector GetBallSpinRadPerSec() const;

	/** Returns true when ball is flying fast or unowned (key condition for StateTree Intercept) */
	UFUNCTION(BlueprintPure, Category = "Football|BallPhysics")
	bool IsBallLooseOrInFlight() const;

	UFUNCTION(BlueprintPure, Category = "Football|BallPhysics")
	bool IsBallGrounded() const;

	UFUNCTION(BlueprintPure, Category = "Football|BallPhysics")
	AFootballCharacter* GetCurrentOwnerPlayer() const { return CurrentOwnerPlayer.Get(); }

	UFUNCTION(BlueprintCallable, Category = "Football|BallPhysics")
	void SetCurrentOwnerPlayer(AFootballCharacter* NewOwner);

	/** Capture deterministic network snapshot */
	UFUNCTION(BlueprintPure, Category = "Football|Deterministic")
	FFootballDeterministicBallState CaptureDeterministicState() const;

	// --- IFootballDeterministicReplica ---
	virtual void OnDeterministicStep_Implementation(int32 SimulationTick, float FixedDeltaTime) override;
	virtual int32 GetDeterministicStateHash_Implementation() const override;
	virtual void ApplyDeterministicSnapshot_Implementation(const FFootballDeterministicFrameSnapshot& Snapshot) override;

	// --- IFootballBlueprintInterface ---
	virtual void OnFootballKickExecuted_Implementation(const FFootballKickRequest& KickRequest, FVector ContactWorldLocation) override;
	virtual void OnFootballBallImpact_Implementation(EFootballBallSurface Surface, float ImpactSpeedCmPerSec, FVector ImpactPoint) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Procedural geodesic soccer ball so the project is immediately playable with zero external imports */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UProceduralMeshComponent> ProceduralBallMesh;

	/** Optional high-res StaticMesh override (when importing a free Fab/Blender soccer ball mesh) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UStaticMeshComponent> CustomBallMesh;

	/** Optional Niagara Magnus airflow / neon spin trail */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UNiagaraComponent> MagnusTrailNiagara;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	FFootballBallAerodynamicsConfig AeroConfig;

	/** Current elastic squash deformation [0..MaxDeformationRatio] for shaders / WPO */
	UPROPERTY(BlueprintReadOnly, Category = "Football|BallPhysics|Deformation")
	float CurrentDeformationAmount = 0.0f;

	/** World-space axis along which the ball is currently compressed */
	UPROPERTY(BlueprintReadOnly, Category = "Football|BallPhysics|Deformation")
	FVector CurrentDeformationAxis = FVector::UpVector;

	UPROPERTY(BlueprintAssignable, Category = "Football|Events")
	FOnFootballBallImpactSignature OnBallImpact;

	UPROPERTY(BlueprintAssignable, Category = "Football|Events")
	FOnFootballBallKickedSignature OnBallKicked;

protected:
	UFUNCTION()
	void HandleBallHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void BuildProceduralFootballMesh();
	void ApplyAerodynamicForces(float DeltaTime);
	void UpdateDeformationSpring(float DeltaTime);
	void ExciteDeformation(const FVector& ImpactNormal, float ImpactSpeedCmPerSec);

	FVector WindVelocityCmPerSec = FVector::ZeroVector;
	float PitchWetness = 0.0f; // 0 = dry, 1 = soaked rain pitch
	float DeformationVelocity = 0.0f;
	float TimeSinceLastKick = 10.0f;

	TWeakObjectPtr<AFootballCharacter> CurrentOwnerPlayer;
	int32 LastTouchPlayerId = -1;
};
