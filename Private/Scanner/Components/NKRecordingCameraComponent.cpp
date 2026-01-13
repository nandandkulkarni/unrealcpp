// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Components/NKRecordingCameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"

UNKRecordingCameraComponent::UNKRecordingCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;  // Only tick when playing
}

void UNKRecordingCameraComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNKRecordingCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bIsPlaying || bIsPaused || MappingHitPoints.Num() < 2)
	{
		return;
	}
	
	// 1. Advance smoothly along path
	CurrentDistance += (RecordingPlaybackSpeed * 100.0f) * DeltaTime;  // Convert m/s to cm/s
	
	// 2. Handle looping or completion
	if (CurrentDistance >= TotalPathLength)
	{
		if (bRecordingLoopPlayback)
		{
			CurrentDistance = FMath::Fmod(CurrentDistance, TotalPathLength);
			UE_LOG(LogTemp, Log, TEXT("Recording playback looped"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Recording playback complete"));
			StopPlayback();
			return;
		}
	}
	
	// 3. Get current position on orbit
	FVector OrbitPoint = GetInterpolatedPosition(CurrentDistance);
	
	// 4. Get tangent direction
	FVector TangentDirection = GetTangentAtDistance(CurrentDistance);
	
	// 5. Calculate perpendicular camera position
	FVector CameraPosition = CalculateCameraPosition(OrbitPoint, TangentDirection);
	
	// 6. Calculate camera rotation based on look mode
	FRotator CameraRotation = CalculateCameraRotation(CameraPosition, OrbitPoint);
	
	// 7. Apply to owner (camera actor)
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorLocation(CameraPosition);
		Owner->SetActorRotation(CameraRotation);
	}
	
	// 8. Draw debug visualization
	DrawDebugVisualization(CameraPosition, OrbitPoint);
	
	// 9. Movement logging (if enabled)
	if (bRecordingEnableMovementLogging)
	{
		TimeSinceLastMovementLog += DeltaTime;
		
		if (TimeSinceLastMovementLog >= RecordingMovementLogInterval)
		{
			TimeSinceLastMovementLog = 0.0f;
			
			float ProgressPercent = GetProgress() * 100.0f;
			float DistanceMeters = CurrentDistance / 100.0f;
			float PathLengthMeters = TotalPathLength / 100.0f;
			
			UE_LOG(LogTemp, Log, TEXT("???????????????????????????????????????????????????????"));
			UE_LOG(LogTemp, Log, TEXT("? ?? RECORDING CAMERA MOVEMENT                        ?"));
			UE_LOG(LogTemp, Log, TEXT("???????????????????????????????????????????????????????"));
			UE_LOG(LogTemp, Log, TEXT("? Progress: %.1f%% (%.2fm / %.2fm)"), 
				ProgressPercent, DistanceMeters, PathLengthMeters);
			UE_LOG(LogTemp, Log, TEXT("? Speed: %.1f m/s"), RecordingPlaybackSpeed);
			UE_LOG(LogTemp, Log, TEXT("? Camera Pos (m): (%.2f, %.2f, %.2f)"), 
				CameraPosition.X/100.0f, CameraPosition.Y/100.0f, CameraPosition.Z/100.0f);
			UE_LOG(LogTemp, Log, TEXT("? Orbit Point (m): (%.2f, %.2f, %.2f)"), 
				OrbitPoint.X/100.0f, OrbitPoint.Y/100.0f, OrbitPoint.Z/100.0f);
			UE_LOG(LogTemp, Log, TEXT("? Look Mode: %s"), 
				RecordingLookMode == ERecordingLookMode::Perpendicular ? TEXT("Perpendicular") :
				RecordingLookMode == ERecordingLookMode::Center ? TEXT("Center") : TEXT("Look-Ahead"));
			UE_LOG(LogTemp, Log, TEXT("? Camera Rot: P=%.1f° Y=%.1f° R=%.1f°"), 
				CameraRotation.Pitch, CameraRotation.Yaw, CameraRotation.Roll);
			UE_LOG(LogTemp, Log, TEXT("???????????????????????????????????????????????????????"));
		}
	}
}

