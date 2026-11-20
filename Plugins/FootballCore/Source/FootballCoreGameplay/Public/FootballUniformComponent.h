// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FootballTypes.h"
#include "FootballUniformComponent.generated.h"

class USkeletalMeshComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;

/**
 * Modular 3D Football Kit Component (NEVER painted onto skin!).
 * Attaches real separate 3D garment meshes (Jersey, Shorts, Socks, Boots, GK Gloves)
 * synchronized via LeaderPoseComponent to MetaHuman / Mixamo / UE5 Manny skeletons,
 * with distance-based Chaos Cloth LOD for 60 FPS on Android.
 * Also builds stylized low-poly separate 3D garment shells automatically when no external
 * SkeletalMesh asset is assigned yet so every player wears a real jersey & shorts immediately.
 */
UCLASS(ClassGroup = (Football), meta = (BlueprintSpawnableComponent))
class FOOTBALLCOREGAMEPLAY_API UFootballUniformComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFootballUniformComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Attach modular SkeletalMesh garments to the character's base skeleton (MetaHuman/Manny/Mixamo) */
	UFUNCTION(BlueprintCallable, Category = "Football|Uniform")
	void InitializeGarments(USkeletalMeshComponent* BaseBodyMesh, USceneComponent* FallbackRoot, bool bIsGoalkeeper);

	/** Apply team colors, squad number, and Chaos Cloth configuration dynamically */
	UFUNCTION(BlueprintCallable, Category = "Football|Uniform")
	void ApplyKitConfig(const FFootballKitConfig& NewConfig);

	/** Update cloth wetness darkening from weather manager [0..1] */
	UFUNCTION(BlueprintCallable, Category = "Football|Uniform")
	void SetFabricWetness(float WetnessNormalized);

	/** Force-toggle Chaos Cloth simulation (useful for Android battery/performance profiles) */
	UFUNCTION(BlueprintCallable, Category = "Football|Uniform")
	void SetChaosClothEnabled(bool bEnable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Uniform")
	FFootballKitConfig KitConfig;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Uniform|Garments")
	TObjectPtr<USkeletalMeshComponent> JerseySkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Uniform|Garments")
	TObjectPtr<USkeletalMeshComponent> ShortsSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Uniform|Garments")
	TObjectPtr<USkeletalMeshComponent> SocksSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Uniform|Garments")
	TObjectPtr<USkeletalMeshComponent> BootsSkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Uniform|Garments")
	TObjectPtr<USkeletalMeshComponent> GlovesSkeletalMesh;

	/** Separate procedural 3D kit shell (Torso Jersey, Shorts, Head/Limbs) active until external FBX/MetaHuman meshes are assigned */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Uniform|Fallback")
	TObjectPtr<UProceduralMeshComponent> ProceduralKitVisual;

protected:
	void BuildProceduralLayeredPlayerVisual(USceneComponent* AttachParent, bool bIsGoalkeeper);
	void UpdateClothLODFromCamera();
	void RefreshMaterialColors();

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> JerseyMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> ShortsMID;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> SocksMID;

	bool bClothCurrentlyActive = false;
	bool bIsGoalkeeperKit = false;
	float CurrentWetness = 0.0f;
	float ClothLODCheckTimer = 0.0f;
};
