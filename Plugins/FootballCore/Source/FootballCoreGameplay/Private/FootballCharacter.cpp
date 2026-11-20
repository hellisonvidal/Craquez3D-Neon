// Copyright Craquez Studio. All Rights Reserved.

#include "FootballCharacter.h"
#include "FootballBallActor.h"
#include "FootballUniformComponent.h"
#include "FootballProceduralAnimComponent.h"
#include "FootballMatchGameMode.h"
#include "FootballWeatherAndAudioManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "MotionWarpingComponent.h"
#include "EngineUtils.h"

AFootballCharacter::AFootballCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(38.0f, 88.0f);
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement = true;
		MoveComp->RotationRate = FRotator(0.0f, 580.0f, 0.0f);
		MoveComp->MaxWalkSpeed = JogSpeedCmPerSec;
		MoveComp->BrakingDecelerationWalking = 1800.0f;
		MoveComp->MaxAcceleration = 1600.0f;
	}

	// Broadcast-friendly action camera arm (smoothly tracks play on Android landscape screens)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1250.0f;
	CameraBoom->SetRelativeRotation(FRotator(-34.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = false;
	CameraBoom->bInheritPitch = false;
	CameraBoom->bInheritYaw = false;
	CameraBoom->bInheritRoll = false;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 6.5f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->FieldOfView = 62.0f;

	UniformComponent = CreateDefaultSubobject<UFootballUniformComponent>(TEXT("UniformComponent"));
	ProceduralAnimComponent = CreateDefaultSubobject<UFootballProceduralAnimComponent>(TEXT("ProceduralAnimComponent"));
	MotionWarpingComponent = CreateDefaultSubobject<UMotionWarpingComponent>(TEXT("MotionWarpingComponent"));
}

void AFootballCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (UniformComponent)
	{
		UniformComponent->InitializeGarments(GetMesh(), GetRootComponent(), PlayerRole == EFootballPlayerRole::Goalkeeper);
	}
}

void AFootballCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (KickCooldownTimer > 0.0f)
	{
		KickCooldownTimer = FMath::Max(0.0f, KickCooldownTimer - DeltaTime);
	}
	if (DiveCooldownTimer > 0.0f)
	{
		DiveCooldownTimer = FMath::Max(0.0f, DiveCooldownTimer - DeltaTime);
	}

	if (bIsChargingShot)
	{
		CurrentShotCharge = FMath::Clamp(CurrentShotCharge + DeltaTime * 1.15f, 0.0f, 1.0f);
	}

	UpdateStaminaAndSpeed(DeltaTime);
	UpdateDribbleControl(DeltaTime);
}

void AFootballCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (!PlayerInputComponent)
	{
		return;
	}

	PlayerInputComponent->BindAxis(TEXT("Football_MoveForward"), this, &AFootballCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("Football_MoveRight"), this, &AFootballCharacter::MoveRight);

	PlayerInputComponent->BindAction(TEXT("Football_Shoot"), IE_Pressed, this, &AFootballCharacter::Input_ShootPressed);
	PlayerInputComponent->BindAction(TEXT("Football_Shoot"), IE_Released, this, &AFootballCharacter::Input_ShootReleased);
	PlayerInputComponent->BindAction(TEXT("Football_Pass"), IE_Pressed, this, &AFootballCharacter::Input_PassPressed);
	PlayerInputComponent->BindAction(TEXT("Football_LobPass"), IE_Pressed, this, &AFootballCharacter::Input_LobPassPressed);
	PlayerInputComponent->BindAction(TEXT("Football_Sprint"), IE_Pressed, this, &AFootballCharacter::Input_SprintStart);
	PlayerInputComponent->BindAction(TEXT("Football_Sprint"), IE_Released, this, &AFootballCharacter::Input_SprintStop);
	PlayerInputComponent->BindAction(TEXT("Football_SwitchPlayer"), IE_Pressed, this, &AFootballCharacter::Input_SwitchPlayer);
	PlayerInputComponent->BindAction(TEXT("Football_ToggleMode"), IE_Pressed, this, &AFootballCharacter::Input_ToggleMatchMode);
	PlayerInputComponent->BindAction(TEXT("Football_CycleWeather"), IE_Pressed, this, &AFootballCharacter::Input_CycleWeather);
}

