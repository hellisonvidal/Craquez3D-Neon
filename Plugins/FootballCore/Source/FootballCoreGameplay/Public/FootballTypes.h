// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "FootballTypes.generated.h"

UENUM(BlueprintType)
enum class EFootballMatchMode : uint8
{
	/** 11vs11 full-pitch tactical match ("Jogo-Treino") */
	Training11v11 UMETA(DisplayName = "Jogo-Treino 11vs11"),
	/** 1vs1 Striker vs Goalkeeper finishing & save duel */
	Duel1v1 UMETA(DisplayName = "Duelo 1vs1 (Atacante x Goleiro)")
};

UENUM(BlueprintType)
enum class EFootballTeamSide : uint8
{
	Home UMETA(DisplayName = "Craquez FC (Casa)"),
	Away UMETA(DisplayName = "Rival FC (Visitante)"),
	Neutral UMETA(DisplayName = "Neutro")
};

UENUM(BlueprintType)
enum class EFootballPlayerRole : uint8
{
	Goalkeeper UMETA(DisplayName = "Goleiro (GK)"),
	CenterBack UMETA(DisplayName = "Zagueiro (CB)"),
	LeftBack UMETA(DisplayName = "Lateral Esquerdo (LB)"),
	RightBack UMETA(DisplayName = "Lateral Direito (RB)"),
	DefensiveMid UMETA(DisplayName = "Volante (CDM)"),
	CentralMid UMETA(DisplayName = "Meia Central (CM)"),
	AttackingMid UMETA(DisplayName = "Meia Atacante (CAM)"),
	LeftWing UMETA(DisplayName = "Ponta Esquerda (LW)"),
	RightWing UMETA(DisplayName = "Ponta Direita (RW)"),
	Striker UMETA(DisplayName = "Centroavante (ST)")
};

UENUM(BlueprintType)
enum class EFootballWeatherType : uint8
{
	ClearDay UMETA(DisplayName = "Dia Ensolarado"),
	GoldenHour UMETA(DisplayName = "Por do Sol"),
	NightFloodlights UMETA(DisplayName = "Noite com Refletores"),
	LightRain UMETA(DisplayName = "Chuva Leve"),
	StormRain UMETA(DisplayName = "Tempestade")
};

UENUM(BlueprintType)
enum class EFootballKickStyle : uint8
{
	DrivenPass UMETA(DisplayName = "Passe Rasteiro"),
	LobPass UMETA(DisplayName = "Lancamento / Cavadinha"),
	PowerShot UMETA(DisplayName = "Chute Forte (Peito do Pe)"),
	FinesseCurl UMETA(DisplayName = "Chute Colocado (Efeito Magnus)"),
	TrivelaOutside UMETA(DisplayName = "Trivela (Tres Dedos)"),
	Clearance UMETA(DisplayName = "Chutao / Afastar"),
	GoalkeeperThrow UMETA(DisplayName = "Reposicao do Goleiro")
};

UENUM(BlueprintType)
enum class EFootballBallSurface : uint8
{
	Turf UMETA(DisplayName = "Grama"),
	Post UMETA(DisplayName = "Trave"),
	Net UMETA(DisplayName = "Rede"),
	PlayerBoot UMETA(DisplayName = "Chuteira"),
	KeeperGlove UMETA(DisplayName = "Luva do Goleiro"),
	StadiumWall UMETA(DisplayName = "Placa / Estadio")
};

/**
 * Physical kit appearance & Chaos Cloth configuration for separate 3D uniform garments
 * (Jersey, Shorts, Socks, Boots — never painted on character skin).
 */
USTRUCT(BlueprintType)
struct FOOTBALLCOREGAMEPLAY_API FFootballKitConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	FLinearColor JerseyPrimaryColor = FLinearColor(0.02f, 0.72f, 0.35f, 1.0f); // Craquez Neon Emerald

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	FLinearColor JerseySecondaryColor = FLinearColor(0.0f, 0.90f, 0.95f, 1.0f); // Electric Cyan

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	FLinearColor ShortsColor = FLinearColor(0.06f, 0.08f, 0.14f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	FLinearColor SocksColor = FLinearColor(0.02f, 0.72f, 0.35f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	FLinearColor BootsColor = FLinearColor(0.95f, 0.98f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	int32 SquadNumber = 10;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit")
	FName TeamName = TEXT("CRAQUEZ FC");

	/** Enable real-time Chaos Cloth simulation on Jersey & Shorts (auto-LODs on Android) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit|Cloth")
	bool bEnableChaosCloth = true;

	/** Max distance (cm) from camera where Chaos Cloth stays active (mobile perf optimization) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit|Cloth")
	float ClothLODDisableDistanceCm = 2200.0f;

	/** Fabric stretch/bend stiffness [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kit|Cloth", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ClothStiffness = 0.65f;
};

/**
 * FIFA Size-5 aerodynamic & Chaos collision parameters for realistic ball behavior.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCOREGAMEPLAY_API FFootballBallAerodynamicsConfig
{
	GENERATED_BODY()

	/** Official FIFA Size 5 ball mass: 0.43 kg */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float MassKg = 0.43f;

	/** Official FIFA Size 5 radius: 11.0 cm (diameter 22 cm) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float RadiusCm = 11.0f;

	/** Air density kg/m^3 at sea level */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float AirDensity = 1.225f;

	/** Quadratic drag coefficient C_D */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float DragCoefficient = 0.24f;

	/** Magnus lift coefficient C_L (controls curve intensity from spin) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float MagnusCoefficient = 0.34f;

	/** Rolling resistance deceleration on dry turf */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float BaseRollingFriction = 0.42f;

	/** Bounce elasticity on turf [0..1] */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float TurfRestitution = 0.62f;

	/** Angular velocity damping (spin decay in air) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float SpinAirDamping = 0.35f;

	/** Damped harmonic spring stiffness for impact deformation squash */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float DeformationSpringStiffness = 220.0f;

	/** Damped harmonic damping for impact deformation recovery */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float DeformationSpringDamping = 16.0f;

	/** Maximum visual squash ratio (0.28 = up to 28% compression on a rocket shot) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|BallPhysics")
	float MaxDeformationRatio = 0.28f;
};

/**
 * Kick execution request passed between Player/AI and Ball/Animation.
 */
USTRUCT(BlueprintType)
struct FOOTBALLCOREGAMEPLAY_API FFootballKickRequest
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kick")
	FVector Direction = FVector::ForwardVector;

	/** Desired exit speed in cm/s (e.g., 1600 = pass, 2800 = strong shot, 3300 = rocket) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kick")
	float SpeedCmPerSec = 2400.0f;

	/** Spin vector in rad/s: Z = side curl (Magnus left/right), Y = topspin/backspin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kick")
	FVector SpinRadPerSec = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kick")
	EFootballKickStyle KickStyle = EFootballKickStyle::PowerShot;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Kick")
	int32 KickerPlayerId = -1;
};
