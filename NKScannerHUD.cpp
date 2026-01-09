// Fill out your copyright notice in the Description page of Project Settings.

#include "NKScannerHUD.h"
#include "NKScannerCameraActor.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "EngineUtils.h"

ANKScannerHUD::ANKScannerHUD()
{
	// Initialize HUD settings
	HUDXPosition = 20.0f;
	HUDYPosition = 20.0f;
	LineHeight = 20.0f;
	FontScale = 1.0f;
	ScannerCamera = nullptr;
}

void ANKScannerHUD::BeginPlay()
{
	Super::BeginPlay();
	FindScannerCamera();
}

void ANKScannerHUD::DrawHUD()
{
	Super::DrawHUD();

	// Find scanner if not found yet
	if (!ScannerCamera)
	{
		FindScannerCamera();
		if (!ScannerCamera)
		{
			return;  // No scanner found, skip drawing
		}
	}

	// Check if HUD should be shown
	if (!ScannerCamera->bShowDebugHUD)
	{
		return;
	}

	float YPos = HUDYPosition;

	// ===== HEADER =====
	DrawSectionHeader(TEXT("=== SCANNER STATUS ==="), YPos);
	YPos += LineHeight * 0.5f;

	// ===== SCANNER STATE =====
	FString StateStr;
	switch (ScannerCamera->GetScannerState())
	{
		case EScannerState::Idle:
			StateStr = TEXT("IDLE");
			break;
		case EScannerState::Validating:
			StateStr = TEXT("VALIDATING");
			break;
		case EScannerState::Mapping:
			StateStr = TEXT("MAPPING");
			break;
		case EScannerState::Complete:
			StateStr = TEXT("COMPLETE");
			break;
		default:
			StateStr = TEXT("UNKNOWN");
	}

	DrawStatusLine(FString::Printf(TEXT("State: %s"), *StateStr), YPos, GetStateColor(StateStr));
	DrawStatusLine(FString::Printf(TEXT("Enabled: %s | Progress: %.1f%%"), 
		ScannerCamera->IsScannerEnabled() ? TEXT("YES") : TEXT("NO"),
		ScannerCamera->GetScanProgress() * 100.0f), YPos);

	YPos += LineHeight * 0.3f;

	// ===== TARGET FINDER =====
	if (ScannerCamera->IsValidating())
	{
		DrawSectionHeader(TEXT("TARGET FINDER:"), YPos);
		DrawStatusLine(FString::Printf(TEXT("Attempts: %d | Angle: %.1f°"), 
			ScannerCamera->GetValidationAttempts(),
			ScannerCamera->GetCurrentValidationAngle()), YPos, FLinearColor::Yellow);
		DrawStatusLine(TEXT("Status: Searching..."), YPos, FLinearColor::Yellow);
		YPos += LineHeight * 0.3f;
	}

	// ===== LASER INFO =====
	DrawSectionHeader(TEXT("LASER:"), YPos);
	if (ScannerCamera->GetLastShotHit())
	{
		FString HitActorName = ScannerCamera->GetLastHitActor() ? 
			ScannerCamera->GetLastHitActor()->GetName() : TEXT("Unknown");
		
		DrawStatusLine(FString::Printf(TEXT("Hit: %s (%.0f cm)"), 
			*HitActorName, ScannerCamera->GetLastHitDistance()), YPos, FLinearColor::Green);
		
		FVector HitLoc = ScannerCamera->GetLastHitLocation();
		DrawStatusLine(FString::Printf(TEXT("Loc: X=%.0f Y=%.0f Z=%.0f"), 
			HitLoc.X, HitLoc.Y, HitLoc.Z), YPos);
	}
	else
	{
		DrawStatusLine(TEXT("Hit: NONE"), YPos, FLinearColor::Red);
	}
	DrawStatusLine(FString::Printf(TEXT("Range: %.0f cm"), 
		ScannerCamera->GetLaserMaxRange()), YPos);

	YPos += LineHeight * 0.3f;

	// ===== AUDIO STATUS =====
	DrawSectionHeader(TEXT("AUDIO:"), YPos);
	FString AudioStatus = ScannerCamera->IsAudioEnabled() ? TEXT("Enabled") : TEXT("Disabled");
	FLinearColor AudioColor = ScannerCamera->IsAudioEnabled() ? FLinearColor::Green : FLinearColor::Gray;
	
	if (ScannerCamera->IsAudioEnabled())
	{
		FString StateAudio;
		switch (ScannerCamera->GetScannerState())
		{
			case EScannerState::Validating:
				StateAudio = TEXT("(Fast Beeping)");
				break;
			case EScannerState::Mapping:
				StateAudio = TEXT("(Slow Beeping)");
				break;
			default:
				StateAudio = TEXT("(Silent)");
		}
		AudioStatus += TEXT(" ") + StateAudio;
	}
	DrawStatusLine(AudioStatus, YPos, AudioColor);

	YPos += LineHeight * 0.3f;

	// ===== SCAN DATA =====
	if (ScannerCamera->IsCinematicScanActive() || ScannerCamera->GetRecordedDataCount() > 0)
	{
		DrawSectionHeader(TEXT("SCAN DATA:"), YPos);
		DrawStatusLine(FString::Printf(TEXT("Orbit: %.1f° | Points: %d"), 
			ScannerCamera->GetCurrentOrbitAngle(),
			ScannerCamera->GetRecordedDataCount()), YPos, FLinearColor(0.0f, 1.0f, 1.0f));  // Cyan color (R=0, G=1, B=1)
		DrawStatusLine(FString::Printf(TEXT("Time: %.1fs"), 
			ScannerCamera->GetCinematicScanElapsedTime()), YPos);
	}
}

void ANKScannerHUD::DrawStatusLine(const FString& Text, float& YPos, FLinearColor Color)
{
	if (!Canvas)
	{
		return;
	}

	FCanvasTextItem TextItem(FVector2D(HUDXPosition, YPos), FText::FromString(Text), GEngine->GetSmallFont(), Color);
	TextItem.Scale = FVector2D(FontScale, FontScale);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);

	YPos += LineHeight;
}

void ANKScannerHUD::DrawSectionHeader(const FString& Text, float& YPos)
{
	DrawStatusLine(Text, YPos, FLinearColor(1.0f, 0.8f, 0.2f));  // Orange/yellow color
}

FLinearColor ANKScannerHUD::GetStateColor(const FString& State)
{
	if (State == TEXT("IDLE"))
	{
		return FLinearColor::Gray;
	}
	else if (State == TEXT("VALIDATING"))
	{
		return FLinearColor::Yellow;
	}
	else if (State == TEXT("MAPPING"))
	{
		return FLinearColor(0.0f, 1.0f, 1.0f);  // Cyan color (R=0, G=1, B=1)
	}
	else if (State == TEXT("COMPLETE"))
	{
		return FLinearColor::Green;
	}
	
	return FLinearColor::White;
}

void ANKScannerHUD::FindScannerCamera()
{
	if (GetWorld())
	{
		for (TActorIterator<ANKScannerCameraActor> It(GetWorld()); It; ++It)
		{
			ScannerCamera = *It;
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Found scanner camera: %s"), *ScannerCamera->GetName());
			break;  // Use the first one found
		}

		if (!ScannerCamera)
		{
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: No scanner camera found in level!"));
		}
	}
}
