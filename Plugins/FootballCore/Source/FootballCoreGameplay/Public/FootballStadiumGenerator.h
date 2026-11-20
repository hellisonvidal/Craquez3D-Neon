// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FootballTypes.h"
#include "FootballStadiumGenerator.generated.h"

class UProceduralMeshComponent;
class UBoxComponent;
class USpotLightComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFootballGoalScoredSignature, EFootballTeamSide, ScoringTeam);

/**
 * Procedural Low-Poly FIFA Stadium Generator (105m x 68m) for Craquez.
 * Generates:
 * - Lush striped turf pitch with crisp white markings (touchlines, penalty boxes, center circle)
 * - Regulation goals (7.32m x 2.44m) with physical GoalPost & GoalNet collisions + Goal Trigger Volumes
 * - 4 low-poly tiered stands (North/South/East/West) + advertising perimeter boards
 * - 4 corner stadium floodlight towers (toggleable by Weather Manager)
 * - Clean bounds exported for UE5 PCG Graphs (Procedural Grass Blades & Spectator Crowd Instancing)
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREGAMEPLAY_API AFootballStadiumGenerator : public AActor
{
	GENERATED_BODY()

public:
	AFootballStadiumGenerator();

	virtual void BeginPlay() override;

	/** Build or rebuild the entire low-poly stadium geometry at runtime or in editor */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Football|Stadium")
	void BuildLowPolyStadium();

	/** Toggle stadium corner floodlights intensity (0 = Day, 1 = Night/Rain) */
	UFUNCTION(BlueprintCallable, Category = "Football|Stadium")
	void SetFloodlightsIntensity(float NormalizedIntensity);

	/** Pitch half-length in cm (FIFA standard: 5250 cm = 52.5 m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Stadium|Dimensions")
	float PitchHalfLengthCm = 5250.0f;

	/** Pitch half-width in cm (FIFA standard: 3400 cm = 34.0 m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Stadium|Dimensions")
	float PitchHalfWidthCm = 3400.0f;

	/** Regulation goal half-width: 366 cm (7.32m total) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Stadium|Dimensions")
	float GoalHalfWidthCm = 366.0f;

	/** Regulation goal height: 244 cm (2.44m) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Stadium|Dimensions")
	float GoalHeightCm = 244.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Stadium")
	TObjectPtr<UProceduralMeshComponent> PitchMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Stadium")
	TObjectPtr<UProceduralMeshComponent> StandsAndGoalsMesh;

	/** Trigger inside Home goal (-X): crossing this scores for Away team */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Stadium|Goals")
	TObjectPtr<UBoxComponent> HomeGoalTrigger;

	/** Trigger inside Away goal (+X): crossing this scores for Home team */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Stadium|Goals")
	TObjectPtr<UBoxComponent> AwayGoalTrigger;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Stadium|Lights")
	TArray<TObjectPtr<USpotLightComponent>> Floodlights;

	UPROPERTY(BlueprintAssignable, Category = "Football|Stadium")
	FOnFootballGoalScoredSignature OnGoalScored;

protected:
	UFUNCTION()
	void OnHomeGoalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnAwayGoalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	void BuildPitchAndMarkings();
	void BuildStandsAndGoals();
	void BuildGoalCollisions(float GoalX, float DepthSign, const FString& Prefix);

	float LastGoalTime = -10.0f;
};
