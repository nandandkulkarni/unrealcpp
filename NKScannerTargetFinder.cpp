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
	
	// ===== OPTIMIZED: Only move camera ONCE at start, then rotate in place =====
	if (ValidationAttempts == 1)
	{
		// First attempt: Position camera at starting point (angle 0° = South)
		FVector StartPosition = CalculateOrbitPosition(0.0f);
		SetActorLocation(StartPosition);
		
		LogMessage(FString::Printf(TEXT("UpdateTargetFinder: Positioned camera at starting point: %s"), 
			*StartPosition.ToString()), true);
		LogMessage(TEXT("UpdateTargetFinder: Camera will now rotate in place to sweep 360°"), true);
	}
	
	// ===== FIXED: Directly calculate rotation to sweep around target =====
	// Camera is positioned South of target (at angle 0°)
	// We want to rotate the camera to look at different angles around the target
	
	// Calculate the direction from camera to target center
	FVector CameraToTarget = CinematicLookAtTarget - GetActorLocation();
	CameraToTarget.Z = 0.0f;  // Keep horizontal (ignore Z difference)
	CameraToTarget.Normalize();
	
	// Get the base yaw (direction from camera to target center)
	float BaseYaw = FMath::RadiansToDegrees(FMath::Atan2(CameraToTarget.Y, CameraToTarget.X));
	
	// Add the current validation angle to sweep around
	// This rotates the camera's view around the target
	float SweepYaw = BaseYaw + CurrentValidationAngle;
	
	// Create rotation with the sweep yaw (pitch = 0 for horizontal laser)
	FRotator TestRotation = FRotator(0.0f, SweepYaw, 0.0f);
	
	SetActorRotation(TestRotation);
	
	// ===== DEBUG: Log position to verify camera is NOT moving =====
	if (ValidationAttempts <= 5 || ValidationAttempts % 10 == 0)
	{
		FVector CurrentPos = GetActorLocation();
		LogMessage(FString::Printf(TEXT("UpdateTargetFinder: Attempt %d | Angle %.1f° | Pos: %s | Yaw: %.1f°"), 
			ValidationAttempts, CurrentValidationAngle, *CurrentPos.ToString(), SweepYaw), true);
	}

	
	// Perform laser trace
	FHitResult TestHitResult;
	bool bHit = PerformLaserTrace(TestHitResult);
	
	// Update scanner's hit properties so HUD displays correctly
	UpdateLaserHitProperties(TestHitResult, bHit);
	
	// ===== DRAW PERSISTENT DISCOVERY LASER SHOTS =====
	if (bShowLaserBeam && GetWorld())
	{
		UCineCameraComponent* CineCamComponent = GetCineCameraComponent();
		if (CineCamComponent)
		{
			FVector Start = CineCamComponent->GetComponentLocation();
			FVector End = bHit ? TestHitResult.Location : Start + (CineCamComponent->GetForwardVector() * LaserMaxRange);
			
			// Color-coded persistent lasers (ALL PERMANENT NOW)
			FColor TraceColor;
			float TraceThickness;
			
			if (bHit)
			{
				// HIT: Green, thick
				TraceColor = FColor::Green;
				TraceThickness = 3.0f;
			}
			else
			{
				// MISS: Red, thin
				TraceColor = FColor::Red;
				TraceThickness = 1.0f;
			}
			
			// Draw persistent line (stays until manually cleared)
			DrawDebugLine(GetWorld(), Start, End, TraceColor, true, DebugVisualsLifetime, 0, TraceThickness);
			
			// Draw sphere at hit point for hits
			if (bHit)
			{
				DrawDebugSphere(GetWorld(), End, 15.0f, 8, FColor::Yellow, true, DebugVisualsLifetime);
			}
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
	
	// ===== CHECK AUTO-MAPPING SETTING =====
	if (bAutoStartMapping)
	{
		// Auto-mapping enabled - start mapping immediately
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
	else
	{
		// Auto-mapping disabled - wait for manual start
		ScannerState = EScannerState::Validating;  // Stay in validating state
		
		LogMessage(TEXT("========================================"), true);
		LogMessage(TEXT("STEP 3 COMPLETE: Target found, ready for mapping"), true);
		LogMessage(FString::Printf(TEXT("  First hit at angle: %.2f°"), FirstHitAngle), true);
		LogMessage(TEXT("  Auto-mapping is DISABLED"), true);
		LogMessage(TEXT("  Click 'Start Mapping' button to begin terrain mapping"), true);
		LogMessage(TEXT("========================================"), true);
	}
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
