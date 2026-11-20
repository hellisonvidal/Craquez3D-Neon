// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "FootballDeterministicTypes.generated.h"

/**
 * Fixed-point quantized 3D vector (millimeter precision: 1 unit = 0.1 cm).
 * Guarantees bit-identical state serialization and cross-platform determinism
 * between Android ARM64 and Desktop x64.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCORENETWORKING_API FFootballQuantizedVector
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 X = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 Y = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 Z = 0;

	static constexpr double ScaleFactor = 10.0; // 0.1 cm (1 mm) resolution

	static FFootballQuantizedVector FromFVector(const FVector& InVec)
	{
		FFootballQuantizedVector Out;
		Out.X = FMath::RoundToInt32(InVec.X * ScaleFactor);
		Out.Y = FMath::RoundToInt32(InVec.Y * ScaleFactor);
		Out.Z = FMath::RoundToInt32(InVec.Z * ScaleFactor);
		return Out;
	}

	FVector ToFVector() const
	{
		return FVector(
			static_cast<double>(X) / ScaleFactor,
			static_cast<double>(Y) / ScaleFactor,
			static_cast<double>(Z) / ScaleFactor
		);
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar.SerializeIntPacked(reinterpret_cast<uint32&>(X));
		Ar.SerializeIntPacked(reinterpret_cast<uint32&>(Y));
		Ar.SerializeIntPacked(reinterpret_cast<uint32&>(Z));
		bOutSuccess = !Ar.IsError();
		return true;
	}

	uint32 ComputeHash() const
	{
		uint32 Hash = 2166136261u;
		Hash = (Hash ^ static_cast<uint32>(X)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Y)) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(Z)) * 16777619u;
		return Hash;
	}
};

template<>
struct TStructOpsTypeTraits<FFootballQuantizedVector> : public TStructOpsTypeTraitsBase2<FFootballQuantizedVector>
{
	enum
	{
		WithNetSerializer = true,
	};
};

/**
 * Deterministic replicated snapshot of the football (position, linear velocity, Magnus angular spin).
 */
USTRUCT(BlueprintType)
struct FOOTBALLCORENETWORKING_API FFootballDeterministicBallState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 SimulationTick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	FFootballQuantizedVector Position;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	FFootballQuantizedVector LinearVelocity;

	/** Angular velocity in rad/s scaled via fixed-point (drives Magnus curve) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	FFootballQuantizedVector AngularVelocityRad;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	uint8 bIsGrounded = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 LastTouchPlayerId = -1;

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << SimulationTick;
		bool bPosOk = true, bVelOk = true, bAngOk = true;
		Position.NetSerialize(Ar, Map, bPosOk);
		LinearVelocity.NetSerialize(Ar, Map, bVelOk);
		AngularVelocityRad.NetSerialize(Ar, Map, bAngOk);
		Ar << bIsGrounded;
		Ar << LastTouchPlayerId;
		bOutSuccess = bPosOk && bVelOk && bAngOk && !Ar.IsError();
		return true;
	}

	uint32 ComputeHash() const
	{
		uint32 Hash = 2166136261u;
		Hash = (Hash ^ static_cast<uint32>(SimulationTick)) * 16777619u;
		Hash = (Hash ^ Position.ComputeHash()) * 16777619u;
		Hash = (Hash ^ LinearVelocity.ComputeHash()) * 16777619u;
		Hash = (Hash ^ AngularVelocityRad.ComputeHash()) * 16777619u;
		Hash = (Hash ^ static_cast<uint32>(bIsGrounded)) * 16777619u;
		return Hash;
	}
};

template<>
struct TStructOpsTypeTraits<FFootballDeterministicBallState> : public TStructOpsTypeTraitsBase2<FFootballDeterministicBallState>
{
	enum
	{
		WithNetSerializer = true,
	};
};

/**
 * Compact input packet for deterministic lockstep / rollback prediction.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCORENETWORKING_API FFootballDeterministicPlayerInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 SimulationTick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 PlayerSlotIndex = 0;

	/** Quantized stick input in [-1000, +1000] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int16 MoveX = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int16 MoveY = 0;

	/** Bitmask: bit0=Shoot, bit1=Pass, bit2=Lob, bit3=Sprint, bit4=Tackle, bit5=Switch */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	uint8 ActionFlags = 0;

	/** Quantized kick power [0..255] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	uint8 KickChargeByte = 0;

	/** Quantized Magnus curve input [-127..127] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int8 SpinCurveByte = 0;
};

/**
 * Full deterministic frame snapshot stored in the ring buffer for replay & rollback reconciliation.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCORENETWORKING_API FFootballDeterministicFrameSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 SimulationTick = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	FFootballDeterministicBallState BallState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 HomeScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 AwayScore = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 StateChecksum = 0;
};

/**
 * Cross-platform deterministic PRNG (Xorshift32) for consistent AI decisions and replays.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCORENETWORKING_API FFootballDeterministicRNG
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Networking")
	int32 State = 0x5A17C2D9;

	void Seed(int32 InSeed)
	{
		State = (InSeed == 0) ? 0x5A17C2D9 : InSeed;
	}

	uint32 NextUInt32()
	{
		uint32 X = static_cast<uint32>(State);
		X ^= X << 13;
		X ^= X >> 17;
		X ^= X << 5;
		State = static_cast<int32>(X);
		return X;
	}

	/** Returns deterministic float in [0.0, 1.0] */
	float NextFloat()
	{
		return static_cast<float>(NextUInt32() & 0x00FFFFFF) / static_cast<float>(0x00FFFFFF);
	}

	float NextFloatRange(float MinVal, float MaxVal)
	{
		return MinVal + (MaxVal - MinVal) * NextFloat();
	}
};

/**
 * Blueprint-friendly UInterface allowing Actors/Components (Ball, Players, Stadium, UI)
 * to step deterministically and support snapshot capture/rollback.
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UFootballDeterministicReplica : public UInterface
{
	GENERATED_BODY()
};

class FOOTBALLCORENETWORKING_API IFootballDeterministicReplica
{
	GENERATED_BODY()

public:
	/** Called at fixed deterministic timestep (e.g., 60 Hz) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|Deterministic")
	void OnDeterministicStep(int32 SimulationTick, float FixedDeltaTime);

	/** Capture deterministic state hash for desync checks */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|Deterministic")
	int32 GetDeterministicStateHash() const;

	/** Restore deterministic ball/actor state after rollback or replay seek */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Football|Deterministic")
	void ApplyDeterministicSnapshot(const FFootballDeterministicFrameSnapshot& Snapshot);
};
