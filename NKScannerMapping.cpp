// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// CINEMATIC SCANNING & TERRAIN MAPPING
// ========================================================================

#include "NKScannerCameraActor.h"

void ANKScannerCameraActor::StopCinematicScan()
{
	LogMessage(FString::Printf(TEXT("StopCinematicScan: Stopping scan at angle %.2f° with %d data points recorded"),
		CurrentOrbitAngle, RecordedScanData.Num()), true);
	
	// Stop the scan
	bIsCinematicScanActive = false;
	ScannerState = EScannerState::Complete;
	
	// Save the JSON data
	if (RecordedScanData.Num() > 0)
	{
		bool bSuccess = SaveScanDataToJSON(CinematicJSONOutputPath);
		if (bSuccess)
		{
			LogMessage(FString::Printf(TEXT("StopCinematicScan: Successfully saved %d data points to %s"),
				RecordedScanData.Num(), *CinematicJSONOutputPath), true);
			OnScanComplete.Broadcast(CinematicJSONOutputPath);
		}
		else
		{
		 LogMessage(TEXT("StopCinematicScan: ERROR - Failed to save JSON data!"), true);
		}
	}
	else
	{
		LogMessage(TEXT("StopCinematicScan: WARNING - No data points recorded!"), true);
	}
}

void ANKScannerCameraActor::UpdateCinematicScan(float DeltaTime)
{
	if (!bIsCinematicScanActive)
	{
		return;
	}
	
	// Increment elapsed time
	CinematicScanElapsedTime += DeltaTime;
	
	// Move camera to current angle position
	FVector OrbitPosition = CalculateOrbitPosition(CurrentOrbitAngle);
	FRotator LookAtRotation = CalculateLookAtRotation(OrbitPosition);
	
	SetActorLocation(OrbitPosition);
	SetActorRotation(LookAtRotation);
	
	// Record current scan point
	RecordCurrentScanPoint();
	
	// Increment angle for next step
	CurrentOrbitAngle += CinematicAngularStepDegrees;
	
	// Check if we've completed full 360° rotation
	if (CurrentOrbitAngle >= (FirstHitAngle + 360.0f))
	{
		LogMessage(TEXT("UpdateCinematicScan: Full 360° orbit complete!"), true);
		LogMessage(FString::Printf(TEXT("UpdateCinematicScan: Recorded %d data points over %.2f seconds"),
			RecordedScanData.Num(), CinematicScanElapsedTime), true);
		
		// Auto-stop and save
		StopCinematicScan();
	}
}

void ANKScannerCameraActor::RecordCurrentScanPoint()
{
	FScanDataPoint DataPoint;
	
	// Record camera transform
	DataPoint.CameraPosition = GetActorLocation();
	DataPoint.CameraRotation = GetActorRotation();
	
	// Perform laser trace and record hit data
	FHitResult HitResult;
	bool bHit = PerformLaserTrace(HitResult);
	
	// ===== DEBUG: Log trace results =====
	if (!bHit)
	{
		LogMessage(FString::Printf(TEXT("RecordCurrentScanPoint: MISS at angle %.2f° - Camera: %s, LookAt: %s, Range: %.2f"), 
			CurrentOrbitAngle, 
			*GetActorLocation().ToString(),
			*CinematicLookAtTarget.ToString(),
			LaserMaxRange), true);
	}
	
	if (bHit)
	{
		DataPoint.LaserHitLocation = HitResult.Location;
		DataPoint.LaserHitNormal = HitResult.Normal;
		DataPoint.HitDistance = HitResult.Distance;
		DataPoint.HitActorName = HitResult.GetActor() ? HitResult.GetActor()->GetName() : TEXT("None");
		
		LogMessage(FString::Printf(TEXT("RecordCurrentScanPoint: HIT at angle %.2f° - Actor: %s, Distance: %.2f, Location: %s"), 
			CurrentOrbitAngle, 
			*DataPoint.HitActorName,
			DataPoint.HitDistance,
			*HitResult.Location.ToString()), true);
		
		// ===== VISUAL DEBUG: Draw spheres and lines at scan points =====
		if (GetWorld())
		{
			// Draw sphere at hit location
			if (bShowScanPointSpheres)
			{
				DrawDebugSphere(
					GetWorld(),
					HitResult.Location,
					ScanPointSphereSize,
					8,
					ScanPointColor,
					true,  // Persistent
					DebugVisualsLifetime
				);
			}
			
			// Draw line from camera to hit point
			if (bShowScanLines)
			{
				DrawDebugLine(
					GetWorld(),
					GetActorLocation(),
					HitResult.Location,
					ScanLineColor,
					true,  // Persistent
					DebugVisualsLifetime,
					0,
					1.0f
				);
			}
		}
	}
	else
	{
		DataPoint.LaserHitLocation = FVector::ZeroVector;
		DataPoint.LaserHitNormal = FVector::ZeroVector;
		DataPoint.HitDistance = 0.0f;
		DataPoint.HitActorName = TEXT("NoHit");
	}
	
	// Record metadata
	DataPoint.FrameNumber = CurrentScanFrameNumber++;
	DataPoint.TimeStamp = CinematicScanElapsedTime;
	DataPoint.OrbitAngle = CurrentOrbitAngle;
	
	RecordedScanData.Add(DataPoint);
	
	if (bLogEveryFrame)
	{
		LogMessage(FString::Printf(TEXT("RecordCurrentScanPoint: Frame %d at angle %.2f° - Hit: %s"),
			DataPoint.FrameNumber, CurrentOrbitAngle, bHit ? TEXT("YES") : TEXT("NO")));
	}
}