void AFootballCharacter::ConfigurePlayer(
	int32 InPlayerId,
	EFootballTeamSide InTeamSide,
	EFootballPlayerRole InRole,
	const FVector& InFormationHomeSpot,
	const FFootballKitConfig& InKitConfig)
{
	PlayerId = InPlayerId;
	TeamSide = InTeamSide;
	PlayerRole = InRole;
	FormationHomeLocation = InFormationHomeSpot;

	if (UniformComponent)
	{
		UniformComponent->InitializeGarments(GetMesh(), GetRootComponent(), PlayerRole == EFootballPlayerRole::Goalkeeper);
		UniformComponent->ApplyKitConfig(InKitConfig);
	}
}

void AFootballCharacter::MoveForward(float Value)
{
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		// Pitch X-axis is attacking length (+X for Home, -X for Away)
		AddMovementInput(FVector::ForwardVector, Value);
	}
}

void AFootballCharacter::MoveRight(float Value)
{
	if (FMath::Abs(Value) > KINDA_SMALL_NUMBER)
	{
		AddMovementInput(FVector::RightVector, Value);
	}
}

void AFootballCharacter::UpdateStaminaAndSpeed(float DeltaTime)
{
	const bool bMoving = GetVelocity().SizeSquared2D() > 100.0f;
	if (bIsSprinting && bMoving && StaminaNormalized > 0.05f)
	{
		StaminaNormalized = FMath::Max(0.0f, StaminaNormalized - DeltaTime * 0.11f);
	}
	else
	{
		StaminaNormalized = FMath::Min(1.0f, StaminaNormalized + DeltaTime * 0.08f);
	}

	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		const bool bCanSprint = bIsSprinting && StaminaNormalized > 0.05f;
		const float SpeedMult = HasBallPossession() ? 0.93f : 1.0f;
		MoveComp->MaxWalkSpeed = (bCanSprint ? SprintSpeedCmPerSec : JogSpeedCmPerSec) * SpeedMult;
	}
}

void AFootballCharacter::UpdateDribbleControl(float DeltaTime)
{
	AFootballBallActor* Ball = GetMatchBall();
	if (!Ball || KickCooldownTimer > 0.0f)
	{
		return;
	}

	const FVector BallLoc = Ball->GetActorLocation();
	const FVector MyLoc = GetActorLocation();
	const float Dist2D = FVector::Dist2D(BallLoc, MyLoc);
	const float HeightDiff = BallLoc.Z - (MyLoc.Z - 88.0f);

	AFootballCharacter* CurrentOwner = Ball->GetCurrentOwnerPlayer();

	// Claim ball if within boot reach and reasonably grounded
	if (Dist2D <= BallCaptureRadiusCm && HeightDiff < 110.0f)
	{
		if (CurrentOwner == nullptr)
		{
			Ball->SetCurrentOwnerPlayer(this);
			CurrentOwner = this;
		}
		else if (CurrentOwner != this && CurrentOwner->TeamSide != TeamSide && Dist2D < 68.0f)
		{
			// Close physical duel — trigger Euphoria reaction & turnover chance
			if (CurrentOwner->ProceduralAnimComponent)
			{
				const FVector PushDir = (CurrentOwner->GetActorLocation() - MyLoc).GetSafeNormal2D();
				CurrentOwner->ProceduralAnimComponent->TriggerEuphoriaImpact(PushDir * 240.0f);
			}
			Ball->SetCurrentOwnerPlayer(this);
			CurrentOwner = this;
			KickCooldownTimer = 0.2f;
		}
	}

	// Keep ball smoothly positioned at feet while dribbling
	if (CurrentOwner == this)
	{
		if (Dist2D > 195.0f)
		{
			Ball->SetCurrentOwnerPlayer(nullptr);
			return;
		}

		const FVector ForwardDir = GetVelocity().SizeSquared2D() > 400.0f
			? GetVelocity().GetSafeNormal2D()
			: GetActorForwardVector().GetSafeNormal2D();

		const FVector DesiredBallSpot = FVector(MyLoc.X, MyLoc.Y, Ball->AeroConfig.RadiusCm + 1.0f) + ForwardDir * 68.0f;
		const FVector ToSpot = DesiredBallSpot - BallLoc;
		const FVector DribbleVel = (ToSpot * 11.0f) + GetVelocity() * 0.72f;

		if (Ball->CollisionSphere)
		{
			Ball->CollisionSphere->SetPhysicsLinearVelocity(FVector(DribbleVel.X, DribbleVel.Y, FMath::Min(Ball->GetBallVelocity().Z, 40.0f)));
			// Roll spin visually matching dribble velocity
			const FVector RollAxis = FVector::CrossProduct(FVector::UpVector, ForwardDir);
			Ball->CollisionSphere->SetPhysicsAngularVelocityInRadians(RollAxis * (GetVelocity().Size2D() / FMath::Max(Ball->AeroConfig.RadiusCm, 1.0f)));
		}
	}
}

