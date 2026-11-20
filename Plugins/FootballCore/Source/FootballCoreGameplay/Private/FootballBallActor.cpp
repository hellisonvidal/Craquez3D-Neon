// Copyright Craquez Studio. All Rights Reserved.

#include "FootballBallActor.h"
#include "FootballCharacter.h"
#include "FootballDeterministicSimSubsystem.h"
#include "FootballCoreGameplayModule.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "NiagaraComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/World.h"

AFootballBallActor::AFootballBallActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;
	bReplicates = true;
	SetReplicateMovement(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("BallCollisionSphere"));
	CollisionSphere->InitSphereRadius(AeroConfig.RadiusCm);
	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetEnableGravity(true);
	CollisionSphere->SetCollisionProfileName(TEXT("PhysicsActor"));
	CollisionSphere->SetNotifyRigidBodyCollision(true);
	CollisionSphere->BodyInstance.bUseCCD = true; // Continuous Collision Detection prevents high-speed tunneling
	CollisionSphere->SetMassOverrideInKg(NAME_None, AeroConfig.MassKg, true);
	CollisionSphere->SetLinearDamping(0.02f);  // Aerodynamic drag is handled explicitly by quadratic formula
	CollisionSphere->SetAngularDamping(0.05f); // Spin decay handled explicitly
	RootComponent = CollisionSphere;

	ProceduralBallMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ProceduralBallMesh"));
	ProceduralBallMesh->SetupAttachment(CollisionSphere);
	ProceduralBallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ProceduralBallMesh->bUseAsyncCooking = false;

	CustomBallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CustomBallMesh"));
	CustomBallMesh->SetupAttachment(CollisionSphere);
	CustomBallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CustomBallMesh->SetVisibility(false);

	MagnusTrailNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MagnusTrailNiagara"));
	MagnusTrailNiagara->SetupAttachment(CollisionSphere);
	MagnusTrailNiagara->bAutoActivate = true;
}

void AFootballBallActor::BeginPlay()
{
	Super::BeginPlay();

	CollisionSphere->SetSphereRadius(AeroConfig.RadiusCm);
	CollisionSphere->SetMassOverrideInKg(NAME_None, AeroConfig.MassKg, true);
	CollisionSphere->OnComponentHit.AddDynamic(this, &AFootballBallActor::HandleBallHit);

	if (CustomBallMesh && CustomBallMesh->GetStaticMesh() != nullptr)
	{
		CustomBallMesh->SetVisibility(true);
		ProceduralBallMesh->SetVisibility(false);
	}
	else
	{
		BuildProceduralFootballMesh();
	}

	if (UWorld* World = GetWorld())
	{
		if (UFootballDeterministicSimSubsystem* SimSub = World->GetSubsystem<UFootballDeterministicSimSubsystem>())
		{
			SimSub->RegisterReplica(this);
		}
	}
}

void AFootballBallActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UFootballDeterministicSimSubsystem* SimSub = World->GetSubsystem<UFootballDeterministicSimSubsystem>())
		{
			SimSub->UnregisterReplica(this);
		}
	}
	Super::EndPlay(EndPlayReason);
}

void AFootballBallActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	TimeSinceLastKick += DeltaTime;
	ApplyAerodynamicForces(DeltaTime);
	UpdateDeformationSpring(DeltaTime);

	// Update Niagara user parameters for visual Magnus airflow trail
	if (MagnusTrailNiagara && MagnusTrailNiagara->GetAsset())
	{
		const float SpeedRatio = FMath::Clamp(GetBallVelocity().Size() / 3200.0f, 0.0f, 1.0f);
		const float SpinMagnitude = GetBallSpinRadPerSec().Size();
		MagnusTrailNiagara->SetFloatParameter(TEXT("BallSpeedNorm"), SpeedRatio);
		MagnusTrailNiagara->SetFloatParameter(TEXT("BallSpinRad"), SpinMagnitude);
	}
}