void UNKRecordingCameraComponent::StartPlayback(const TArray<FVector>& InMappingHitPoints)
{
	if (InMappingHitPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("RecordingCamera: Need at least 2 hit points for playback!"));
		return;
	}
	
	// Store hit points
	MappingHitPoints = InMappingHitPoints;
	
	// Calculate path metrics
	TotalPathLength = CalculateTotalPathLength();
	OrbitCenter = CalculateOrbitCenter();
	
	// Reset state
	CurrentDistance = 0.0f;
	bIsPlaying = true;
	bIsPaused = false;
	TimeSinceLastMovementLog = 0.0f;
	SetComponentTickEnabled(true);
	
	UE_LOG(LogTemp, Warning, TEXT("???????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("?? RECORDING CAMERA PLAYBACK STARTED"));
	UE_LOG(LogTemp, Warning, TEXT("???????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("  Hit Points: %d"), MappingHitPoints.Num());
	UE_LOG(LogTemp, Warning, TEXT("  Path Length: %.2f meters"), TotalPathLength / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Center: (%.2f, %.2f, %.2f) m"), 
		OrbitCenter.X/100.0f, OrbitCenter.Y/100.0f, OrbitCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Playback Speed: %.1f m/s"), RecordingPlaybackSpeed);
	UE_LOG(LogTemp, Warning, TEXT("  Camera Offset: %.1f m"), RecordingOffsetDistanceCm / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Look Mode: %s"), 
		RecordingLookMode == ERecordingLookMode::Perpendicular ? TEXT("Perpendicular") :
		RecordingLookMode == ERecordingLookMode::Center ? TEXT("Center") : TEXT("Look-Ahead"));
	UE_LOG(LogTemp, Warning, TEXT("  Loop Playback: %s"), bRecordingLoopPlayback ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Warning, TEXT("  Estimated Duration: %.1f seconds"), 
		TotalPathLength / (RecordingPlaybackSpeed * 100.0f));
	UE_LOG(LogTemp, Warning, TEXT("???????????????????????????????????????????????????????"));
}

void UNKRecordingCameraComponent::StopPlayback()
{
	bIsPlaying = false;
	bIsPaused = false;
	SetComponentTickEnabled(false);
	
	UE_LOG(LogTemp, Warning, TEXT("?? Recording camera playback stopped"));
}

void UNKRecordingCameraComponent::SetPaused(bool bPause)
{
	if (!bIsPlaying)
	{
		UE_LOG(LogTemp, Warning, TEXT("RecordingCamera: Cannot pause - not playing"));
		return;
	}
	
	bIsPaused = bPause;
	UE_LOG(LogTemp, Log, TEXT("Recording playback %s"), bIsPaused ? TEXT("PAUSED") : TEXT("RESUMED"));
}

float UNKRecordingCameraComponent::GetProgress() const
{
	if (TotalPathLength <= 0.0f)
		return 0.0f;
	
	return FMath::Clamp(CurrentDistance / TotalPathLength, 0.0f, 1.0f);
}

float UNKRecordingCameraComponent::CalculateTotalPathLength() const
{
	if (MappingHitPoints.Num() < 2)
		return 0.0f;
	
	float TotalLength = 0.0f;
	
	for (int32 i = 0; i < MappingHitPoints.Num(); i++)
	{
		FVector Start = MappingHitPoints[i];
		FVector End = MappingHitPoints[(i + 1) % MappingHitPoints.Num()];
		
		TotalLength += FVector::Dist(Start, End);
	}
	
	return TotalLength;
}

FVector UNKRecordingCameraComponent::CalculateOrbitCenter() const
{
	if (MappingHitPoints.Num() == 0)
		return FVector::ZeroVector;
	
	// Calculate average position (2D - XY plane only, preserve original Z height)
	FVector Sum = FVector::ZeroVector;
	for (const FVector& Point : MappingHitPoints)
	{
		Sum.X += Point.X;
		Sum.Y += Point.Y;
		// Don't average Z - use first point's Z as reference height
	}
	
	// Average X and Y, use first point's Z
	FVector Center;
	Center.X = Sum.X / MappingHitPoints.Num();
	Center.Y = Sum.Y / MappingHitPoints.Num();
	Center.Z = MappingHitPoints[0].Z;  // Use same height as hit points
	
	UE_LOG(LogTemp, Warning, TEXT("?? ORBIT CENTER FIX ACTIVE: Using 2D averaging (XY only, Z preserved)"));
	UE_LOG(LogTemp, Warning, TEXT("   First hit point Z: %.2f m"), MappingHitPoints[0].Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("   Last hit point Z: %.2f m"), MappingHitPoints.Last().Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("   Calculated center: (%.2f, %.2f, %.2f) m"), 
		Center.X/100.0f, Center.Y/100.0f, Center.Z/100.0f);
	
	return Center;
}

FVector UNKRecordingCameraComponent::GetInterpolatedPosition(float DistanceAlongPath) const
{
	if (MappingHitPoints.Num() < 2)
		return FVector::ZeroVector;
	
	// Wrap distance to path length
	float WrappedDistance = FMath::Fmod(DistanceAlongPath, TotalPathLength);
	if (WrappedDistance < 0.0f)
		WrappedDistance += TotalPathLength;
	
	// Find which segment we're in
	float AccumulatedDistance = 0.0f;
	
	for (int32 i = 0; i < MappingHitPoints.Num(); i++)
	{
		FVector Start = MappingHitPoints[i];
		FVector End = MappingHitPoints[(i + 1) % MappingHitPoints.Num()];
		float SegmentLength = FVector::Dist(Start, End);
		
		if (WrappedDistance <= AccumulatedDistance + SegmentLength)
		{
			// We're in this segment - interpolate
			float SegmentAlpha = (WrappedDistance - AccumulatedDistance) / SegmentLength;
			return FMath::Lerp(Start, End, SegmentAlpha);
		}
		
		AccumulatedDistance += SegmentLength;
	}
	
	// Shouldn't reach here, return last point
	return MappingHitPoints.Last();
}

FVector UNKRecordingCameraComponent::GetTangentAtDistance(float DistanceAlongPath) const
{
	if (MappingHitPoints.Num() < 2)
		return FVector::ForwardVector;
	
	// Wrap distance
	float WrappedDistance = FMath::Fmod(DistanceAlongPath, TotalPathLength);
	if (WrappedDistance < 0.0f)
		WrappedDistance += TotalPathLength;
	
	// Find which segment we're in
	float AccumulatedDistance = 0.0f;
	
	for (int32 i = 0; i < MappingHitPoints.Num(); i++)
	{
		FVector Start = MappingHitPoints[i];
		FVector End = MappingHitPoints[(i + 1) % MappingHitPoints.Num()];
		float SegmentLength = FVector::Dist(Start, End);
		
		if (WrappedDistance <= AccumulatedDistance + SegmentLength)
		{
			// Tangent is direction from start to end
			return (End - Start).GetSafeNormal();
		}
		
		AccumulatedDistance += SegmentLength;
	}
	
	// Default to first segment direction
	return (MappingHitPoints[1] - MappingHitPoints[0]).GetSafeNormal();
}

FVector UNKRecordingCameraComponent::CalculateCameraPosition(FVector OrbitPoint, FVector TangentDirection) const
{
	// Calculate perpendicular direction (OUTWARD from orbit center)
	// Use 2D calculation (XY plane only) - ignore Z completely
	FVector Center2D = FVector(OrbitCenter.X, OrbitCenter.Y, 0.0f);
	FVector OrbitPoint2D = FVector(OrbitPoint.X, OrbitPoint.Y, 0.0f);
	FVector OutwardDirection2D = (OrbitPoint2D - Center2D).GetSafeNormal();
	
	// Add back Z=0 to make it a proper 3D direction (horizontal only)
	FVector OutwardDirection = FVector(OutwardDirection2D.X, OutwardDirection2D.Y, 0.0f);
	
	// Offset camera position OUTWARD from hit point (5 meters away from surface, SAME Z height)
	FVector CameraPosition = OrbitPoint + (OutwardDirection * RecordingOffsetDistanceCm);
	
	// 🔍 DEBUG LOGGING - Camera Position Calculation
	static int32 LogCounter = 0;
	if (LogCounter % 30 == 0) // Log every 30 frames to avoid spam
	{
		UE_LOG(LogTemp, Warning, TEXT("🎥 [RecordingCamera] CalculateCameraPosition DEBUG:"));
		UE_LOG(LogTemp, Warning, TEXT("   OrbitCenter (3D): (%.2f, %.2f, %.2f) m"), 
			OrbitCenter.X / 100.0f, OrbitCenter.Y / 100.0f, OrbitCenter.Z / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   OrbitPoint (Hit): (%.2f, %.2f, %.2f) m"), 
			OrbitPoint.X / 100.0f, OrbitPoint.Y / 100.0f, OrbitPoint.Z / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   Center2D: (%.2f, %.2f, %.2f)"), 
			Center2D.X / 100.0f, Center2D.Y / 100.0f, Center2D.Z / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   OrbitPoint2D: (%.2f, %.2f, %.2f)"), 
			OrbitPoint2D.X / 100.0f, OrbitPoint2D.Y / 100.0f, OrbitPoint2D.Z / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   OutwardDirection2D: (%.4f, %.4f, %.4f) [Length: %.4f]"), 
			OutwardDirection2D.X, OutwardDirection2D.Y, OutwardDirection2D.Z, OutwardDirection2D.Size());
		UE_LOG(LogTemp, Warning, TEXT("   OutwardDirection: (%.4f, %.4f, %.4f)"), 
			OutwardDirection.X, OutwardDirection.Y, OutwardDirection.Z);
		UE_LOG(LogTemp, Warning, TEXT("   RecordingOffsetDistanceCm: %.2f cm (%.2f m)"), 
			RecordingOffsetDistanceCm, RecordingOffsetDistanceCm / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   Offset Vector: (%.2f, %.2f, %.2f) m"), 
			(OutwardDirection * RecordingOffsetDistanceCm).X / 100.0f,
			(OutwardDirection * RecordingOffsetDistanceCm).Y / 100.0f,
			(OutwardDirection * RecordingOffsetDistanceCm).Z / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   ➡️ Final CameraPosition: (%.2f, %.2f, %.2f) m"), 
			CameraPosition.X / 100.0f, CameraPosition.Y / 100.0f, CameraPosition.Z / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   📏 Distance from OrbitPoint to Camera: %.2f m"), 
			FVector::Dist(OrbitPoint, CameraPosition) / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("   📏 Distance from OrbitCenter to Camera: %.2f m"), 
			FVector::Dist(OrbitCenter, CameraPosition) / 100.0f);
	}
	LogCounter++;
	
	return CameraPosition;
}

FRotator UNKRecordingCameraComponent::CalculateCameraRotation(FVector CameraPosition, FVector OrbitPoint) const
{
	FVector LookAtTarget;
	
	switch (RecordingLookMode)
	{
		case ERecordingLookMode::Perpendicular:
			// Look at orbit point (perpendicular view)
			LookAtTarget = OrbitPoint;
			break;
			
		case ERecordingLookMode::Center:
			// Look at orbit center
			if (RecordingTargetActor)
			{
				LookAtTarget = RecordingTargetActor->GetComponentsBoundingBox(true).GetCenter();
			}
			else
			{
				LookAtTarget = OrbitCenter;
			}
			break;
			
		case ERecordingLookMode::LookAhead:
			// Look ahead on path
			{
				float LookAheadDist = FMath::Min(CurrentDistance + RecordingLookAheadDistanceCm, TotalPathLength);
				LookAtTarget = GetInterpolatedPosition(LookAheadDist);
			}
			break;
			
		default:
			LookAtTarget = OrbitPoint;
			break;
	}
	
	return UKismetMathLibrary::FindLookAtRotation(CameraPosition, LookAtTarget);
}

void UNKRecordingCameraComponent::DrawDebugVisualization(FVector CameraPosition, FVector OrbitPoint)
{
	UWorld* World = GetWorld();
	if (!World)
		return;
	
	float DrawDuration = 0.016f;  // One frame at 60 FPS
	
	// Draw camera-to-orbit connection
	if (bRecordingDrawDebugPath)
	{
		DrawDebugLine(World, CameraPosition, OrbitPoint, 
			FColor::Yellow, false, DrawDuration, 0, 2.0f);
		
		// Camera sphere
		DrawDebugSphere(World, CameraPosition, 
			30.0f, 8, FColor::Cyan, false, DrawDuration);
		
		// Orbit point sphere
		DrawDebugSphere(World, OrbitPoint, 
			15.0f, 8, FColor::Green, false, DrawDuration);
	}
	
	// Draw orbital path
	if (bRecordingDrawOrbitPath)
	{
		for (int32 i = 0; i < MappingHitPoints.Num(); i++)
		{
			FVector Start = MappingHitPoints[i];
			FVector End = MappingHitPoints[(i + 1) % MappingHitPoints.Num()];
			
			DrawDebugLine(World, Start, End, 
				FColor::Green, false, DrawDuration, 0, 2.0f);
		}
	}
	
	// Draw camera path (offset orbit)
	if (bRecordingDrawCameraPath)
	{
		for (int32 i = 0; i < MappingHitPoints.Num(); i++)
		{
			FVector OrbitPt = MappingHitPoints[i];
			FVector NextOrbitPt = MappingHitPoints[(i + 1) % MappingHitPoints.Num()];
			
			FVector TangentDir = (NextOrbitPt - OrbitPt).GetSafeNormal();
			FVector CamPos = CalculateCameraPosition(OrbitPt, TangentDir);
			
			FVector NextTangentDir = (MappingHitPoints[(i + 2) % MappingHitPoints.Num()] - NextOrbitPt).GetSafeNormal();
			FVector NextCamPos = CalculateCameraPosition(NextOrbitPt, NextTangentDir);
			
			DrawDebugLine(World, CamPos, NextCamPos, 
				FColor::Cyan, false, DrawDuration, 0, 3.0f);
		}
	}
}