bool AFootballCharacter::PerformShot(FVector AimDirection, float PowerNormalized, float MagnusCurveNormalized)
{
	AFootballBallActor* Ball = GetMatchBall();
	if (!Ball)
	{
		return false;
	}

	const float Dist = FVector::Dist2D(GetActorLocation(), Ball->GetActorLocation());
	if (!HasBallPossession() && Dist > 165.0f)
	{
		return false;
	}

	if (AimDirection.IsNearlyZero())
	{
		AimDirection = (GetAttackingGoalLocation() - Ball->GetActorLocation()).GetSafeNormal2D();
	}

	if (ProceduralAnimComponent)
	{
		const EFootballKickStyle Style = FMath::Abs(MagnusCurveNormalized) > 0.25f ? EFootballKickStyle::FinesseCurl : EFootballKickStyle::PowerShot;
		ProceduralAnimComponent->PrepareKickMotionWarp(Ball->GetActorLocation(), AimDirection, Style, true);
	}

	KickCooldownTimer = 0.38f;
	Ball->ExecuteCurvedShot(AimDirection, PowerNormalized, MagnusCurveNormalized, this);
	return true;
}

bool AFootballCharacter::PerformPass(AFootballCharacter* ExplicitTarget, bool bLobbed)
{
	AFootballBallActor* Ball = GetMatchBall();
	if (!Ball)
	{
		return false;
	}

	const float DistToBall = FVector::Dist2D(GetActorLocation(), Ball->GetActorLocation());
	if (!HasBallPossession() && DistToBall > 160.0f)
	{
		return false;
	}

	AFootballCharacter* Target = ExplicitTarget ? ExplicitTarget : FindBestPassTarget();
	FVector Dir = GetActorForwardVector().GetSafeNormal2D();
	float Speed = bLobbed ? 1950.0f : 1750.0f;

	if (Target)
	{
		// Lead the pass slightly ahead of teammate velocity
		const FVector LeadTarget = Target->GetActorLocation() + Target->GetVelocity() * 0.45f;
		const FVector Delta = LeadTarget - Ball->GetActorLocation();
		const float Dist = Delta.Size2D();
		Dir = Delta.GetSafeNormal2D();
		Speed = FMath::Clamp(Dist * (bLobbed ? 0.92f : 1.05f), 1150.0f, 2550.0f);
	}

	FFootballKickRequest Req;
	Req.Direction = (Dir + FVector(0.0f, 0.0f, bLobbed ? 0.42f : 0.05f)).GetSafeNormal();
	Req.SpeedCmPerSec = Speed;
	// Slight backspin on lobbed passes so ball sits nicely on the turf
	const FVector RightAxis = FVector::CrossProduct(FVector::UpVector, Dir);
	Req.SpinRadPerSec = bLobbed ? (-RightAxis * 18.0f) : FVector::ZeroVector;
	Req.KickStyle = bLobbed ? EFootballKickStyle::LobPass : EFootballKickStyle::DrivenPass;
	Req.KickerPlayerId = PlayerId;

	if (ProceduralAnimComponent)
	{
		ProceduralAnimComponent->PrepareKickMotionWarp(Ball->GetActorLocation(), Dir, Req.KickStyle, true);
	}

	KickCooldownTimer = 0.35f;
	Ball->ExecuteKick(Req, this);
	return true;
}

