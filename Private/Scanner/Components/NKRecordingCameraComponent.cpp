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
	CurrentDistance += (PlaybackSpeed * 100.0f) * DeltaTime;  // Convert m/s to cm/s
	
	// 2. Handle looping or completion
	if (CurrentDistance >= TotalPathLength)
	{
		if (bLoopPlayback)
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
	SetComponentTickEnabled(true);
	
	UE_LOG(LogTemp, Warning, TEXT("???????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("?? RECORDING CAMERA PLAYBACK STARTED"));
	UE_LOG(LogTemp, Warning, TEXT("???????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("  Hit Points: %d"), MappingHitPoints.Num());
	UE_LOG(LogTemp, Warning, TEXT("  Path Length: %.2f meters"), TotalPathLength / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Center: (%.2f, %.2f, %.2f) m"), 
		OrbitCenter.X/100.0f, OrbitCenter.Y/100.0f, OrbitCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Playback Speed: %.1f m/s"), PlaybackSpeed);
	UE_LOG(LogTemp, Warning, TEXT("  Camera Offset: %.1f m"), OffsetDistanceCm / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Look Mode: %s"), 
		LookMode == ERecordingLookMode::Perpendicular ? TEXT("Perpendicular") :
		LookMode == ERecordingLookMode::Center ? TEXT("Center") : TEXT("Look-Ahead"));
	UE_LOG(LogTemp, Warning, TEXT("  Loop Playback: %s"), bLoopPlayback ? TEXT("Yes") : TEXT("No"));
	UE_LOG(LogTemp, Warning, TEXT("  Estimated Duration: %.1f seconds"), 
		TotalPathLength / (PlaybackSpeed * 100.0f));
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
	
	// Simple average of all points (works for circular orbit)
	FVector Sum = FVector::ZeroVector;
	for (const FVector& Point : MappingHitPoints)
	{
		Sum += Point;
	}
	
	return Sum / MappingHitPoints.Num();
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
	// Calculate perpendicular direction (RIGHT of tangent)
	FVector RightDirection = FVector::CrossProduct(
		TangentDirection,
		FVector::UpVector
	).GetSafeNormal();
	
	// Inward direction (toward orbit center) is opposite of right
	FVector InwardDirection = -RightDirection;
	
	// Offset camera position inward from orbit
	FVector CameraPosition = OrbitPoint + (InwardDirection * OffsetDistanceCm);
	
	return CameraPosition;
}

FRotator UNKRecordingCameraComponent::CalculateCameraRotation(FVector CameraPosition, FVector OrbitPoint) const
{
	FVector LookAtTarget;
	
	switch (LookMode)
	{
		case ERecordingLookMode::Perpendicular:
			// Look at orbit point (perpendicular view)
			LookAtTarget = OrbitPoint;
			break;
			
		case ERecordingLookMode::Center:
			// Look at orbit center
			if (TargetActor)
			{
				LookAtTarget = TargetActor->GetComponentsBoundingBox(true).GetCenter();
			}
			else
			{
				LookAtTarget = OrbitCenter;
			}
			break;
			
		case ERecordingLookMode::LookAhead:
			// Look ahead on path
			{
				float LookAheadDist = FMath::Min(CurrentDistance + LookAheadDistanceCm, TotalPathLength);
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
	if (bDrawDebugPath)
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
	if (bDrawOrbitPath)
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
	if (bDrawCameraPath)
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
