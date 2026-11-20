// Copyright Craquez Studio. All Rights Reserved.

#include "FootballStadiumGenerator.h"
#include "FootballBallActor.h"
#include "ProceduralMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"

namespace
{
	void AddStadiumQuad(
		TArray<FVector>& Verts, TArray<int32>& Tris, TArray<FVector>& Normals,
		TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors,
		const FVector& A, const FVector& B, const FVector& C, const FVector& D,
		const FVector& Normal, const FLinearColor& Color)
	{
		const int32 Base = Verts.Num();
		Verts.Add(A); Verts.Add(B); Verts.Add(C); Verts.Add(D);
		Normals.Add(Normal); Normals.Add(Normal); Normals.Add(Normal); Normals.Add(Normal);
		UVs.Add(FVector2D(0, 0)); UVs.Add(FVector2D(1, 0)); UVs.Add(FVector2D(1, 1)); UVs.Add(FVector2D(0, 1));
		Colors.Add(Color); Colors.Add(Color); Colors.Add(Color); Colors.Add(Color);

		Tris.Add(Base + 0); Tris.Add(Base + 1); Tris.Add(Base + 2);
		Tris.Add(Base + 0); Tris.Add(Base + 2); Tris.Add(Base + 3);
	}

	void AddStadiumBox(
		TArray<FVector>& Verts, TArray<int32>& Tris, TArray<FVector>& Normals,
		TArray<FVector2D>& UVs, TArray<FLinearColor>& Colors,
		const FVector& Center, const FVector& HalfExtent, const FLinearColor& Color)
	{
		const FVector Signs[8] = {
			{-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
			{-1,-1, 1}, {1,-1, 1}, {1,1, 1}, {-1,1, 1}
		};
		const int32 Faces[6][4] = {
			{0,1,2,3}, {4,7,6,5}, {0,4,5,1}, {2,6,7,3}, {1,5,6,2}, {3,7,4,0}
		};
		const FVector FNorms[6] = {
			{0,0,-1}, {0,0,1}, {0,-1,0}, {0,1,0}, {1,0,0}, {-1,0,0}
		};

		for (int32 f = 0; f < 6; ++f)
		{
			const int32 Base = Verts.Num();
			for (int32 c = 0; c < 4; ++c)
			{
				const FVector S = Signs[Faces[f][c]];
				Verts.Add(Center + FVector(S.X * HalfExtent.X, S.Y * HalfExtent.Y, S.Z * HalfExtent.Z));
				Normals.Add(FNorms[f]);
				UVs.Add(FVector2D((c == 1 || c == 2) ? 1.f : 0.f, (c >= 2) ? 1.f : 0.f));
				Colors.Add(Color);
			}
			Tris.Add(Base + 0); Tris.Add(Base + 1); Tris.Add(Base + 2);
			Tris.Add(Base + 0); Tris.Add(Base + 2); Tris.Add(Base + 3);
		}
	}
}

AFootballStadiumGenerator::AFootballStadiumGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	PitchMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("PitchMesh"));
	PitchMesh->bUseComplexAsSimpleCollision = true;
	PitchMesh->SetCollisionProfileName(TEXT("BlockAll"));
	RootComponent = PitchMesh;

	StandsAndGoalsMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StandsAndGoalsMesh"));
	StandsAndGoalsMesh->SetupAttachment(RootComponent);
	StandsAndGoalsMesh->SetCollisionProfileName(TEXT("BlockAll"));

	HomeGoalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("HomeGoalTrigger"));
	HomeGoalTrigger->SetupAttachment(RootComponent);
	HomeGoalTrigger->SetBoxExtent(FVector(100.0f, GoalHalfWidthCm - 12.0f, GoalHeightCm * 0.5f));
	HomeGoalTrigger->SetRelativeLocation(FVector(-PitchHalfLengthCm - 110.0f, 0.0f, GoalHeightCm * 0.5f));
	HomeGoalTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	AwayGoalTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("AwayGoalTrigger"));
	AwayGoalTrigger->SetupAttachment(RootComponent);
	AwayGoalTrigger->SetBoxExtent(FVector(100.0f, GoalHalfWidthCm - 12.0f, GoalHeightCm * 0.5f));
	AwayGoalTrigger->SetRelativeLocation(FVector(PitchHalfLengthCm + 110.0f, 0.0f, GoalHeightCm * 0.5f));
	AwayGoalTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	// 4 Corner Stadium Floodlights
	const FVector TowerCoords[4] = {
		{-PitchHalfLengthCm - 1400.0f, -PitchHalfWidthCm - 1200.0f, 3400.0f},
		{ PitchHalfLengthCm + 1400.0f, -PitchHalfWidthCm - 1200.0f, 3400.0f},
		{-PitchHalfLengthCm - 1400.0f,  PitchHalfWidthCm + 1200.0f, 3400.0f},
		{ PitchHalfLengthCm + 1400.0f,  PitchHalfWidthCm + 1200.0f, 3400.0f}
	};

	for (int32 i = 0; i < 4; ++i)
	{
		USpotLightComponent* Light = CreateDefaultSubobject<USpotLightComponent>(*FString::Printf(TEXT("Floodlight_%d"), i));
		Light->SetupAttachment(RootComponent);
		Light->SetRelativeLocation(TowerCoords[i]);
		Light->SetRelativeRotation((FVector::ZeroVector - TowerCoords[i]).Rotation());
		Light->Intensity = 85000.0f;
		Light->AttenuationRadius = 15000.0f;
		Light->OuterConeAngle = 72.0f;
		Light->InnerConeAngle = 45.0f;
		Light->LightColor = FColor(235, 245, 255);
		Floodlights.Add(Light);
	}
}