void AFootballBallActor::BuildProceduralFootballMesh()
{
	// Generate a clean UV sphere with classic soccer pentagon/hexagon alternating vertex colors
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> Colors;
	TArray<FProcMeshTangent> Tangents;

	const int32 LatSegments = 16;
	const int32 LonSegments = 24;
	const float Radius = AeroConfig.RadiusCm;

	for (int32 Lat = 0; Lat <= LatSegments; ++Lat)
	{
		const float Theta = static_cast<float>(Lat) * PI / static_cast<float>(LatSegments);
		const float SinTheta = FMath::Sin(Theta);
		const float CosTheta = FMath::Cos(Theta);

		for (int32 Lon = 0; Lon <= LonSegments; ++Lon)
		{
			const float Phi = static_cast<float>(Lon) * 2.0f * PI / static_cast<float>(LonSegments);
			const float SinPhi = FMath::Sin(Phi);
			const float CosPhi = FMath::Cos(Phi);

			const FVector Normal(CosPhi * SinTheta, SinPhi * SinTheta, CosTheta);
			Vertices.Add(Normal * Radius);
			Normals.Add(Normal);
			UVs.Add(FVector2D(static_cast<float>(Lon) / LonSegments, static_cast<float>(Lat) / LatSegments));

			// Stylized Craquez Neon-Football panel pattern (white + dark carbon + cyan seam accents)
			const int32 Patch = ((Lat / 3) + (Lon / 4)) % 2;
			Colors.Add(Patch == 0 ? FLinearColor(0.96f, 0.98f, 1.0f, 1.0f) : FLinearColor(0.06f, 0.08f, 0.14f, 1.0f));
			Tangents.Add(FProcMeshTangent(-SinPhi, CosPhi, 0.0f));
		}
	}

	const int32 Stride = LonSegments + 1;
	for (int32 Lat = 0; Lat < LatSegments; ++Lat)
	{
		for (int32 Lon = 0; Lon < LonSegments; ++Lon)
		{
			const int32 First = Lat * Stride + Lon;
			const int32 Second = First + Stride;

			Triangles.Add(First);
			Triangles.Add(Second);
			Triangles.Add(First + 1);

			Triangles.Add(Second);
			Triangles.Add(Second + 1);
			Triangles.Add(First + 1);
		}
	}

	ProceduralBallMesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, Colors, Tangents, false);
}

