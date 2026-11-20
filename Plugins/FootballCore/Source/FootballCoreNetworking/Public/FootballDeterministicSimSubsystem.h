// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "FootballDeterministicTypes.h"
#include "FootballDeterministicSimSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnFootballDeterministicTickSignature, int32, SimulationTick, float, FixedDeltaTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFootballSnapshotRecordedSignature, const FFootballDeterministicFrameSnapshot&, Snapshot);

/**
 * World Subsystem coordinating deterministic fixed-step simulation,
 * ring-buffer snapshot history (rollback/instant replay), and Blueprint replica dispatch.
 */
UCLASS(BlueprintType)
class FOOTBALLCORENETWORKING_API UFootballDeterministicSimSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	/** Register an Actor/Object implementing IFootballDeterministicReplica */
	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	void RegisterReplica(UObject* ReplicaObject);

	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	void UnregisterReplica(UObject* ReplicaObject);

	/** Reset simulation tick counter and RNG seed (called at match/round start) */
	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	void ResetSimulation(int32 MatchSeed = 20261120);

	/** Record latest ball/match state snapshot into ring buffer */
	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	void RecordSnapshot(const FFootballDeterministicBallState& BallState, int32 HomeScore, int32 AwayScore);

	/** Fetch a past snapshot for instant replay or rollback reconciliation */
	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	bool GetSnapshotForTick(int32 TargetTick, FFootballDeterministicFrameSnapshot& OutSnapshot) const;

	/** Roll back all registered replicas to a specific snapshot tick */
	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	bool RollbackToTick(int32 TargetTick);

	UFUNCTION(BlueprintPure, Category = "Football|Deterministic")
	int32 GetCurrentTick() const { return CurrentSimulationTick; }

	UFUNCTION(BlueprintPure, Category = "Football|Deterministic")
	float GetFixedDeltaTime() const { return FixedTimeStep; }

	UFUNCTION(BlueprintCallable, Category = "Football|Deterministic")
	float NextDeterministicFloat(float MinVal = 0.0f, float MaxVal = 1.0f);

	UPROPERTY(BlueprintAssignable, Category = "Football|Deterministic")
	FOnFootballDeterministicTickSignature OnDeterministicTick;

	UPROPERTY(BlueprintAssignable, Category = "Football|Deterministic")
	FOnFootballSnapshotRecordedSignature OnSnapshotRecorded;

protected:
	/** Fixed simulation step (60 Hz default for Android battery efficiency + deterministic accuracy) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Football|Deterministic")
	float FixedTimeStep = 1.0f / 60.0f;

	/** Number of historical snapshots retained (120 frames = 2 seconds at 60 Hz) */
	static constexpr int32 SnapshotHistorySize = 120;

private:
	double AccumulatorSeconds = 0.0;
	int32 CurrentSimulationTick = 0;

	FFootballDeterministicRNG MatchRNG;
	TArray<FFootballDeterministicFrameSnapshot> SnapshotRingBuffer;
	TArray<TWeakObjectPtr<UObject>> RegisteredReplicas;
};
