// Copyright Craquez Studio. All Rights Reserved.

#include "FootballDeterministicSimSubsystem.h"
#include "FootballCoreNetworkingModule.h"

void UFootballDeterministicSimSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	SnapshotRingBuffer.SetNum(SnapshotHistorySize);
	ResetSimulation(20261120);
}

void UFootballDeterministicSimSubsystem::Deinitialize()
{
	RegisteredReplicas.Reset();
	SnapshotRingBuffer.Reset();
	Super::Deinitialize();
}

TStatId UFootballDeterministicSimSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFootballDeterministicSimSubsystem, STATGROUP_Tickables);
}

void UFootballDeterministicSimSubsystem::Tick(float DeltaTime)
{
	// Clamp frame delta to avoid spiral of death on mobile thermal throttling
	const double ClampedDelta = FMath::Min(static_cast<double>(DeltaTime), 0.1);
	AccumulatorSeconds += ClampedDelta;

	int32 StepsThisFrame = 0;
	constexpr int32 MaxStepsPerFrame = 4;

	while (AccumulatorSeconds >= static_cast<double>(FixedTimeStep) && StepsThisFrame < MaxStepsPerFrame)
	{
		AccumulatorSeconds -= static_cast<double>(FixedTimeStep);
		++CurrentSimulationTick;
		++StepsThisFrame;

		// Dispatch fixed-step update to registered deterministic replicas
		for (int32 i = RegisteredReplicas.Num() - 1; i >= 0; --i)
		{
			if (UObject* Obj = RegisteredReplicas[i].Get())
			{
				if (Obj->Implements<UFootballDeterministicReplica>())
				{
					IFootballDeterministicReplica::Execute_OnDeterministicStep(Obj, CurrentSimulationTick, FixedTimeStep);
				}
			}
			else
			{
				RegisteredReplicas.RemoveAtSwap(i);
			}
		}

		OnDeterministicTick.Broadcast(CurrentSimulationTick, FixedTimeStep);
	}
}

void UFootballDeterministicSimSubsystem::RegisterReplica(UObject* ReplicaObject)
{
	if (!ReplicaObject || !ReplicaObject->Implements<UFootballDeterministicReplica>())
	{
		return;
	}

	for (const TWeakObjectPtr<UObject>& Existing : RegisteredReplicas)
	{
		if (Existing.Get() == ReplicaObject)
		{
			return;
		}
	}
	RegisteredReplicas.Add(ReplicaObject);
}

void UFootballDeterministicSimSubsystem::UnregisterReplica(UObject* ReplicaObject)
{
	RegisteredReplicas.RemoveAll([ReplicaObject](const TWeakObjectPtr<UObject>& Entry)
	{
		return !Entry.IsValid() || Entry.Get() == ReplicaObject;
	});
}

void UFootballDeterministicSimSubsystem::ResetSimulation(int32 MatchSeed)
{
	CurrentSimulationTick = 0;
	AccumulatorSeconds = 0.0;
	MatchRNG.Seed(MatchSeed);

	for (FFootballDeterministicFrameSnapshot& Slot : SnapshotRingBuffer)
	{
		Slot = FFootballDeterministicFrameSnapshot();
	}
}

void UFootballDeterministicSimSubsystem::RecordSnapshot(const FFootballDeterministicBallState& BallState, int32 HomeScore, int32 AwayScore)
{
	if (SnapshotRingBuffer.Num() != SnapshotHistorySize)
	{
		SnapshotRingBuffer.SetNum(SnapshotHistorySize);
	}

	FFootballDeterministicFrameSnapshot Snapshot;
	Snapshot.SimulationTick = CurrentSimulationTick;
	Snapshot.BallState = BallState;
	Snapshot.BallState.SimulationTick = CurrentSimulationTick;
	Snapshot.HomeScore = HomeScore;
	Snapshot.AwayScore = AwayScore;

	uint32 CombinedHash = Snapshot.BallState.ComputeHash();
	CombinedHash = (CombinedHash ^ static_cast<uint32>(HomeScore)) * 16777619u;
	CombinedHash = (CombinedHash ^ static_cast<uint32>(AwayScore)) * 16777619u;
	for (const TWeakObjectPtr<UObject>& Entry : RegisteredReplicas)
	{
		if (UObject* Obj = Entry.Get())
		{
			if (Obj->Implements<UFootballDeterministicReplica>())
			{
				const int32 ReplicaHash = IFootballDeterministicReplica::Execute_GetDeterministicStateHash(Obj);
				CombinedHash = (CombinedHash ^ static_cast<uint32>(ReplicaHash)) * 16777619u;
			}
		}
	}
	Snapshot.StateChecksum = static_cast<int32>(CombinedHash);

	const int32 SlotIndex = CurrentSimulationTick % SnapshotHistorySize;
	SnapshotRingBuffer[SlotIndex] = Snapshot;

	OnSnapshotRecorded.Broadcast(Snapshot);
}

bool UFootballDeterministicSimSubsystem::GetSnapshotForTick(int32 TargetTick, FFootballDeterministicFrameSnapshot& OutSnapshot) const
{
	if (TargetTick <= 0 || SnapshotRingBuffer.Num() != SnapshotHistorySize)
	{
		return false;
	}

	const int32 SlotIndex = TargetTick % SnapshotHistorySize;
	const FFootballDeterministicFrameSnapshot& Candidate = SnapshotRingBuffer[SlotIndex];
	if (Candidate.SimulationTick == TargetTick)
	{
		OutSnapshot = Candidate;
		return true;
	}
	return false;
}

bool UFootballDeterministicSimSubsystem::RollbackToTick(int32 TargetTick)
{
	FFootballDeterministicFrameSnapshot Snapshot;
	if (!GetSnapshotForTick(TargetTick, Snapshot))
	{
		return false;
	}

	CurrentSimulationTick = TargetTick;
	for (const TWeakObjectPtr<UObject>& Entry : RegisteredReplicas)
	{
		if (UObject* Obj = Entry.Get())
		{
			if (Obj->Implements<UFootballDeterministicReplica>())
			{
				IFootballDeterministicReplica::Execute_ApplyDeterministicSnapshot(Obj, Snapshot);
			}
		}
	}

	UE_LOG(LogFootballNetworking, Verbose, TEXT("Rolled back deterministic simulation to tick %d (Checksum: %d)."), TargetTick, Snapshot.StateChecksum);
	return true;
}

float UFootballDeterministicSimSubsystem::NextDeterministicFloat(float MinVal, float MaxVal)
{
	return MatchRNG.NextFloatRange(MinVal, MaxVal);
}