void AFootballBallActor::ApplyAerodynamicForces(float DeltaTime)
{
	if (!CollisionSphere || !CollisionSphere->IsSimulatingPhysics())
	{
		return;
	}

	// Convert UE units (cm) -> SI (meters) for physically exact aerodynamics
	const FVector VelCm = CollisionSphere->GetPhysicsLinearVelocity();
	const FVector RelVelM = (VelCm - WindVelocityCmPerSec) * 0.01f;
	const float SpeedM = RelVelM.Size();

	const FVector SpinRad = CollisionSphere->GetPhysicsAngularVelocityInRadians();
	const float RadiusM = AeroConfig.RadiusCm * 0.01f;
	const float CrossSectionAreaM2 = PI * RadiusM * RadiusM;

	if (SpeedM > 0.05f)
	{
		// 1. Quadratic Aerodynamic Drag: F_d = -0.5 * rho * C_d * A * |v| * v
		const float DragMagN = 0.5f * AeroConfig.AirDensity * AeroConfig.DragCoefficient * CrossSectionAreaM2 * SpeedM * SpeedM;
		const FVector DragForceN = -RelVelM.GetSafeNormal() * DragMagN;

		// 2. Magnus Lift Force: F_m = 0.5 * rho * C_l * A * r * (omega x v)
		// Scaled with spin parameter S = (r * |omega|) / |v| capped for realistic FIFA ball Reynolds numbers
		const FVector CrossSpinVel = FVector::CrossProduct(SpinRad, RelVelM);
		const float SpinSpeed = SpinRad.Size();
		const float EffectiveCL = AeroConfig.MagnusCoefficient * FMath::Clamp((RadiusM * SpinSpeed) / FMath::Max(SpeedM, 1.0f), 0.0f, 1.6f);
		const FVector MagnusDir = CrossSpinVel.GetSafeNormal();
		const float MagnusMagN = 0.5f * AeroConfig.AirDensity * EffectiveCL * CrossSectionAreaM2 * SpeedM * SpeedM;
		const FVector MagnusForceN = MagnusDir * MagnusMagN;

		// Convert Newtons (kg*m/s^2) -> UE Force (kg*cm/s^2 = N * 100)
		const FVector TotalAeroForceUE = (DragForceN + MagnusForceN) * 100.0f;
		CollisionSphere->AddForce(TotalAeroForceUE, NAME_None, false);
	}

	// 3. Spin decay in air
	if (!SpinRad.IsNearlyZero(0.01f))
	{
		const FVector DampedSpin = SpinRad * FMath::Exp(-AeroConfig.SpinAirDamping * DeltaTime);
		CollisionSphere->SetPhysicsAngularVelocityInRadians(DampedSpin, false);
	}

	// 4. Turf Rolling Friction when grounded (wet pitch = lower friction, faster skid!)
	if (IsBallGrounded())
	{
		const float WetFrictionScale = FMath::Lerp(1.0f, 0.58f, FMath::Clamp(PitchWetness, 0.0f, 1.0f));
		const float RollingDrag = AeroConfig.BaseRollingFriction * WetFrictionScale;
		FVector HorizVel(VelCm.X, VelCm.Y, 0.0f);
		const float HorizSpeed = HorizVel.Size();
		if (HorizSpeed > 2.0f)
		{
			const float Decel = RollingDrag * 980.665f * DeltaTime;
			const float NewSpeed = FMath::Max(0.0f, HorizSpeed - Decel);
			const FVector NewHoriz = HorizVel.GetSafeNormal() * NewSpeed;
			CollisionSphere->SetPhysicsLinearVelocity(FVector(NewHoriz.X, NewHoriz.Y, VelCm.Z));
		}
		else if (HorizSpeed > 0.0f && TimeSinceLastKick > 0.25f)
		{
			CollisionSphere->SetPhysicsLinearVelocity(FVector(0.0f, 0.0f, VelCm.Z));
		}
	}
}

void AFootballBallActor::UpdateDeformationSpring(float DeltaTime)
{
	// Damped harmonic oscillator simulating air-pressure elasticity of the leather/PU shell
	const float SpringAccel = (-AeroConfig.DeformationSpringStiffness * CurrentDeformationAmount)
		- (AeroConfig.DeformationSpringDamping * DeformationVelocity);

	DeformationVelocity += SpringAccel * DeltaTime;
	CurrentDeformationAmount += DeformationVelocity * DeltaTime;

	if (FMath::Abs(CurrentDeformationAmount) < 0.001f && FMath::Abs(DeformationVelocity) < 0.01f)
	{
		CurrentDeformationAmount = 0.0f;
		DeformationVelocity = 0.0f;
	}

	const float ClampedDef = FMath::Clamp(CurrentDeformationAmount, -AeroConfig.MaxDeformationRatio, AeroConfig.MaxDeformationRatio);
	// Volume-preserving squash & stretch: compress along impact axis, bulge perpendicular axes
	const float Compress = 1.0f - FMath::Abs(ClampedDef);
	const float Bulge = 1.0f / FMath::Sqrt(FMath::Max(Compress, 0.5f));

	const FVector LocalScale(Bulge, Bulge, Compress);
	if (ProceduralBallMesh && ProceduralBallMesh->IsVisible())
	{
		ProceduralBallMesh->SetRelativeScale3D(LocalScale);
	}
	if (CustomBallMesh && CustomBallMesh->IsVisible())
	{
		CustomBallMesh->SetRelativeScale3D(LocalScale);
	}
}