void AFootballCharacter::PerformGoalkeeperDive(const FVector& TargetInterceptPoint)
{
	if (DiveCooldownTimer > 0.0f)
	{
		return;
	}

	DiveCooldownTimer = 1.15f;
	const FVector DiveDir = (TargetInterceptPoint - GetActorLocation()).GetSafeNormal2D();
	LaunchCharacter(DiveDir * 680.0f + FVector(0.0f, 0.0f, 240.0f), true, true);

	if (ProceduralAnimComponent)
	{
		ProceduralAnimComponent->TriggerEuphoriaImpact(DiveDir * 180.0f, TEXT("spine_01"), 0.75f);
	}

	// Check parry/save radius against ball
	if (AFootballBallActor* Ball = GetMatchBall())
	{
		const float Dist = FVector::Dist(GetActorLocation(), Ball->GetActorLocation());
		if (Dist < 235.0f)
		{
			FFootballKickRequest Parry;
			Parry.Direction = (GetActorForwardVector().GetSafeNormal2D() + FVector(0.0f, FMath::Sign(DiveDir.Y) * 0.6f, 0.35f)).GetSafeNormal();
			Parry.SpeedCmPerSec = 1450.0f;
			Parry.KickStyle = EFootballKickStyle::GoalkeeperThrow;
			Parry.KickerPlayerId = PlayerId;
			Ball->ExecuteKick(Parry, this);
			Ball->Execute_OnFootballBallImpact(Ball, EFootballBallSurface::KeeperGlove, 1450.0f, Ball->GetActorLocation());
		}
	}
}

bool AFootballCharacter::PerformTackle()
{
	AFootballBallActor* Ball = GetMatchBall();
	if (!Ball)
	{
		return false;
	}

	const float Dist = FVector::Dist2D(GetActorLocation(), Ball->GetActorLocation());
	if (Dist > 185.0f)
	{
		return false;
	}

	const FVector LungeDir = (Ball->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	LaunchCharacter(LungeDir * 440.0f, true, false);

	if (AFootballCharacter* Carrier = Ball->GetCurrentOwnerPlayer())
	{
		if (Carrier != this && Carrier->TeamSide != TeamSide && Carrier->ProceduralAnimComponent)
		{
			Carrier->ProceduralAnimComponent->TriggerEuphoriaImpact(LungeDir * 340.0f);
		}
	}

	Ball->SetCurrentOwnerPlayer(this);
	return true;
}

void AFootballCharacter::Input_ShootPressed()
{
	bIsChargingShot = true;
	CurrentShotCharge = 0.22f;
}

void AFootballCharacter::Input_ShootReleased()
{
	if (!bIsChargingShot)
	{
		return;
	}
	bIsChargingShot = false;

	const FVector ToGoal = (GetAttackingGoalLocation() - GetActorLocation()).GetSafeNormal2D();
	const FVector Facing = GetActorForwardVector().GetSafeNormal2D();
	// Blend player facing with goal direction for natural assisted aiming on Android touch
	const FVector AimDir = (Facing * 0.65f + ToGoal * 0.35f).GetSafeNormal2D();
	const float CrossZ = FVector::CrossProduct(Facing, ToGoal).Z;
	const float MagnusCurve = FMath::Clamp(CrossZ * 1.4f, -0.85f, 0.85f);

	if (!PerformShot(AimDir, CurrentShotCharge, MagnusCurve))
	{
		// When defending without the ball, Shoot button performs Tackle!
		PerformTackle();
	}
	CurrentShotCharge = 0.0f;
}

void AFootballCharacter::Input_PassPressed()
{
	if (!PerformPass(nullptr, false))
	{
		Input_SwitchPlayer();
	}
}

void AFootballCharacter::Input_LobPassPressed()
{
	PerformPass(nullptr, true);
}

void AFootballCharacter::Input_SprintStart()
{
	bIsSprinting = true;
}

void AFootballCharacter::Input_SprintStop()
{
	bIsSprinting = false;
}

void AFootballCharacter::Input_SwitchPlayer()
{
	if (UWorld* World = GetWorld())
	{
		if (AFootballMatchGameMode* GM = World->GetAuthGameMode<AFootballMatchGameMode>())
		{
			GM->SwitchHumanControlToClosestTeammate();
		}
	}
}

void AFootballCharacter::Input_ToggleMatchMode()
{
	if (UWorld* World = GetWorld())
	{
		if (AFootballMatchGameMode* GM = World->GetAuthGameMode<AFootballMatchGameMode>())
		{
			const EFootballMatchMode NextMode = (GM->CurrentMatchMode == EFootballMatchMode::Training11v11)
				? EFootballMatchMode::Duel1v1
				: EFootballMatchMode::Training11v11;
			GM->StartMatchMode(NextMode);
		}
	}
}

void AFootballCharacter::Input_CycleWeather()
{
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFootballWeatherAndAudioManager> It(World); It; ++It)
		{
			It->CycleNextWeather();
			break;
		}
	}
}

