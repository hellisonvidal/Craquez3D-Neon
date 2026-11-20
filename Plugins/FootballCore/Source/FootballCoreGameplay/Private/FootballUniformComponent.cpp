// Copyright Craquez Studio. All Rights Reserved.

#include "FootballUniformComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Engine/World.h"

namespace
{
	void AppendBoxSection(
		TArray<FVector>& Verts,
		TArray<int32>& Tris,
		TArray<FVector>& Normals,
		TArray<FVector2D>& UVs,
		TArray<FLinearColor>& Colors,
		const FVector& Center,
		const FVector& HalfExtent,
		const FLinearColor& Color)
	{
		const int32 BaseIdx = Verts.Num();
		const FVector Signs[8] = {
			{-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
			{-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1}
		};

		const int32 FaceIndices[6][4] = {
			{0, 1, 2, 3}, // Bottom
			{4, 7, 6, 5}, // Top
			{0, 4, 5, 1}, // Front-Y
			{2, 6, 7, 3}, // Back+Y
			{1, 5, 6, 2}, // Right+X
			{3, 7, 4, 0}  // Left-X
		};

		const FVector FaceNormals[6] = {
			{0,0,-1}, {0,0,1}, {0,-1,0}, {0,1,0}, {1,0,0}, {-1,0,0}
		};

		for (int32 f = 0; f < 6; ++f)
		{
			const int32 FBase = Verts.Num();
			for (int32 c = 0; c < 4; ++c)
			{
				const FVector S = Signs[FaceIndices[f][c]];
				Verts.Add(Center + FVector(S.X * HalfExtent.X, S.Y * HalfExtent.Y, S.Z * HalfExtent.Z));
				Normals.Add(FaceNormals[f]);
				UVs.Add(FVector2D((c == 1 || c == 2) ? 1.0f : 0.0f, (c >= 2) ? 1.0f : 0.0f));
				Colors.Add(Color);
			}
			Tris.Add(FBase + 0); Tris.Add(FBase + 1); Tris.Add(FBase + 2);
			Tris.Add(FBase + 0); Tris.Add(FBase + 2); Tris.Add(FBase + 3);
		}
		(void)BaseIdx;
	}
}

UFootballUniformComponent::UFootballUniformComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.2f; // Efficient mobile cloth LOD check (5 Hz)
}

void UFootballUniformComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UFootballUniformComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	ClothLODCheckTimer += DeltaTime;
	if (ClothLODCheckTimer >= 0.2f)
	{
		ClothLODCheckTimer = 0.0f;
		UpdateClothLODFromCamera();
	}
}

void UFootballUniformComponent::InitializeGarments(USkeletalMeshComponent* BaseBodyMesh, USceneComponent* FallbackRoot, bool bIsGoalkeeper)
{
	bIsGoalkeeperKit = bIsGoalkeeper;
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	auto CreateGarmentSlot = [Owner, BaseBodyMesh](const FName& SlotName) -> USkeletalMeshComponent*
	{
		USkeletalMeshComponent* Comp = NewObject<USkeletalMeshComponent>(Owner, SlotName);
		if (Comp)
		{
			Comp->RegisterComponent();
			if (BaseBodyMesh)
			{
				Comp->AttachToComponent(BaseBodyMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
				Comp->SetLeaderPoseComponent(BaseBodyMesh, true);
			}
			Comp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Comp->bEnableUpdateRateOptimizations = true;
		}
		return Comp;
	};

	if (BaseBodyMesh && BaseBodyMesh->GetSkeletalMeshAsset() != nullptr)
	{
		JerseySkeletalMesh = CreateGarmentSlot(TEXT("Garment_Jersey"));
		ShortsSkeletalMesh = CreateGarmentSlot(TEXT("Garment_Shorts"));
		SocksSkeletalMesh = CreateGarmentSlot(TEXT("Garment_Socks"));
		BootsSkeletalMesh = CreateGarmentSlot(TEXT("Garment_Boots"));
		if (bIsGoalkeeper)
		{
			GlovesSkeletalMesh = CreateGarmentSlot(TEXT("Garment_GKGloves"));
		}

		// Enable Chaos Cloth on Jersey by default if configured
		SetChaosClothEnabled(KitConfig.bEnableChaosCloth);
	}
	else if (FallbackRoot)
	{
		// Build distinct layered 3D geometry (body + separate jersey shell + shorts + socks + boots)
		BuildProceduralLayeredPlayerVisual(FallbackRoot, bIsGoalkeeper);
	}

	RefreshMaterialColors();
}

void UFootballUniformComponent::BuildProceduralLayeredPlayerVisual(USceneComponent* AttachParent, bool bIsGoalkeeper)
{
	AActor* Owner = GetOwner();
	if (!Owner || !AttachParent)
	{
		return;
	}

	if (!ProceduralKitVisual)
	{
		ProceduralKitVisual = NewObject<UProceduralMeshComponent>(Owner, TEXT("ProceduralKitVisual"));
		ProceduralKitVisual->RegisterComponent();
		ProceduralKitVisual->AttachToComponent(AttachParent, FAttachmentTransformRules::KeepRelativeTransform);
		ProceduralKitVisual->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f)); // Ground feet at capsule bottom
		ProceduralKitVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const FLinearColor SkinColor(0.76f, 0.56f, 0.42f, 1.0f);
	const FLinearColor HairColor(0.08f, 0.06f, 0.05f, 1.0f);
	const FLinearColor GloveColor(0.95f, 0.82f, 0.08f, 1.0f);

	// 1. Skin Head + Hair (Subsurface scattering slot in final materials)
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, 0, 162), FVector(11, 10, 12), SkinColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(-1, 0, 173), FVector(11.5f, 10.5f, 4), HairColor);

	// 2. Separate Physical 3D Jersey Shell (slightly offset outside torso + sleeves + collar accent)
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, 0, 122), FVector(13, 21, 25), KitConfig.JerseyPrimaryColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, 0, 145), FVector(13.5f, 21.5f, 3), KitConfig.JerseySecondaryColor);
	// Left & Right Jersey Sleeves
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, -27, 132), FVector(8, 7, 12), KitConfig.JerseyPrimaryColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0,  27, 132), FVector(8, 7, 12), KitConfig.JerseyPrimaryColor);

	// Forearms (Skin) & GK Gloves
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(2, -27, 108), FVector(5.5f, 5.5f, 13), bIsGoalkeeper ? KitConfig.JerseyPrimaryColor : SkinColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(2,  27, 108), FVector(5.5f, 5.5f, 13), bIsGoalkeeper ? KitConfig.JerseyPrimaryColor : SkinColor);
	if (bIsGoalkeeper)
	{
		AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(4, -27, 92), FVector(7, 7, 7), GloveColor);
		AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(4,  27, 92), FVector(7, 7, 7), GloveColor);
	}

	// 3. Separate 3D Shorts (Hip + Thigh loose fabric shell)
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, 0, 88), FVector(13, 19, 11), KitConfig.ShortsColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, -10, 68), FVector(9.5f, 9.5f, 13), KitConfig.ShortsColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0,  10, 68), FVector(9.5f, 9.5f, 13), KitConfig.ShortsColor);

	// Knees (Skin)
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, -10, 50), FVector(6.5f, 6.5f, 6), SkinColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0,  10, 50), FVector(6.5f, 6.5f, 6), SkinColor);

	// 4. Separate 3D Football Socks
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0, -10, 28), FVector(6.8f, 6.8f, 17), KitConfig.SocksColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(0,  10, 28), FVector(6.8f, 6.8f, 17), KitConfig.SocksColor);

	// 5. Separate 3D Boots / Cleats
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(5, -10, 6), FVector(12, 6.5f, 5.5f), KitConfig.BootsColor);
	AppendBoxSection(Verts, Tris, Normals, UVs, Colors, FVector(5,  10, 6), FVector(12, 6.5f, 5.5f), KitConfig.BootsColor);

	ProceduralKitVisual->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);
}

