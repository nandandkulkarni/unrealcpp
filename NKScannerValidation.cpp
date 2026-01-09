// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// VALIDATION STATE MACHINE - Step 3: Incremental Target Discovery
// ========================================================================

#include "NKScannerCameraActor.h"
#include "CineCameraComponent.h"
#include "DrawDebugHelpers.h"

void ANKScannerCameraActor::StartValidationState()
{
	LogMessage(TEXT("StartValidationState: Entering validation state"), true);
	
	ScannerState = EScannerState::Validating;
	bIsValidating = true;
	CurrentValidationAngle = 0.0f;
	ValidationAttempts = 0;
	FirstHitAngle = -1.0f;
	FirstHitResult = FHitResult();
	
	LogMessage(TEXT("StartValidationState: Validation state initialized - Tick() will handle incremental discovery"), true);
}

void ANKScannerCameraActor::UpdateValidation(float DeltaTime)
{
	// Safety check: If validation is disabled, exit immediately
	if (!bIsValidating || ScannerState != EScannerState::Validating)
	{
		LogMessage(TEXT("UpdateValidation: Validation state disabled - exiting"), true);
		return;
	}
	
	// Perform ONE validation attempt per frame
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
		OnValidationSuccess();
		return;
	}
	
	// Move to next angle
	CurrentValidationAngle += ValidationAngularStepDegrees;
	
	// Check if we've completed full rotation without finding target
	if (CurrentValidationAngle >= 360.0f)
	{
		OnValidationFailure();
		return;  // Exit immediately after validation failure
	}
}

void ANKScannerCameraActor::OnValidationSuccess()
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
	
	// Exit validation state
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
	CurrentOrbitAngle = FirstHitAngle;  // ? Start from validated hit angle!
	CurrentScanFrameNumber = 0;
	CinematicScanElapsedTime = 0.0f;
	CinematicScanUpdateAccumulator = 0.0f;  // Reset update accumulator
	bIsCinematicScanActive = true;  // ? Activates UpdateCinematicScan() in Tick()
	
	LogMessage(TEXT("STEP 4: Terrain mapping initiated - UpdateCinematicScan() will complete full orbit"), true);
}

void ANKScannerCameraActor::OnValidationFailure()
{
	LogMessage(FString::Printf(TEXT("STEP 3 FAILED: No hit found in full 360° rotation (%d attempts)"), 
		ValidationAttempts), true);
	LogMessage(TEXT("  Possible issues:"), true);
	LogMessage(TEXT("  - Target is out of laser range (increase LaserMaxRange)"), true);
	LogMessage(TEXT("  - Target has no collision geometry"), true);
	LogMessage(TEXT("  - Wrong laser trace channel (check LaserTraceChannel)"), true);
	LogMessage(TEXT("  - Target is too small or occluded"), true);
	LogMessage(FString::Printf(TEXT("  - Try reducing ValidationAngularStepDegrees (currently %.2f°) for more attempts"), 
		ValidationAngularStepDegrees), true);
	LogMessage(TEXT("TERRAIN MAPPING ABORTED - Cannot map unreachable target!"), true);
	
	// Play validation failed sound
	if (bEnableAudioFeedback && ValidationFailedSound)
	{
		PlayScannerSound(ValidationFailedSound);
	}
	
	// Exit validation state
	bIsValidating = false;
	ScannerState = EScannerState::Idle;
}
