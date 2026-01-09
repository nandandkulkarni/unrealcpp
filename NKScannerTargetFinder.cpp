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
	LogMessage(FString::Printf(TEXT("STEP 3: No hit at distance %.2f m (after %d attempts in 360° rotation)"), 
		CinematicOrbitRadius / 100.0f, ValidationAttempts), true);
	
	// Log camera state for debugging
	LogMessage(FString::Printf(TEXT("STEP 3 DEBUG: Camera at %s, LookingAt %s, LaserRange %.2f m"),
		*GetActorLocation().ToString(),
		*CinematicLookAtTarget.ToString(),
		LaserMaxRange / 100.0f), true);
	
	// ===== SPIRAL SEARCH: Move outward and try again =====
	const float MaxSearchDistance = 500000.0f;  // 5000m max (5km)
	const float DistanceIncrement = 10000.0f;   // 100m per step
	
	CinematicOrbitRadius += DistanceIncrement;
	
	if (CinematicOrbitRadius > MaxSearchDistance)
	{
		// Exhausted search area - abort
		LogMessage(FString::Printf(TEXT("STEP 3 FAILED: Target not found after searching up to %.2f m"), 
			MaxSearchDistance / 100.0f), true);
		LogMessage(TEXT("  Possible issues:"), true);
		LogMessage(TEXT("  - Target has no collision geometry"), true);
		LogMessage(TEXT("  - Wrong laser trace channel (check LaserTraceChannel)"), true);
		LogMessage(TEXT("  - Target is beyond 5km search radius"), true);
		LogMessage(FString::Printf(TEXT("  - Camera forward vector: %s"), 
			*GetCineCameraComponent()->GetForwardVector().ToString()), true);
		LogMessage(TEXT("TERRAIN MAPPING ABORTED - Cannot map unreachable target!"), true);
		
		// Play failure sound
		if (bEnableAudioFeedback && ValidationFailedSound)
		{
			PlayScannerSound(ValidationFailedSound);
		}
		
		// Exit to idle
		bIsValidating = false;
		ScannerState = EScannerState::Idle;
		return;
	}
	
	// Continue search at new distance
	LogMessage(FString::Printf(TEXT("STEP 3: Moving outward to %.2f m and retrying..."), 
		CinematicOrbitRadius / 100.0f), true);
	
	// Reset validation for new distance
	CurrentValidationAngle = 0.0f;
	ValidationAttempts = 0;
	
	// Update camera position to new radius
	FVector NewPosition = CinematicOrbitCenter;
	NewPosition.Y -= CinematicOrbitRadius;  // Move south
	SetActorLocation(NewPosition);
	
	LogMessage(FString::Printf(TEXT("STEP 3: New camera position: %s"), *NewPosition.ToString()), true);
	
	// Ensure laser range is sufficient for new distance
	float DistanceToTarget = CinematicOrbitRadius;
	if (DistanceToTarget > LaserMaxRange)
	{
		LaserMaxRange = DistanceToTarget * 2.0f;
		LogMessage(FString::Printf(TEXT("STEP 3: Auto-adjusted laser range to %.2f m"), 
			LaserMaxRange / 100.0f), true);
	}
	
	// Continue validating (don't exit) - next Tick will test new distance
}
