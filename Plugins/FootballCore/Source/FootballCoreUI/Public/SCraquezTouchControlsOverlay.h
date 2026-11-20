// Copyright Craquez Studio. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class APlayerController;

/**
 * Pure-Slate Android Touch Controls Overlay for Craquez.
 * Renders directly on mobile viewports without requiring any external Blueprint/UMG asset:
 * - Bottom-left ergonomic touch joystick pad (360-degree drag movement)
 * - Bottom-right neon tactile action buttons (CHUTE, PASSE, LANCAR, CORRER, TROCAR)
 * - Top-left quick toggles for 11vs11 / 1vs1 mode and stadium weather
 */
class FOOTBALLCOREUI_API SCraquezTouchControlsOverlay : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCraquezTouchControlsOverlay) {}
		SLATE_ARGUMENT(TWeakObjectPtr<APlayerController>, OwningPlayerController)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

private:
	FReply HandleJoystickTouchStart(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply HandleJoystickTouchMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	FReply HandleJoystickTouchEnd(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void UpdateJoystickVector(const FGeometry& MyGeometry, const FVector2D& ScreenSpacePos);

	class AFootballCharacter* GetControlledPlayer() const;

	TWeakObjectPtr<APlayerController> OwningPC;
	bool bJoystickActive = false;
	FVector2D CurrentMoveAxis = FVector2D::ZeroVector;
};
