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
	
	// Initialize rotation history
	MaxRotationHistory = 10;
	RotationUpdateTimer = 0.0f;
	RotationUpdateInterval = 0.5f;  // Update every 0.5 seconds
	RotationSerialNumber = 0;  // Start at 0
	LastRecordedRotation = FRotator::ZeroRotator;
	bUpdateOnMovement = true;  // TRUE = update on every camera movement, FALSE = update on timer
	
	// Performance settings
	FramesSinceLastHUDUpdate = 0;
	HUDUpdateFrequency = 1;  // 1 = every frame (no skipping), 2 = every other frame, 3 = every 3rd frame
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
	
	// ===== CAMERA POSITION =====
	FVector CamPos = ScannerCamera->GetCameraPosition();
	DrawSectionHeader(TEXT("CAMERA:"), YPos);
	DrawStatusLine(FString::Printf(TEXT("Pos: X=%.1f Y=%.1f Z=%.1f"), 
		CamPos.X, CamPos.Y, CamPos.Z), YPos, FLinearColor(0.8f, 0.8f, 1.0f));
	DrawStatusLine(FString::Printf(TEXT("     (%.2fm, %.2fm, %.2fm)"), 
		CamPos.X/100.0f, CamPos.Y/100.0f, CamPos.Z/100.0f), YPos, FLinearColor(0.6f, 0.6f, 0.8f));
	
	// Current rotation
	FRotator CamRot = ScannerCamera->GetCameraRotation();
	DrawStatusLine(FString::Printf(TEXT("Rot: P=%.1f° Y=%.1f° R=%.1f°"), 
		CamRot.Pitch, CamRot.Yaw, CamRot.Roll), YPos, FLinearColor(1.0f, 0.9f, 0.7f));
	
	YPos += LineHeight * 0.3f;

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
	
	// Show discovery hit (first hit found during validation)
	if (ScannerCamera->GetScannerState() == EScannerState::Mapping || 
	    ScannerCamera->GetScannerState() == EScannerState::Complete)
	{
		DrawSectionHeader(TEXT("  Discovery Hit (First):"), YPos);
		FHitResult DiscoveryHit = ScannerCamera->GetFirstHitResult();
		if (DiscoveryHit.bBlockingHit)
		{
			FString HitActorName = DiscoveryHit.GetActor() ? 
				DiscoveryHit.GetActor()->GetName() : TEXT("Unknown");
			
			DrawStatusLine(FString::Printf(TEXT("    Actor: %s"), *HitActorName), YPos, FLinearColor::Green);
			DrawStatusLine(FString::Printf(TEXT("    Angle: %.1f°"), ScannerCamera->GetFirstHitAngle()), YPos);
			
			FVector HitLoc = DiscoveryHit.Location;
			DrawStatusLine(FString::Printf(TEXT("    Loc: X=%.0f Y=%.0f Z=%.0f"), 
				HitLoc.X, HitLoc.Y, HitLoc.Z), YPos);
			DrawStatusLine(FString::Printf(TEXT("    Dist: %.0f cm"), DiscoveryHit.Distance), YPos);
		}
	}
	
	// Show current orbital hit (most recent hit during mapping)
	DrawSectionHeader(TEXT("  Current Orbital Hit:"), YPos);
	if (ScannerCamera->GetLastShotHit())
	{
		FString HitActorName = ScannerCamera->GetLastHitActor() ? 
			ScannerCamera->GetLastHitActor()->GetName() : TEXT("Unknown");
		
		DrawStatusLine(FString::Printf(TEXT("    Actor: %s"), *HitActorName), YPos, FLinearColor::Cyan);
		
		FVector HitLoc = ScannerCamera->GetLastHitLocation();
		DrawStatusLine(FString::Printf(TEXT("    Loc: X=%.0f Y=%.0f Z=%.0f"), 
			HitLoc.X, HitLoc.Y, HitLoc.Z), YPos);
		DrawStatusLine(FString::Printf(TEXT("    Dist: %.0f cm"), ScannerCamera->GetLastHitDistance()), YPos);
	}
	else
	{
		DrawStatusLine(TEXT("    No hit"), YPos, FLinearColor::Red);
	}
	
	DrawStatusLine(FString::Printf(TEXT("Range: %.0f cm"), 
		ScannerCamera->GetLaserMaxRange()), YPos);

	YPos += LineHeight * 0.3f;
	
	// ===== TARGET INFO =====
	AActor* TargetActor = ScannerCamera->GetCinematicTargetLandscape();
	if (TargetActor)
	{
		DrawSectionHeader(TEXT("TARGET:"), YPos);
		
		// Display target label (the name shown in the outliner, like "MyCube")
		FString TargetLabel = TargetActor->GetActorLabel();
		DrawStatusLine(FString::Printf(TEXT("Name: %s"), *TargetLabel), YPos, FLinearColor(1.0f, 1.0f, 0.5f));
		
		// Get target bounds to show Z height range
		FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
		float MinHeightMeters = TargetBounds.Min.Z / 100.0f;
		float MaxHeightMeters = TargetBounds.Max.Z / 100.0f;
		
		DrawStatusLine(FString::Printf(TEXT("Z Range: %.2fm to %.2fm"), MinHeightMeters, MaxHeightMeters), 
			YPos, FLinearColor(0.7f, 0.9f, 1.0f));
		
		// Show the scan height (what percentage we're scanning at)
		float ScanHeightMeters = ScannerCamera->GetCinematicOrbitHeight() / 100.0f;
		DrawStatusLine(FString::Printf(TEXT("Scan Height: %.2fm (%.0f%%)"), 
			ScanHeightMeters, ScannerCamera->GetCinematicHeightPercent()), 
			YPos, FLinearColor(0.5f, 1.0f, 0.5f));
		
		YPos += LineHeight * 0.3f;
	}

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
		
		YPos += LineHeight * 0.3f;
	}
	
	// ===== ROTATION HISTORY (Last 10) =====
	if (RotationHistory.Num() > 0)
	{
		DrawSectionHeader(TEXT("ROTATION HISTORY (Last 10):"), YPos);
		
		// Draw from newest to oldest
		for (int32 i = RotationHistory.Num() - 1; i >= 0; i--)
		{
			const FRotator& Rot = RotationHistory[i];
			int32 SerialNum = RotationSerialNumbers[i];
			
			// Color gradient from bright (newest) to dim (oldest)
			float Alpha = (float)(i + 1) / (float)RotationHistory.Num();
			FLinearColor HistoryColor = FLinearColor(0.7f * Alpha, 0.9f * Alpha, 0.7f * Alpha);
			
			// Format: #00001: P=0.2° Y=90.5° R=0.0°
			DrawStatusLine(FString::Printf(TEXT("#%05d: P=%6.1f° Y=%6.1f° R=%6.1f°"), 
				SerialNum,
				Rot.Pitch, Rot.Yaw, Rot.Roll), 
				YPos, HistoryColor);
		}
	}
	
	// Update rotation history
	UpdateRotationHistory(ScannerCamera->GetCameraRotation(), GetWorld()->GetDeltaSeconds());
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