void AFootballStadiumGenerator::BeginPlay()
{
	Super::BeginPlay();

	HomeGoalTrigger->OnComponentBeginOverlap.AddDynamic(this, &AFootballStadiumGenerator::OnHomeGoalOverlap);
	AwayGoalTrigger->OnComponentBeginOverlap.AddDynamic(this, &AFootballStadiumGenerator::OnAwayGoalOverlap);

	BuildLowPolyStadium();
}

void AFootballStadiumGenerator::BuildLowPolyStadium()
{
	BuildPitchAndMarkings();
	BuildStandsAndGoals();
	BuildGoalCollisions(-PitchHalfLengthCm, -1.0f, TEXT("Home"));
	BuildGoalCollisions(PitchHalfLengthCm, 1.0f, TEXT("Away"));
}

void AFootballStadiumGenerator::BuildPitchAndMarkings()
{
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	// Outer stadium apron (deep emerald runoff track)
	const float OuterX = PitchHalfLengthCm + 1600.0f;
	const float OuterY = PitchHalfWidthCm + 1400.0f;
	AddStadiumQuad(Verts, Tris, Normals, UVs, Colors,
		FVector(-OuterX, -OuterY, -2.0f), FVector(OuterX, -OuterY, -2.0f),
		FVector(OuterX, OuterY, -2.0f), FVector(-OuterX, OuterY, -2.0f),
		FVector::UpVector, FLinearColor(0.05f, 0.16f, 0.08f, 1.0f));

	// Mowed alternating grass stripes (18 stripes across pitch length)
	const int32 NumStripes = 18;
	const float StripeLen = (PitchHalfLengthCm * 2.0f) / static_cast<float>(NumStripes);
	for (int32 i = 0; i < NumStripes; ++i)
	{
		const float X0 = -PitchHalfLengthCm + i * StripeLen;
		const float X1 = X0 + StripeLen;
		const FLinearColor TurfColor = (i % 2 == 0)
			? FLinearColor(0.08f, 0.36f, 0.11f, 1.0f)
			: FLinearColor(0.11f, 0.44f, 0.14f, 1.0f);

		AddStadiumQuad(Verts, Tris, Normals, UVs, Colors,
			FVector(X0, -PitchHalfWidthCm, 0.0f), FVector(X1, -PitchHalfWidthCm, 0.0f),
			FVector(X1, PitchHalfWidthCm, 0.0f), FVector(X0, PitchHalfWidthCm, 0.0f),
			FVector::UpVector, TurfColor);
	}

	// Crisp white chalk pitch lines (Z = +1.5 cm to avoid Z-fighting)
	const FLinearColor LineWhite(0.96f, 0.98f, 0.96f, 1.0f);
	const float LW = 14.0f; // 14cm line width
	const float LZ = 1.5f;

	auto AddLine = [&](float X0, float Y0, float X1, float Y1)
	{
		AddStadiumQuad(Verts, Tris, Normals, UVs, Colors,
			FVector(X0, Y0, LZ), FVector(X1, Y0, LZ),
			FVector(X1, Y1, LZ), FVector(X0, Y1, LZ),
			FVector::UpVector, LineWhite);
	};

	// Outer boundary lines + halfway line
	AddLine(-PitchHalfLengthCm, -PitchHalfWidthCm, PitchHalfLengthCm, -PitchHalfWidthCm + LW);
	AddLine(-PitchHalfLengthCm, PitchHalfWidthCm - LW, PitchHalfLengthCm, PitchHalfWidthCm);
	AddLine(-PitchHalfLengthCm, -PitchHalfWidthCm, -PitchHalfLengthCm + LW, PitchHalfWidthCm);
	AddLine(PitchHalfLengthCm - LW, -PitchHalfWidthCm, PitchHalfLengthCm, PitchHalfWidthCm);
	AddLine(-LW * 0.5f, -PitchHalfWidthCm, LW * 0.5f, PitchHalfWidthCm);

	// Penalty boxes (16.5m deep x 40.32m wide)
	const float PenDepth = 1650.0f;
	const float PenHalfW = 2016.0f;
	// Home penalty box (-X)
	AddLine(-PitchHalfLengthCm + PenDepth, -PenHalfW, -PitchHalfLengthCm + PenDepth + LW, PenHalfW);
	AddLine(-PitchHalfLengthCm, -PenHalfW, -PitchHalfLengthCm + PenDepth, -PenHalfW + LW);
	AddLine(-PitchHalfLengthCm, PenHalfW - LW, -PitchHalfLengthCm + PenDepth, PenHalfW);
	// Away penalty box (+X)
	AddLine(PitchHalfLengthCm - PenDepth - LW, -PenHalfW, PitchHalfLengthCm - PenDepth, PenHalfW);
	AddLine(PitchHalfLengthCm - PenDepth, -PenHalfW, PitchHalfLengthCm, -PenHalfW + LW);
	AddLine(PitchHalfLengthCm - PenDepth, PenHalfW - LW, PitchHalfLengthCm, PenHalfW);

	// Center circle (9.15m radius = 915 cm)
	const int32 CircleSegs = 36;
	const float CircleR = 915.0f;
	for (int32 s = 0; s < CircleSegs; ++s)
	{
		const float A0 = (static_cast<float>(s) / CircleSegs) * 2.0f * PI;
		const float A1 = (static_cast<float>(s + 1) / CircleSegs) * 2.0f * PI;
		const FVector Inner0(FMath::Cos(A0) * (CircleR - LW), FMath::Sin(A0) * (CircleR - LW), LZ);
		const FVector Outer0(FMath::Cos(A0) * CircleR, FMath::Sin(A0) * CircleR, LZ);
		const FVector Outer1(FMath::Cos(A1) * CircleR, FMath::Sin(A1) * CircleR, LZ);
		const FVector Inner1(FMath::Cos(A1) * (CircleR - LW), FMath::Sin(A1) * (CircleR - LW), LZ);
		AddStadiumQuad(Verts, Tris, Normals, UVs, Colors, Inner0, Outer0, Outer1, Inner1, FVector::UpVector, LineWhite);
	}

	PitchMesh->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UVs, Colors, Tangents, true);
}