void UFootballUniformComponent::ApplyKitConfig(const FFootballKitConfig& NewConfig)
{
	KitConfig = NewConfig;
	SetChaosClothEnabled(KitConfig.bEnableChaosCloth);
	RefreshMaterialColors();

	if (ProceduralKitVisual && GetOwner())
	{
		BuildProceduralLayeredPlayerVisual(GetOwner()->GetRootComponent(), bIsGoalkeeperKit);
	}
}

void UFootballUniformComponent::SetFabricWetness(float WetnessNormalized)
{
	CurrentWetness = FMath::Clamp(WetnessNormalized, 0.0f, 1.0f);
	if (JerseyMID)
	{
		JerseyMID->SetScalarParameterValue(TEXT("Wetness"), CurrentWetness);
	}
	if (ShortsMID)
	{
		ShortsMID->SetScalarParameterValue(TEXT("Wetness"), CurrentWetness);
	}
}

void UFootballUniformComponent::SetChaosClothEnabled(bool bEnable)
{
	bClothCurrentlyActive = bEnable;
	if (JerseySkeletalMesh)
	{
		JerseySkeletalMesh->bDisableClothSimulation = !bEnable;
	}
	if (ShortsSkeletalMesh)
	{
		ShortsSkeletalMesh->bDisableClothSimulation = !bEnable;
	}
}

void UFootballUniformComponent::UpdateClothLODFromCamera()
{
	if (!KitConfig.bEnableChaosCloth || !JerseySkeletalMesh || !GetWorld())
	{
		return;
	}

	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (!PC || !PC->PlayerCameraManager || !GetOwner())
	{
		return;
	}

	const float DistToCam = FVector::Dist(PC->PlayerCameraManager->GetCameraLocation(), GetOwner()->GetActorLocation());
	const bool bShouldSimCloth = DistToCam <= KitConfig.ClothLODDisableDistanceCm;
	if (bShouldSimCloth != bClothCurrentlyActive)
	{
		SetChaosClothEnabled(bShouldSimCloth);
	}
}

void UFootballUniformComponent::RefreshMaterialColors()
{
	auto ApplyMID = [](USkeletalMeshComponent* Comp, TObjectPtr<UMaterialInstanceDynamic>& OutMID, const FLinearColor& Primary, const FLinearColor& Secondary, int32 Number)
	{
		if (!Comp || Comp->GetNumMaterials() == 0)
		{
			return;
		}
		if (!OutMID)
		{
			OutMID = Comp->CreateAndSetMaterialInstanceDynamic(0);
		}
		if (OutMID)
		{
			OutMID->SetVectorParameterValue(TEXT("PrimaryColor"), Primary);
			OutMID->SetVectorParameterValue(TEXT("SecondaryColor"), Secondary);
			OutMID->SetScalarParameterValue(TEXT("SquadNumber"), static_cast<float>(Number));
		}
	};

	ApplyMID(JerseySkeletalMesh, JerseyMID, KitConfig.JerseyPrimaryColor, KitConfig.JerseySecondaryColor, KitConfig.SquadNumber);
	ApplyMID(ShortsSkeletalMesh, ShortsMID, KitConfig.ShortsColor, KitConfig.JerseySecondaryColor, KitConfig.SquadNumber);
	ApplyMID(SocksSkeletalMesh, SocksMID, KitConfig.SocksColor, KitConfig.JerseySecondaryColor, KitConfig.SquadNumber);
}
