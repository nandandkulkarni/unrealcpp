// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// TARGET FINDER STATE MACHINE - Target Discovery & Acquisition
// ========================================================================

#include "NKScannerCameraActor.h"
#include "CineCameraComponent.h"

void ANKScannerCameraActor::StartTargetFinderState()
{
	LogMessage(TEXT("StartTargetFinderState: Entering target finder state"), true);
	
	ScannerState = EScannerState::Validating;
	bIsValidating = true;
	CurrentValidationAngle = 0.0f;
	ValidationAttempts = 0;
	FirstHitAngle = -1.0f;
	FirstHitResult = FHitResult();
	
	LogMessage(TEXT("StartTargetFinderState: Target finder state initialized - Tick() will handle incremental discovery"), true);
}

void ANKScannerCameraActor::UpdateTargetFinder(float DeltaTime)
{
	// Safety check: If target finder is disabled, exit immediately
	if (!bIsValidating || ScannerState != EScannerState::Validating)
	{
		LogMessage(TEXT("UpdateTargetFinder: Target finder state disabled - exiting"), true);
		return;
	}
	
	// Perform ONE target finder attempt per frame
	ValidationAttempts++;
	
	// Calculate position and rotation for current angle
	FVector TestPosition = CalculateOrbitPosition(CurrentValidationAngle);
	FRotator TestRotation = CalculateLookAtRotation(TestPosition);
	
	SetActorLocation(TestPosition);
	SetActorRotation(TestRotation);
	
	// Perform laser trace
	FHitResult TestHitResult;
	bool bHit = PerformLaserTrace(TestHitResult);
	
	// Draw laser beam (visible to user!)
	if (bShowLaserBeam)
	{
		UCineCameraComponent* CineCamComponent = GetCineCameraComponent();
		if (CineCamComponent)
		{
			FVector Start = CineCamComponent->GetComponentLocation();
			FVector End = bHit ? TestHitResult.Location : Start + (CineCamComponent->GetForwardVector() * LaserMaxRange);
			DrawLaserBeam(Start, End, bHit);
		}
	}
	
	if (bHit)
	{
		// FOUND FIRST HIT!
		FirstHitAngle = CurrentValidationAngle;
		FirstHitResult = TestHitResult;
		OnTargetFinderSuccess();
		return;
	}
	
	// Move to next angle
	CurrentValidationAngle += ValidationAngularStepDegrees;
	
	// Check if we've completed full rotation without finding target
	if (CurrentValidationAngle >= 360.0f)
	{
		OnTargetFinderFailure();
		return;  // Exit immediately after target finder failure
	}
}

void ANKScannerCameraActor::OnTargetFinderSuccess()
{
	LogMessage(FString::Printf(TEXT("STEP 3 SUCCESS: First hit found at angle %.2f° (after %d attempts)"), 
		FirstHitAngle, ValidationAttempts), true);
	LogMessage(FString::Printf(TEXT("  Hit Actor: %s"), 
		*FirstHitResult.GetActor()->GetName()), true);
	LogMessage(FString::Printf(TEXT("  Hit Location: %s"), 
		*FirstHitResult.Location.ToString()), true);
	LogMessage(FString::Printf(TEXT("  Hit Distance: %.2f cm"), 
		FirstHitResult.Distance), true);
	
	// Play target found sound
	if (bEnableAudioFeedback && TargetFoundSound)
	{
		PlayScannerSound(TargetFoundSound);
	}
	
	// Exit target finder state
	bIsValidating = false;
	ScannerState = EScannerState::Mapping;
	
	// ===== STEP 4: Full 360° Terrain Mapping =====
	LogMessage(TEXT("========================================"), true);
	LogMessage(FString::Printf(TEXT("STEP 4: Starting full terrain mapping from angle %.2f°"), 
		FirstHitAngle), true);
	LogMessage(FString::Printf(TEXT("STEP 4: Will record ~%.0f data points"), 
		360.0f / CinematicAngularStepDegrees), true);
	LogMessage(FString::Printf(TEXT("STEP 4: Output: %s"), *CinematicJSONOutputPath), true);
	LogMessage(TEXT("========================================"), true);
	
	// Initialize Step 4 (existing orbit scan logic)
	RecordedScanData.Empty();
	CurrentOrbitAngle = FirstHitAngle;  // Start from validated hit angle!
	CurrentScanFrameNumber = 0;
	CinematicScanElapsedTime = 0.0f;
	CinematicScanUpdateAccumulator = 0.0f;  // Reset update accumulator
	bIsCinematicScanActive = true;  // Activates UpdateCinematicScan() in Tick()
	
	LogMessage(TEXT("STEP 4: Terrain mapping initiated - UpdateCinematicScan() will complete full orbit"), true);
}

void ANKScannerCameraActor::OnTargetFinderFailure()
{
	LogMessage(FString::Printf(TEXT("STEP 3 FAILED: No hit found after 360° rotation at distance %.2f m"), 
		CinematicOrbitRadius / 100.0f), true);
	LogMessage(FString::Printf(TEXT("  Completed %d laser trace attempts"), ValidationAttempts), true);
	
	// Log camera state for debugging
	LogMessage(FString::Printf(TEXT("STEP 3 DEBUG: Camera at %s"), *GetActorLocation().ToString()), true);
	LogMessage(FString::Printf(TEXT("STEP 3 DEBUG: Looking at %s"), *CinematicLookAtTarget.ToString()), true);
	LogMessage(FString::Printf(TEXT("STEP 3 DEBUG: Laser range %.2f m"), LaserMaxRange / 100.0f), true);
	LogMessage(FString::Printf(TEXT("STEP 3 DEBUG: Orbit radius %.2f m"), CinematicOrbitRadius / 100.0f), true);
	
	// Provide diagnostic information
	LogMessage(TEXT("========================================"), true);
	LogMessage(TEXT("TERRAIN MAPPING ABORTED - No target found!"), true);
	LogMessage(TEXT("Possible issues:"), true);
	LogMessage(TEXT("  1. Target has no collision geometry"), true);
	LogMessage(TEXT("     - Check if target has collision enabled"), true);
	LogMessage(TEXT("     - Verify collision complexity is not 'No Collision'"), true);
	LogMessage(TEXT("  2. Wrong laser trace channel"), true);
	LogMessage(TEXT("     - Current channel: LaserTraceChannel"), true);
	LogMessage(TEXT("     - Try changing to ECC_WorldStatic or ECC_Visibility"), true);
	LogMessage(TEXT("  3. Laser range insufficient"), true);
	LogMessage(FString::Printf(TEXT("     - Current range: %.2f m"), LaserMaxRange / 100.0f), true);
	LogMessage(FString::Printf(TEXT("     - Distance to target: %.2f m"), CinematicOrbitRadius / 100.0f), true);
	LogMessage(TEXT("  4. Target is at wrong height"), true);
	LogMessage(FString::Printf(TEXT("     - Scan height: %.2f m (%.0f%%)"), 
		CinematicOrbitHeight / 100.0f, CinematicHeightPercent), true);
	LogMessage(TEXT("     - Try adjusting CinematicHeightPercent"), true);
	LogMessage(TEXT("========================================"), true);
	
	// Play failure sound
	if (bEnableAudioFeedback && ValidationFailedSound)
	{
		PlayScannerSound(ValidationFailedSound);
	}
	
	// Exit to idle state
	bIsValidating = false;
	ScannerState = EScannerState::Idle;
	
	LogMessage(TEXT("OnTargetFinderFailure: Returned to Idle state"), true);
}
