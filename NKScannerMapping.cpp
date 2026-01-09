// Fill out your copyright notice in the Description page of Project Settings.
// ========================================================================
// TERRAIN MAPPING - Step 4: Full 360° Orbit Scan & Data Recording
// ========================================================================

#include "NKScannerCameraActor.h"
#include "CineCameraComponent.h"
#include "Misc/Paths.h"

void ANKScannerCameraActor::StartCinematicScan(AActor* TargetLandscape, float HeightPercent, float DistanceMeters, FString OutputJSONPath)
{
	LogMessage(TEXT("=== TERRAIN MAPPING: 4-STEP WORKFLOW ==="), true);
	
	// ===== STEP 1: Validate Target (Mandatory) =====
	LogMessage(TEXT("STEP 1: Validating target..."), true);
	
	if (!TargetLandscape)
	{
		LogMessage(TEXT("STEP 1 FAILED: Target is NULL - terrain mapping aborted!"), true);
		return;
	}
	
	LogMessage(FString::Printf(TEXT("STEP 1 SUCCESS: Target validated - %s"), 
		*TargetLandscape->GetName()), true);
	
	CinematicTargetLandscape = TargetLandscape;
	CinematicHeightPercent = HeightPercent;
	CinematicDistanceMeters = DistanceMeters;
	CinematicJSONOutputPath = OutputJSONPath.IsEmpty() ? 
		FPaths::ProjectSavedDir() / TEXT("ScanData") / TEXT("CinematicScan.json") : OutputJSONPath;
	
	// ===== STEP 2: Move to Optimal Mapping Position =====
	LogMessage(TEXT("STEP 2: Calculating optimal mapping position..."), true);
	
	FBox TargetBounds = TargetLandscape->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	FVector BoundsExtent = TargetBounds.GetExtent();
	float TargetSize = TargetBounds.GetSize().Size();
	
	LogMessage(FString::Printf(TEXT("STEP 2: Target bounds - Center: %s, Extent: %s, Size: %.2f"), 
		*TargetCenter.ToString(), *BoundsExtent.ToString(), TargetSize), true);
	
	// Calculate optimal distance (1.5x target size for better coverage)
	float OptimalDistance = FMath::Max(TargetSize * 1.5f, DistanceMeters * 100.0f);
	float MappingHeight = TargetBounds.Min.Z + 
		((TargetBounds.Max.Z - TargetBounds.Min.Z) * (HeightPercent / 100.0f));
	
	// Position camera at optimal distance (start at East)
	FVector MappingPosition = TargetCenter;
	MappingPosition.X += OptimalDistance;
	MappingPosition.Z = MappingHeight;
	
	SetActorLocation(MappingPosition);
	
	LogMessage(FString::Printf(TEXT("STEP 2 SUCCESS: Moved to mapping position at %.2f cm from target (height: %.2f cm)"), 
		OptimalDistance, MappingHeight), true);
	
	// Adjust laser range to ensure target is reachable
	float DistanceToTarget = FVector::Dist(MappingPosition, TargetCenter);
	if (DistanceToTarget > LaserMaxRange)
	{
		LaserMaxRange = DistanceToTarget * 2.0f;
		LogMessage(FString::Printf(TEXT("STEP 2: Auto-adjusted laser range to %.2f cm to reach target"), 
			LaserMaxRange), true);
	}
	
	// Setup orbit parameters for validation scan
	CinematicOrbitCenter = TargetCenter;
	CinematicOrbitRadius = OptimalDistance;
	CinematicOrbitHeight = MappingHeight;
	CinematicLookAtTarget = TargetCenter;
	
	// ===== STEP 3: Enter Validation State (Incremental Discovery) =====
	LogMessage(TEXT("STEP 3: Entering validation state for incremental target discovery..."), true);
	LogMessage(FString::Printf(TEXT("STEP 3: Will search using %.2f° steps (~%.0f attempts max)"),
		ValidationAngularStepDegrees, 360.0f / ValidationAngularStepDegrees), true);
	LogMessage(TEXT("STEP 3: Green lasers will be visible during discovery - watch for RED laser hit!"), true);
	
	// Enter validation state - Tick() will handle incremental search
	StartValidationState();
	
	// The UpdateValidation() in Tick() will handle the incremental discovery!
	// Once target is found, it will automatically transition to Step 4
}

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
	
	if (bHit)
	{
		DataPoint.LaserHitLocation = HitResult.Location;
		DataPoint.LaserHitNormal = HitResult.Normal;
		DataPoint.HitDistance = HitResult.Distance;
		DataPoint.HitActorName = HitResult.GetActor() ? HitResult.GetActor()->GetName() : TEXT("None");
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
	
	// Calculate position on circular orbit
	FVector Position;
	Position.X = CinematicOrbitCenter.X + (CinematicOrbitRadius * FMath::Cos(AngleRadians));
	Position.Y = CinematicOrbitCenter.Y + (CinematicOrbitRadius * FMath::Sin(AngleRadians));
	Position.Z = CinematicOrbitHeight;
	
	return Position;
}

FRotator ANKScannerCameraActor::CalculateLookAtRotation(const FVector& CameraPosition)
{
	// Calculate direction from camera to look-at target
	FVector Direction = CinematicLookAtTarget - CameraPosition;
	Direction.Normalize();
	
	// Convert direction to rotator
	return Direction.Rotation();
}
