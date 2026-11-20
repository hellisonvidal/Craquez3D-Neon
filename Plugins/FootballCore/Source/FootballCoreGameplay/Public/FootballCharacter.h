// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FootballTypes.h"
#include "FootballBlueprintInterface.h"
#include "FootballCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UMotionWarpingComponent;
class UFootballUniformComponent;
class UFootballProceduralAnimComponent;
class AFootballBallActor;

/**
 * Core Footballer Character for Craquez (Human & AI controlled, Outfield & Goalkeeper).
 * Features:
 * - Modular separate 3D uniform garments via UFootballUniformComponent (Chaos Cloth ready)
 * - Procedural Foot IK, Kick Motion Warping & Euphoria Physics reactions
 * - Smooth ball dribbling magnet, power/Magnus-curved shooting, passing, lobbing & GK dives
 * - Touch-friendly Android + Gamepad + Keyboard controls
 */
UCLASS(Blueprintable, BlueprintType)
class FOOTBALLCOREGAMEPLAY_API AFootballCharacter : public ACharacter, public IFootballBlueprintInterface
{
	GENERATED_BODY()

public:
	AFootballCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	/** Initialize identity, formation home spot, and physical kit */
	UFUNCTION(BlueprintCallable, Category = "Football|Player")
	void ConfigurePlayer(
		int32 InPlayerId,
		EFootballTeamSide InTeamSide,
		EFootballPlayerRole InRole,
		const FVector& InFormationHomeSpot,
		const FFootballKitConfig& InKitConfig);

	/** Shoot toward aim direction with normalized power [0..1] and Magnus curve [-1..1] */
	UFUNCTION(BlueprintCallable, Category = "Football|Actions")
	bool PerformShot(FVector AimDirection, float PowerNormalized = 0.8f, float MagnusCurveNormalized = 0.35f);

	/** Pass smoothly to best teammate in aim cone (or specific target) */
	UFUNCTION(BlueprintCallable, Category = "Football|Actions")
	bool PerformPass(AFootballCharacter* ExplicitTarget = nullptr, bool bLobbed = false);

	/** Goalkeeper dive toward predicted shot intercept point */
	UFUNCTION(BlueprintCallable, Category = "Football|Actions")
	void PerformGoalkeeperDive(const FVector& TargetInterceptPoint);

	/** Attempt standing tackle / ball steal when close to opponent carrier */
	UFUNCTION(BlueprintCallable, Category = "Football|Actions")
	bool PerformTackle();

	/** Touch/UI callable triggers so Android virtual buttons work directly from UMG */
	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_ShootPressed();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_ShootReleased();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_PassPressed();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_LobPassPressed();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_SprintStart();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_SprintStop();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_SwitchPlayer();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_ToggleMatchMode();

	UFUNCTION(BlueprintCallable, Category = "Football|Input")
	void Input_CycleWeather();

	UFUNCTION(BlueprintPure, Category = "Football|Player")
	bool HasBallPossession() const;

	UFUNCTION(BlueprintPure, Category = "Football|Player")
	AFootballBallActor* GetMatchBall() const;

	UFUNCTION(BlueprintPure, Category = "Football|Player")
	FVector GetAttackingGoalLocation() const;

	UFUNCTION(BlueprintPure, Category = "Football|Player")
	FVector GetDefendingGoalLocation() const;

	// --- IFootballBlueprintInterface ---
	virtual bool RequestFootballPass_Implementation(AActor* TargetTeammate, bool bLobbed) override;
	virtual EFootballTeamSide GetFootballTeamSide_Implementation() const override { return TeamSide; }
	virtual EFootballPlayerRole GetFootballPlayerRole_Implementation() const override { return PlayerRole; }
	virtual void ApplyFootballKit_Implementation(const FFootballKitConfig& NewKit) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UFootballUniformComponent> UniformComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UFootballProceduralAnimComponent> ProceduralAnimComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Football|Components")
	TObjectPtr<UMotionWarpingComponent> MotionWarpingComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Identity")
	int32 PlayerId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Identity")
	EFootballTeamSide TeamSide = EFootballTeamSide::Home;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Identity")
	EFootballPlayerRole PlayerRole = EFootballPlayerRole::Striker;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Football|Identity")
	FVector FormationHomeLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Football|State")
	float StaminaNormalized = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|State")
	float CurrentShotCharge = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Football|State")
	bool bIsChargingShot = false;

	UPROPERTY(BlueprintReadOnly, Category = "Football|State")
	bool bIsSprinting = false;

	/** Latest tactical state string displayed on optional AI debug HUD */
	UPROPERTY(BlueprintReadWrite, Category = "Football|Debug")
	FString CurrentTacticalStateLabel = TEXT("Idle");

protected:
	void MoveForward(float Value);
	void MoveRight(float Value);
	void UpdateDribbleControl(float DeltaTime);
	void UpdateStaminaAndSpeed(float DeltaTime);
	AFootballCharacter* FindBestPassTarget() const;

	UPROPERTY(EditAnywhere, Category = "Football|Movement")
	float JogSpeedCmPerSec = 470.0f;

	UPROPERTY(EditAnywhere, Category = "Football|Movement")
	float SprintSpeedCmPerSec = 710.0f;

	UPROPERTY(EditAnywhere, Category = "Football|Movement")
	float BallCaptureRadiusCm = 115.0f;

	float KickCooldownTimer = 0.0f;
	float DiveCooldownTimer = 0.0f;
	mutable TWeakObjectPtr<AFootballBallActor> CachedBall;
};