void AFootballBallActor::ExciteDeformation(const FVector& ImpactNormal, float ImpactSpeedCmPerSec)
{
	CurrentDeformationAxis = ImpactNormal.GetSafeNormal();
	const float NormalizedHit = FMath::Clamp(ImpactSpeedCmPerSec / 3000.0f, 0.0f, 1.0f);
	CurrentDeformationAmount = FMath::Max(CurrentDeformationAmount, NormalizedHit * AeroConfig.MaxDeformationRatio);
	DeformationVelocity = NormalizedHit * 2.4f;
}

void AFootballBallActor::ExecuteKick(const FFootballKickRequest& Request, AFootballCharacter* Kicker)
{
	if (!CollisionSphere)
	{
		return;
	}

	const FVector SafeDir = Request.Direction.GetSafeNormal();
	const FVector ExitVelocity = SafeDir * Request.SpeedCmPerSec;

	CollisionSphere->SetSimulatePhysics(true);
	CollisionSphere->SetPhysicsLinearVelocity(ExitVelocity, false);
	CollisionSphere->SetPhysicsAngularVelocityInRadians(Request.SpinRadPerSec, false);

	CurrentOwnerPlayer = nullptr;
	LastTouchPlayerId = Request.KickerPlayerId;
	TimeSinceLastKick = 0.0f;

	ExciteDeformation(-SafeDir, Request.SpeedCmPerSec);

	Execute_OnFootballKickExecuted(this, Request, GetActorLocation() - SafeDir * AeroConfig.RadiusCm);
	OnBallKicked.Broadcast(Request, Kicker);
	Execute_OnFootballBallImpact(this, EFootballBallSurface::PlayerBoot, Request.SpeedCmPerSec, GetActorLocation());
}

void AFootballBallActor::ExecuteCurvedShot(FVector TargetDirection, float PowerNormalized, float CurveRightLeftNormalized, AFootballCharacter* Kicker)
{
	FFootballKickRequest Req;
	const float ClampedPower = FMath::Clamp(PowerNormalized, 0.15f, 1.0f);
	const float ClampedCurve = FMath::Clamp(CurveRightLeftNormalized, -1.0f, 1.0f);

	FVector Dir = TargetDirection.GetSafeNormal();
	// Add realistic launch loft based on power
	Dir.Z = FMath::Clamp(Dir.Z + FMath::Lerp(0.10f, 0.24f, ClampedPower), 0.05f, 0.45f);
	Req.Direction = Dir.GetSafeNormal();
	Req.SpeedCmPerSec = FMath::Lerp(1650.0f, 3250.0f, ClampedPower);

	// Magnus spin: Z-axis angular velocity creates horizontal banana curve; perpendicular axis adds topspin dip
	const FVector RightAxis = FVector::CrossProduct(FVector::UpVector, Req.Direction).GetSafeNormal();
	const float SideSpinRad = ClampedCurve * 42.0f; // Up to ~6.7 rev/sec curl!
	const float TopSpinRad = FMath::Lerp(10.0f, 24.0f, ClampedPower);
	Req.SpinRadPerSec = FVector(0.0f, 0.0f, SideSpinRad) + RightAxis * TopSpinRad;
	Req.KickStyle = FMath::Abs(ClampedCurve) > 0.25f ? EFootballKickStyle::FinesseCurl : EFootballKickStyle::PowerShot;

	ExecuteKick(Req, Kicker);
}

void AFootballBallActor::ResetBallToLocation(const FVector& NewWorldLocation)
{
	CurrentOwnerPlayer = nullptr;
	TimeSinceLastKick = 5.0f;
	CurrentDeformationAmount = 0.0f;
	DeformationVelocity = 0.0f;

	SetActorLocation(NewWorldLocation + FVector(0.0f, 0.0f, AeroConfig.RadiusCm + 2.0f), false, nullptr, ETeleportType::TeleportPhysics);
	if (CollisionSphere)
	{
		CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector, false);
		CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector, false);
	}
}