bool AFootballCharacter::HasBallPossession() const
{
	const AFootballBallActor* Ball = GetMatchBall();
	return Ball && Ball->GetCurrentOwnerPlayer() == this;
}

AFootballBallActor* AFootballCharacter::GetMatchBall() const
{
	if (CachedBall.IsValid())
	{
		return CachedBall.Get();
	}
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AFootballBallActor> It(World); It; ++It)
		{
			CachedBall = *It;
			return *It;
		}
	}
	return nullptr;
}

FVector AFootballCharacter::GetAttackingGoalLocation() const
{
	// Pitch runs from X = -5250 cm (Home Goal) to X = +5250 cm (Away Goal)
	return (TeamSide == EFootballTeamSide::Home)
		? FVector(5250.0f, 0.0f, 110.0f)
		: FVector(-5250.0f, 0.0f, 110.0f);
}

FVector AFootballCharacter::GetDefendingGoalLocation() const
{
	return (TeamSide == EFootballTeamSide::Home)
		? FVector(-5250.0f, 0.0f, 110.0f)
		: FVector(5250.0f, 0.0f, 110.0f);
}

bool AFootballCharacter::RequestFootballPass_Implementation(AActor* TargetTeammate, bool bLobbed)
{
	return PerformPass(Cast<AFootballCharacter>(TargetTeammate), bLobbed);
}

void AFootballCharacter::ApplyFootballKit_Implementation(const FFootballKitConfig& NewKit)
{
	if (UniformComponent)
	{
		UniformComponent->ApplyKitConfig(NewKit);
	}
}

AFootballCharacter* AFootballCharacter::FindBestPassTarget() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FVector MyLoc = GetActorLocation();
	const FVector MyForward = GetActorForwardVector().GetSafeNormal2D();

	AFootballCharacter* BestTarget = nullptr;
	float BestScore = -1e9f;

	for (TActorIterator<AFootballCharacter> It(World); It; ++It)
	{
		AFootballCharacter* Candidate = *It;
		if (!Candidate || Candidate == this || Candidate->TeamSide != TeamSide)
		{
			continue;
		}

		const FVector ToCand = Candidate->GetActorLocation() - MyLoc;
		const float Dist = ToCand.Size2D();
		if (Dist < 350.0f || Dist > 4200.0f)
		{
			continue;
		}

		const float Alignment = FVector::DotProduct(MyForward, ToCand.GetSafeNormal2D());
		if (Alignment < 0.15f)
		{
			continue;
		}

		const float Score = (Alignment * 1500.0f) - (Dist * 0.25f);
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Candidate;
		}
	}
	return BestTarget;
}
