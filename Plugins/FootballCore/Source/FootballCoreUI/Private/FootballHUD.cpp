// Copyright Craquez Studio. All Rights Reserved.

#include "FootballHUD.h"
#include "FootballHUDWidget.h"
#include "FootballMatchGameState.h"
#include "FootballCharacter.h"
#include "FootballBallActor.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

void AFootballHUD::BeginPlay()
{
	Super::BeginPlay();

	if (CustomHUDWidgetClass && GetOwningPlayerController())
	{
		ActiveHUDWidget = CreateWidget<UFootballHUDWidget>(GetOwningPlayerController(), CustomHUDWidgetClass);
		if (ActiveHUDWidget)
		{
			ActiveHUDWidget->AddToViewport(10);
		}
	}
}

void AFootballHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	DrawBroadcastScoreboard();
	DrawLivePitchRadar();
	DrawPlayerGaugesAndControls();
	DrawGoalBanner();
}

void AFootballHUD::DrawBroadcastScoreboard()
{
	UWorld* World = GetWorld();
	AFootballMatchGameState* GS = World ? World->GetGameState<AFootballMatchGameState>() : nullptr;

	const float W = Canvas->SizeX;
	const float BarW = 460.0f;
	const float BarH = 56.0f;
	const float X = (W - BarW) * 0.5f;
	const float Y = 22.0f;

	// Sleek midnight-navy glass panel with neon-emerald & cyan accents
	DrawRect(FLinearColor(0.03f, 0.05f, 0.10f, 0.86f), X, Y, BarW, BarH);
	DrawRect(FLinearColor(0.02f, 0.85f, 0.45f, 1.0f), X, Y, 6.0f, BarH);
	DrawRect(FLinearColor(0.92f, 0.14f, 0.22f, 1.0f), X + BarW - 6.0f, Y, 6.0f, BarH);
	DrawRect(FLinearColor(0.0f, 0.90f, 0.96f, 0.85f), X, Y + BarH - 2.0f, BarW, 2.0f);

	const int32 HomeScore = GS ? GS->HomeScore : 0;
	const int32 AwayScore = GS ? GS->AwayScore : 0;
	const FString Clock = GS ? GS->GetFormattedClock() : TEXT("00:00");
	const FString ModeText = (GS && GS->ActiveMode == EFootballMatchMode::Duel1v1)
		? FString::Printf(TEXT("DUELO 1v1  |  R%d"), GS->DuelRoundIndex)
		: TEXT("JOGO-TREINO 11v11");

	const FString ScoreLine = FString::Printf(TEXT("CRAQUEZ FC   %d  -  %d   RIVAL FC"), HomeScore, AwayScore);
	DrawText(ScoreLine, FLinearColor::White, X + 52.0f, Y + 10.0f, GEngine->GetLargeFont(), 1.15f);
	DrawText(FString::Printf(TEXT("%s   •   %s"), *Clock, *ModeText), FLinearColor(0.0f, 0.95f, 0.88f, 1.0f), X + 110.0f, Y + 34.0f, GEngine->GetSmallFont(), 1.0f);
}

void AFootballHUD::DrawLivePitchRadar()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float RadarW = 220.0f;
	const float RadarH = 142.0f;
	const float RX = Canvas->SizeX - RadarW - 24.0f;
	const float RY = 22.0f;

	// Pitch radar background + center line
	DrawRect(FLinearColor(0.04f, 0.18f, 0.08f, 0.82f), RX, RY, RadarW, RadarH);
	DrawRect(FLinearColor(0.85f, 0.95f, 0.88f, 0.55f), RX + RadarW * 0.5f - 1.0f, RY, 2.0f, RadarH);
	DrawRect(FLinearColor(0.0f, 0.90f, 0.95f, 0.7f), RX, RY, RadarW, 2.0f);
	DrawRect(FLinearColor(0.0f, 0.90f, 0.95f, 0.7f), RX, RY + RadarH - 2.0f, RadarW, 2.0f);

	auto WorldToRadar = [&](const FVector& Pos) -> FVector2D
	{
		const float NormX = FMath::Clamp((Pos.X + 5250.0f) / 10500.0f, 0.0f, 1.0f);
		const float NormY = FMath::Clamp((Pos.Y + 3400.0f) / 6800.0f, 0.0f, 1.0f);
		return FVector2D(RX + NormX * RadarW, RY + NormY * RadarH);
	};

	const APawn* ControlledPawn = GetOwningPawn();
	for (TActorIterator<AFootballCharacter> It(World); It; ++It)
	{
		const AFootballCharacter* P = *It;
		const FVector2D Dot = WorldToRadar(P->GetActorLocation());
		const bool bIsHuman = (P == ControlledPawn);
		const FLinearColor Color = bIsHuman
			? FLinearColor(0.98f, 0.98f, 0.15f, 1.0f)
			: (P->TeamSide == EFootballTeamSide::Home
				? FLinearColor(0.05f, 0.92f, 0.45f, 1.0f)
				: FLinearColor(0.95f, 0.20f, 0.25f, 1.0f));
		const float Sz = bIsHuman ? 8.0f : 6.0f;
		DrawRect(Color, Dot.X - Sz * 0.5f, Dot.Y - Sz * 0.5f, Sz, Sz);
	}

	for (TActorIterator<AFootballBallActor> It(World); It; ++It)
	{
		const FVector2D BDot = WorldToRadar(It->GetActorLocation());
		DrawRect(FLinearColor::White, BDot.X - 4.0f, BDot.Y - 4.0f, 8.0f, 8.0f);
		break;
	}
}