void ANKScannerHUD::UpdateRotationHistory(const FRotator& CurrentRotation, float DeltaTime)
{
	if (bUpdateOnMovement)
	{
		// ===== MODE 1: Update on EVERY camera movement =====
		// Check if rotation has changed since last record
		const float RotationThreshold = 0.05f;  // Degrees - very sensitive to catch small changes
		
		bool bRotationChanged = 
			!FMath::IsNearlyEqual(CurrentRotation.Pitch, LastRecordedRotation.Pitch, RotationThreshold) ||
			!FMath::IsNearlyEqual(CurrentRotation.Yaw, LastRecordedRotation.Yaw, RotationThreshold) ||
			!FMath::IsNearlyEqual(CurrentRotation.Roll, LastRecordedRotation.Roll, RotationThreshold);
		
		if (bRotationChanged)
		{
			// Rotation changed - record it!
			UE_LOG(LogTemp, Warning, TEXT("HUD: Rotation changed! Yaw: %.2f -> %.2f (diff: %.2f)"), 
				LastRecordedRotation.Yaw, CurrentRotation.Yaw, 
				FMath::Abs(CurrentRotation.Yaw - LastRecordedRotation.Yaw));
			
			LastRecordedRotation = CurrentRotation;
			
			// Increment serial number
			RotationSerialNumber++;
			
			// Add new rotation to history
			RotationHistory.Add(CurrentRotation);
			RotationSerialNumbers.Add(RotationSerialNumber);
			
			// Keep only the last MaxRotationHistory entries
			if (RotationHistory.Num() > MaxRotationHistory)
			{
				RotationHistory.RemoveAt(0);
				RotationSerialNumbers.RemoveAt(0);
			}
		}
	}
	else
	{
		// ===== MODE 2: Update on TIMER (original behavior) =====
		RotationUpdateTimer += DeltaTime;
		
		// Use faster update rate during VALIDATING state (discovery phase)
		float CurrentUpdateInterval = RotationUpdateInterval;
		if (ScannerCamera && ScannerCamera->GetScannerState() == EScannerState::Validating)
		{
			CurrentUpdateInterval = 0.1f;  // Update every 100ms during discovery
		}
		
		// Only update when timer reaches the interval
		if (RotationUpdateTimer >= CurrentUpdateInterval)
		{
			RotationUpdateTimer = 0.0f;
			
			UE_LOG(LogTemp, Warning, TEXT("HUD: Timer update! Yaw: %.2f"), CurrentRotation.Yaw);
			
			// Increment serial number
			RotationSerialNumber++;
			
			// Add new rotation to history
			RotationHistory.Add(CurrentRotation);
			RotationSerialNumbers.Add(RotationSerialNumber);
			
			// Keep only the last MaxRotationHistory entries
			if (RotationHistory.Num() > MaxRotationHistory)
			{
				RotationHistory.RemoveAt(0);
				RotationSerialNumbers.RemoveAt(0);
			}
		}
	}
}