void AFootballStadiumGenerator::BuildStandsAndGoals()
{
	TArray<FVector> Verts;
	TArray<int32> Tris;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const FLinearColor StandConcrete(0.14f, 0.16f, 0.22f, 1.0f);
	const FLinearColor SeatNeonEmerald(0.03f, 0.58f, 0.34f, 1.0f);
	const FLinearColor SeatNeonCyan(0.04f, 0.72f, 0.85f, 1.0f);
	const FLinearColor AdBoardNeon(0.02f, 0.88f, 0.65f, 1.0f);
	const FLinearColor PostWhite(0.98f, 0.98f, 1.0f, 1.0f);
	const FLinearColor NetTint(0.82f, 0.86f, 0.90f, 1.0f);

	// 1. Low-Poly Tiered Stands on all 4 sides (6 stepped tiers per stand)
	const int32 NumTiers = 6;
	for (int32 t = 0; t < NumTiers; ++t)
	{
		const float Offset = 700.0f + t * 240.0f;
		const float Height = 120.0f + t * 145.0f;
		const FLinearColor TierColor = (t % 2 == 0) ? SeatNeonEmerald : StandConcrete;

		// North & South sideline stands
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors,
			FVector(0.0f, -PitchHalfWidthCm - Offset, Height * 0.5f),
			FVector(PitchHalfLengthCm + 400.0f, 115.0f, Height * 0.5f), TierColor);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors,
			FVector(0.0f, PitchHalfWidthCm + Offset, Height * 0.5f),
			FVector(PitchHalfLengthCm + 400.0f, 115.0f, Height * 0.5f), TierColor);

		// East & West behind-goal stands
		const FLinearColor EndColor = (t % 2 == 0) ? SeatNeonCyan : StandConcrete;
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors,
			FVector(-PitchHalfLengthCm - Offset, 0.0f, Height * 0.5f),
			FVector(115.0f, PitchHalfWidthCm + 250.0f, Height * 0.5f), EndColor);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors,
			FVector(PitchHalfLengthCm + Offset, 0.0f, Height * 0.5f),
			FVector(115.0f, PitchHalfWidthCm + 250.0f, Height * 0.5f), EndColor);
	}

	// 2. Perimeter LED Advertising Boards (Craquez Neon aesthetic!)
	AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(0.0f, -PitchHalfWidthCm - 420.0f, 55.0f), FVector(PitchHalfLengthCm, 16.0f, 55.0f), AdBoardNeon);
	AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(0.0f,  PitchHalfWidthCm + 420.0f, 55.0f), FVector(PitchHalfLengthCm, 16.0f, 55.0f), AdBoardNeon);

	// 3. 4 Corner Floodlight Towers
	const FVector TowerBases[4] = {
		{-PitchHalfLengthCm - 1400.0f, -PitchHalfWidthCm - 1200.0f, 0.0f},
		{ PitchHalfLengthCm + 1400.0f, -PitchHalfWidthCm - 1200.0f, 0.0f},
		{-PitchHalfLengthCm - 1400.0f,  PitchHalfWidthCm + 1200.0f, 0.0f},
		{ PitchHalfLengthCm + 1400.0f,  PitchHalfWidthCm + 1200.0f, 0.0f}
	};
	for (const FVector& Base : TowerBases)
	{
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, Base + FVector(0, 0, 1700), FVector(65, 65, 1700), StandConcrete);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, Base + FVector(0, 0, 3420), FVector(210, 210, 90), PostWhite);
	}

	// 4. Both Goals (Posts, Crossbar & Net Frame)
	auto AddGoalVisual = [&](float GoalX, float DepthSign)
	{
		const float PostR = 7.0f;
		const float NetDepth = 220.0f;
		// Left & Right Uprights
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX, -GoalHalfWidthCm, GoalHeightCm * 0.5f), FVector(PostR, PostR, GoalHeightCm * 0.5f), PostWhite);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX,  GoalHalfWidthCm, GoalHeightCm * 0.5f), FVector(PostR, PostR, GoalHeightCm * 0.5f), PostWhite);
		// Crossbar
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX, 0.0f, GoalHeightCm), FVector(PostR, GoalHalfWidthCm + PostR, PostR), PostWhite);
		// Back & Side Net Panels
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX + DepthSign * NetDepth, 0.0f, GoalHeightCm * 0.5f), FVector(4.0f, GoalHalfWidthCm, GoalHeightCm * 0.5f), NetTint);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX + DepthSign * NetDepth * 0.5f, -GoalHalfWidthCm, GoalHeightCm * 0.5f), FVector(NetDepth * 0.5f, 4.0f, GoalHeightCm * 0.5f), NetTint);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX + DepthSign * NetDepth * 0.5f,  GoalHalfWidthCm, GoalHeightCm * 0.5f), FVector(NetDepth * 0.5f, 4.0f, GoalHeightCm * 0.5f), NetTint);
		AddStadiumBox(Verts, Tris, Normals, UVs, Colors, FVector(GoalX + DepthSign * NetDepth * 0.5f, 0.0f, GoalHeightCm), FVector(NetDepth * 0.5f, GoalHalfWidthCm, 4.0f), NetTint);
	};

	AddGoalVisual(-PitchHalfLengthCm, -1.0f);
	AddGoalVisual(PitchHalfLengthCm, 1.0f);

	StandsAndGoalsMesh->CreateMeshSection_LinearColor(0, Verts, Tris, Normals, UVs, Colors, Tangents, false);
}