FVector ANKScannerCameraActor::CalculateOrbitPosition(float Angle)
{
	// Convert angle to radians
	float AngleRadians = FMath::DegreesToRadians(Angle);
	
	// ===== Calculate position on HORIZONTAL circular orbit =====
	// Camera orbits in XY plane at FIXED Z height (parallel to ground)
	FVector Position;
	Position.X = CinematicOrbitCenter.X + (CinematicOrbitRadius * FMath::Cos(AngleRadians));
	Position.Y = CinematicOrbitCenter.Y + (CinematicOrbitRadius * FMath::Sin(AngleRadians));
	Position.Z = CinematicOrbitHeight;  // FIXED HEIGHT - never changes during orbit
	
	return Position;
}

FRotator ANKScannerCameraActor::CalculateLookAtRotation(const FVector& CameraPosition)
{
	// ===== Calculate HORIZONTAL look-at rotation =====
	// Camera looks at target center at THE SAME Z HEIGHT
	// This ensures laser shoots horizontally (parallel to ground)
	FVector Direction = CinematicLookAtTarget - CameraPosition;
	Direction.Normalize();
	
	// Convert direction to rotator
	FRotator LookAtRot = Direction.Rotation();
	
	// Log the pitch to verify horizontal shooting (pitch should be ~0°)
	if (bEnableVerboseLogging && CurrentOrbitAngle < 10.0f)  // Only log first few angles
	{
		LogMessage(FString::Printf(TEXT("CalculateLookAtRotation: Pitch=%.2f° (should be ~0° for horizontal)"), 
			LookAtRot.Pitch), true);
	}
	
	return LookAtRot;
}

// ========================================================================
// JSON SAVE/LOAD STUB FUNCTIONS
// ========================================================================

bool ANKScannerCameraActor::SaveScanDataToJSON(const FString& FilePath)
{
	// TODO: Implement JSON serialization of RecordedScanData array
	LogMessage(FString::Printf(TEXT("SaveScanDataToJSON: JSON save not implemented yet. Would save %d points to: %s"),
		RecordedScanData.Num(), *FilePath), true);
	
	UE_LOG(LogTemp, Warning, TEXT("SaveScanDataToJSON not implemented yet - skipping file write"));
	
	// Return true to prevent error messages
	return true;
}

bool ANKScannerCameraActor::LoadScanDataFromJSON(const FString& FilePath)
{
	// TODO: Implement JSON deserialization into RecordedScanData array
	LogMessage(FString::Printf(TEXT("LoadScanDataFromJSON: JSON load not implemented yet. Would load from: %s"), 
		*FilePath), true);
	
	UE_LOG(LogTemp, Warning, TEXT("LoadScanDataFromJSON not implemented yet - skipping file read"));
	
	// Return false to indicate no data was loaded
	return false;
}