void AFootballHUD::DrawPlayerGaugesAndControls()
{
	const AFootballCharacter* Player = Cast<AFootballCharacter>(GetOwningPawn());
	const float H = Canvas->SizeY;

	// Bottom-left compact player stamina & shot power meter
	const float BX = 26.0f;
	const float BY = H - 106.0f;
	DrawRect(FLinearColor(0.03f, 0.05f, 0.10f, 0.80f), BX, BY, 290.0f, 82.0f);

	const float Stamina = Player ? Player->StaminaNormalized : 1.0f;
	const float Power = Player ? Player->CurrentShotCharge : 0.0f;

	DrawText(TEXT("ESTAMINA"), FLinearColor(0.75f, 0.92f, 0.85f, 1.0f), BX + 12.0f, BY + 10.0f, GEngine->GetSmallFont(), 0.9f);
	DrawRect(FLinearColor(0.14f, 0.18f, 0.22f, 1.0f), BX + 95.0f, BY + 12.0f, 180.0f, 12.0f);
	DrawRect(FLinearColor(0.02f, 0.88f, 0.45f, 1.0f), BX + 95.0f, BY + 12.0f, 180.0f * Stamina, 12.0f);

	DrawText(TEXT("CHUTE / CURVA"), FLinearColor(0.95f, 0.95f, 0.40f, 1.0f), BX + 12.0f, BY + 34.0f, GEngine->GetSmallFont(), 0.9f);
	DrawRect(FLinearColor(0.14f, 0.18f, 0.22f, 1.0f), BX + 95.0f, BY + 36.0f, 180.0f, 12.0f);
	const FLinearColor PowerCol = FMath::Lerp(FLinearColor(0.0f, 0.88f, 0.96f, 1.0f), FLinearColor(1.0f, 0.25f, 0.12f, 1.0f), Power);
	DrawRect(PowerCol, BX + 95.0f, BY + 36.0f, 180.0f * Power, 12.0f);

	DrawText(TEXT("WASD: Mover | ESPACO: Chute Curva | E: Passe | Q: Cavadinha | TAB: Trocar | M: 11v11/1v1 | C: Clima"),
		FLinearColor(0.82f, 0.88f, 0.95f, 0.95f), BX + 12.0f, BY + 58.0f, GEngine->GetSmallFont(), 0.82f);
}

void AFootballHUD::DrawGoalBanner()
{
	UWorld* World = GetWorld();
	AFootballMatchGameState* GS = World ? World->GetGameState<AFootballMatchGameState>() : nullptr;
	if (!GS || GS->BannerDisplayTimer <= 0.0f || GS->StatusBannerText.IsEmpty())
	{
		return;
	}

	const float W = Canvas->SizeX;
	const float BannerW = 520.0f;
	const float BannerH = 54.0f;
	const float X = (W - BannerW) * 0.5f;
	const float Y = 96.0f;

	DrawRect(FLinearColor(0.01f, 0.65f, 0.35f, 0.92f), X, Y, BannerW, BannerH);
	DrawText(GS->StatusBannerText, FLinearColor::White, X + 54.0f, Y + 14.0f, GEngine->GetLargeFont(), 1.25f);
}