void AFootballBallActor::SetEnvironmentConditions(const FVector& InWindVelocityCmPerSec, float InPitchWetness)
{
	WindVelocityCmPerSec = InWindVelocityCmPerSec;
	PitchWetness = FMath::Clamp(InPitchWetness, 0.0f, 1.0f);
}

FVector AFootballBallActor::PredictPositionAtTime(float SecondsAhead, int32 Substeps) const
{
	if (SecondsAhead <= 0.0f || Substeps <= 0)
	{
		return GetActorLocation();
	}

	FVector PosM = GetActorLocation() * 0.01f;
	FVector VelM = GetBallVelocity() * 0.01f;
	FVector SpinRad = GetBallSpinRadPerSec();
	const FVector WindM = WindVelocityCmPerSec * 0.01f;

	const float RadiusM = AeroConfig.RadiusCm * 0.01f;
	const float AreaM2 = PI * RadiusM * RadiusM;
	const float Dt = SecondsAhead / static_cast<float>(Substeps);

	for (int32 Step = 0; Step < Substeps; ++Step)
	{
		const FVector RelVel = VelM - WindM;
		const float Speed = RelVel.Size();

		FVector Accel(0.0f, 0.0f, -9.80665f);
		if (Speed > 0.05f)
		{
			const float DragF = 0.5f * AeroConfig.AirDensity * AeroConfig.DragCoefficient * AreaM2 * Speed * Speed;
			Accel += (-RelVel.GetSafeNormal() * DragF) / AeroConfig.MassKg;

			const FVector Cross = FVector::CrossProduct(SpinRad, RelVel);
			const float EffCL = AeroConfig.MagnusCoefficient * FMath::Clamp((RadiusM * SpinRad.Size()) / FMath::Max(Speed, 1.0f), 0.0f, 1.6f);
			const float MagF = 0.5f * AeroConfig.AirDensity * EffCL * AreaM2 * Speed * Speed;
			Accel += (Cross.GetSafeNormal() * MagF) / AeroConfig.MassKg;
		}

		VelM += Accel * Dt;
		PosM += VelM * Dt;
		SpinRad *= FMath::Exp(-AeroConfig.SpinAirDamping * Dt);

		// Approximate turf bounce
		if (PosM.Z < RadiusM)
		{
			PosM.Z = RadiusM;
			if (VelM.Z < 0.0f)
			{
				VelM.Z = -VelM.Z * AeroConfig.TurfRestitution;
				VelM.X *= 0.88f;
				VelM.Y *= 0.88f;
			}
		}
	}

	return PosM * 100.0f;
}

FVector AFootballBallActor::GetBallVelocity() const
{
	return CollisionSphere ? CollisionSphere->GetPhysicsLinearVelocity() : FVector::ZeroVector;
}

FVector AFootballBallActor::GetBallSpinRadPerSec() const
{
	return CollisionSphere ? CollisionSphere->GetPhysicsAngularVelocityInRadians() : FVector::ZeroVector;
}

bool AFootballBallActor::IsBallLooseOrInFlight() const
{
	if (CurrentOwnerPlayer.IsValid())
	{
		return false;
	}
	return GetBallVelocity().Size() > 220.0f || !IsBallGrounded() || TimeSinceLastKick < 1.25f;
}

bool AFootballBallActor::IsBallGrounded() const
{
	return GetActorLocation().Z <= (AeroConfig.RadiusCm + 8.0f);
}

void AFootballBallActor::SetCurrentOwnerPlayer(AFootballCharacter* NewOwner)
{
	CurrentOwnerPlayer = NewOwner;
}