void AFootballStadiumGenerator::BuildGoalCollisions(float GoalX, float DepthSign, const FString& Prefix)
{
	auto AddTaggedBox = [&](const FString& Name, const FVector& Loc, const FVector& Extent, const FName& SurfaceTag)
	{
		UBoxComponent* Box = NewObject<UBoxComponent>(this, *Name);
		if (Box)
		{
			Box->RegisterComponent();
			Box->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
			Box->SetRelativeLocation(Loc);
			Box->SetBoxExtent(Extent);
			Box->SetCollisionProfileName(TEXT("BlockAll"));
			Box->ComponentTags.Add(SurfaceTag);
		}
	};

	const float NetDepth = 220.0f;
	AddTaggedBox(Prefix + TEXT("_LeftPostCol"), FVector(GoalX, -GoalHalfWidthCm, GoalHeightCm * 0.5f), FVector(9, 9, GoalHeightCm * 0.5f), TEXT("GoalPost"));
	AddTaggedBox(Prefix + TEXT("_RightPostCol"), FVector(GoalX, GoalHalfWidthCm, GoalHeightCm * 0.5f), FVector(9, 9, GoalHeightCm * 0.5f), TEXT("GoalPost"));
	AddTaggedBox(Prefix + TEXT("_CrossbarCol"), FVector(GoalX, 0.0f, GoalHeightCm), FVector(9, GoalHalfWidthCm, 9), TEXT("GoalPost"));
	AddTaggedBox(Prefix + TEXT("_BackNetCol"), FVector(GoalX + DepthSign * NetDepth, 0.0f, GoalHeightCm * 0.5f), FVector(12, GoalHalfWidthCm, GoalHeightCm * 0.5f), TEXT("GoalNet"));
}

void AFootballStadiumGenerator::SetFloodlightsIntensity(float NormalizedIntensity)
{
	const float Clamped = FMath::Clamp(NormalizedIntensity, 0.0f, 1.0f);
	for (USpotLightComponent* Light : Floodlights)
	{
		if (Light)
		{
			Light->SetIntensity(Clamped * 95000.0f);
			Light->SetVisibility(Clamped > 0.05f);
		}
	}
}

void AFootballStadiumGenerator::OnHomeGoalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<AFootballBallActor>(OtherActor) || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastGoalTime > 3.0f)
	{
		LastGoalTime = Now;
		OnGoalScored.Broadcast(EFootballTeamSide::Away);
	}
}

void AFootballStadiumGenerator::OnAwayGoalOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!Cast<AFootballBallActor>(OtherActor) || !GetWorld())
	{
		return;
	}
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now - LastGoalTime > 3.0f)
	{
		LastGoalTime = Now;
		OnGoalScored.Broadcast(EFootballTeamSide::Home);
	}
}
