// Copyright Craquez Studio. All Rights Reserved.

#include "SCraquezTouchControlsOverlay.h"
#include "FootballCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"

void SCraquezTouchControlsOverlay::Construct(const FArguments& InArgs)
{
	OwningPC = InArgs._OwningPlayerController;

	auto MakeTouchButton = [](const FString& Label, const FLinearColor& Tint, FSimpleDelegate OnPress, FSimpleDelegate OnRelease) -> TSharedRef<SWidget>
	{
		return SNew(SBox)
			.WidthOverride(92.0f)
			.HeightOverride(58.0f)
			.Padding(FMargin(4.0f))
			[
				SNew(SButton)
				.ButtonColorAndOpacity(Tint)
				.ContentPadding(FMargin(6.0f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnPressed_Lambda([OnPress]() { OnPress.ExecuteIfBound(); })
				.OnReleased_Lambda([OnRelease]() { OnRelease.ExecuteIfBound(); })
				[
					SNew(STextBlock)
					.Text(FText::FromString(Label))
					.ColorAndOpacity(FLinearColor::White)
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 12))
				]
			];
	};

	ChildSlot
	[
		SNew(SOverlay)

		// Top-Left Quick Mode & Weather Pill Buttons
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Top)
		.Padding(FMargin(20.0f, 22.0f, 0.0f, 0.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeTouchButton(TEXT("11v11 / 1v1"), FLinearColor(0.02f, 0.65f, 0.42f, 0.88f),
					FSimpleDelegate::CreateLambda([this]()
					{
						if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_ToggleMatchMode(); }
					}),
					FSimpleDelegate())
			]
			+ SHorizontalBox::Slot().AutoWidth()
			[
				MakeTouchButton(TEXT("CLIMA"), FLinearColor(0.06f, 0.48f, 0.72f, 0.88f),
					FSimpleDelegate::CreateLambda([this]()
					{
						if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_CycleWeather(); }
					}),
					FSimpleDelegate())
			]
		]

		// Bottom-Left Interactive Virtual Joystick Pad
		+ SOverlay::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(28.0f, 0.0f, 0.0f, 120.0f))
		[
			SNew(SBox)
			.WidthOverride(156.0f)
			.HeightOverride(156.0f)
			[
				SNew(SBorder)
				.BorderBackgroundColor(FLinearColor(0.04f, 0.12f, 0.18f, 0.62f))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.OnMouseButtonDown(this, &SCraquezTouchControlsOverlay::HandleJoystickTouchStart)
				.OnMouseMove(this, &SCraquezTouchControlsOverlay::HandleJoystickTouchMove)
				.OnMouseButtonUp(this, &SCraquezTouchControlsOverlay::HandleJoystickTouchEnd)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("MOVER")))
					.ColorAndOpacity(FLinearColor(0.0f, 0.92f, 0.88f, 0.85f))
					.Font(FCoreStyle::GetDefaultFontStyle("Bold", 13))
				]
			]
		]

		// Bottom-Right Neon Tactile Action Cluster (CHUTE, PASSE, LANCAR, CORRER, TROCAR)
		+ SOverlay::Slot()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Bottom)
		.Padding(FMargin(0.0f, 0.0f, 24.0f, 28.0f))
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeTouchButton(TEXT("LANCAR"), FLinearColor(0.14f, 0.52f, 0.92f, 0.88f),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_LobPassPressed(); }
						}),
						FSimpleDelegate())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeTouchButton(TEXT("TROCAR"), FLinearColor(0.45f, 0.32f, 0.82f, 0.88f),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_SwitchPlayer(); }
						}),
						FSimpleDelegate())
				]
			]
			+ SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeTouchButton(TEXT("PASSE"), FLinearColor(0.02f, 0.72f, 0.40f, 0.92f),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_PassPressed(); }
						}),
						FSimpleDelegate())
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeTouchButton(TEXT("CHUTE"), FLinearColor(0.92f, 0.22f, 0.14f, 0.95f),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_ShootPressed(); }
						}),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_ShootReleased(); }
						}))
				]
				+ SHorizontalBox::Slot().AutoWidth()
				[
					MakeTouchButton(TEXT("CORRER"), FLinearColor(0.92f, 0.68f, 0.06f, 0.90f),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_SprintStart(); }
						}),
						FSimpleDelegate::CreateLambda([this]()
						{
							if (AFootballCharacter* P = GetControlledPlayer()) { P->Input_SprintStop(); }
						}))
				]
			]
		]
	];
}

void SCraquezTouchControlsOverlay::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bJoystickActive && !CurrentMoveAxis.IsNearlyZero(0.05f))
	{
		if (AFootballCharacter* Player = GetControlledPlayer())
		{
			// Screen Up (-Y in Slate) maps to +X forward along pitch; Screen Right (+X) maps to +Y pitch right
			Player->AddMovementInput(FVector::ForwardVector, CurrentMoveAxis.Y);
			Player->AddMovementInput(FVector::RightVector, CurrentMoveAxis.X);
		}
	}
}

FReply SCraquezTouchControlsOverlay::HandleJoystickTouchStart(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bJoystickActive = true;
	UpdateJoystickVector(MyGeometry, MouseEvent.GetScreenSpacePosition());
	return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SCraquezTouchControlsOverlay::HandleJoystickTouchMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!bJoystickActive)
	{
		return FReply::Unhandled();
	}
	UpdateJoystickVector(MyGeometry, MouseEvent.GetScreenSpacePosition());
	return FReply::Handled();
}

FReply SCraquezTouchControlsOverlay::HandleJoystickTouchEnd(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	bJoystickActive = false;
	CurrentMoveAxis = FVector2D::ZeroVector;
	return FReply::Handled().ReleaseMouseCapture();
}

void SCraquezTouchControlsOverlay::UpdateJoystickVector(const FGeometry& MyGeometry, const FVector2D& ScreenSpacePos)
{
	const FVector2D LocalPos = MyGeometry.AbsoluteToLocal(ScreenSpacePos);
	const FVector2D Center = MyGeometry.GetLocalSize() * 0.5f;
	const float MaxRadius = FMath::Max(Center.X, 1.0f);

	FVector2D Delta = (LocalPos - Center) / MaxRadius;
	if (Delta.Size() > 1.0f)
	{
		Delta.Normalize();
	}

	// Invert Y so dragging upward moves forward (+X) on pitch
	CurrentMoveAxis = FVector2D(Delta.X, -Delta.Y);
}

AFootballCharacter* SCraquezTouchControlsOverlay::GetControlledPlayer() const
{
	if (APlayerController* PC = OwningPC.Get())
	{
		return Cast<AFootballCharacter>(PC->GetPawn());
	}
	return nullptr;
}