FFootballDeterministicBallState AFootballBallActor::CaptureDeterministicState() const
{
	FFootballDeterministicBallState State;
	State.Position = FFootballQuantizedVector::FromFVector(GetActorLocation());
	State.LinearVelocity = FFootballQuantizedVector::FromFVector(GetBallVelocity());
	State.AngularVelocityRad = FFootballQuantizedVector::FromFVector(GetBallSpinRadPerSec());
	State.bIsGrounded = IsBallGrounded();
	State.LastTouchPlayerId = LastTouchPlayerId;
	return State;
}

void AFootballBallActor::OnDeterministicStep_Implementation(int32 SimulationTick, float FixedDeltaTime)
{
	// Deterministic telemetry hook (Chaos async fixed-step handles body integration)
}

int32 AFootballBallActor::GetDeterministicStateHash_Implementation() const
{
	return static_cast<int32>(CaptureDeterministicState().ComputeHash());
}

void AFootballBallActor::ApplyDeterministicSnapshot_Implementation(const FFootballDeterministicFrameSnapshot& Snapshot)
{
	SetActorLocation(Snapshot.BallState.Position.ToFVector(), false, nullptr, ETeleportType::TeleportPhysics);
	if (CollisionSphere)
	{
		CollisionSphere->SetPhysicsLinearVelocity(Snapshot.BallState.LinearVelocity.ToFVector(), false);
		CollisionSphere->SetPhysicsAngularVelocityInRadians(Snapshot.BallState.AngularVelocityRad.ToFVector(), false);
	}
	LastTouchPlayerId = Snapshot.BallState.LastTouchPlayerId;
}

void AFootballBallActor::OnFootballKickExecuted_Implementation(const FFootballKickRequest& KickRequest, FVector ContactWorldLocation)
{
	// Override in Blueprint for extra kick particles/camera punch
}

void AFootballBallActor::OnFootballBallImpact_Implementation(EFootballBallSurface Surface, float ImpactSpeedCmPerSec, FVector ImpactPoint)
{
	OnBallImpact.Broadcast(Surface, ImpactSpeedCmPerSec, ImpactPoint);
}

void AFootballBallActor::HandleBallHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	const float ImpactSpeed = GetBallVelocity().Size();
	if (ImpactSpeed < 80.0f)
	{
		return;
	}

	// Spin-to-velocity transfer on turf bounce (topspin accelerates, backspin checks!)
	if (Hit.ImpactNormal.Z > 0.7f && CollisionSphere)
	{
		const FVector SpinRad = CollisionSphere->GetPhysicsAngularVelocityInRadians();
		const FVector TangentialSpinBoost = FVector::CrossProduct(SpinRad, FVector(0.0f, 0.0f, -AeroConfig.RadiusCm)) * 0.25f;
		const float WetSkidFactor = FMath::Lerp(1.0f, 1.18f, PitchWetness);
		FVector NewVel = CollisionSphere->GetPhysicsLinearVelocity() + TangentialSpinBoost;
		NewVel.X *= WetSkidFactor;
		NewVel.Y *= WetSkidFactor;
		CollisionSphere->SetPhysicsLinearVelocity(NewVel, false);
	}

	ExciteDeformation(Hit.ImpactNormal, ImpactSpeed);

	EFootballBallSurface Surface = EFootballBallSurface::Turf;
	if (OtherComp && OtherComp->ComponentHasTag(TEXT("GoalPost")))
	{
		Surface = EFootballBallSurface::Post;
	}
	else if (OtherComp && OtherComp->ComponentHasTag(TEXT("GoalNet")))
	{
		Surface = EFootballBallSurface::Net;
		// Goal net absorbs momentum realistically
		if (CollisionSphere)
		{
			CollisionSphere->SetPhysicsLinearVelocity(CollisionSphere->GetPhysicsLinearVelocity() * 0.28f, false);
		}
	}
	else if (Hit.ImpactNormal.Z < 0.5f)
	{
		Surface = EFootballBallSurface::StadiumWall;
	}

	Execute_OnFootballBallImpact(this, Surface, ImpactSpeed, Hit.ImpactPoint);
}
