// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Components/NKTerrainMapperComponent.h"
#include "Scanner/Interfaces/INKLaserTracerInterface.h"
#include "Scanner/Interfaces/INKCameraControllerInterface.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

UNKTerrainMapperComponent::UNKTerrainMapperComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNKTerrainMapperComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Find laser tracer component on owner
	AActor* Owner = GetOwner();
	if (Owner)
	{
		TArray<UActorComponent*> Components;
		Owner->GetComponents(Components);
		
		for (UActorComponent* Component : Components)
		{
			if (!LaserTracer)
			{
				LaserTracer = Cast<INKLaserTracerInterface>(Component);
			}
			if (!CameraController)
			{
				CameraController = Cast<INKCameraControllerInterface>(Component);
			}
		}
	}
	
	if (!LaserTracer)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTerrainMapperComponent: LaserTracer not found!"));
	}
	if (!CameraController)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTerrainMapperComponent: CameraController not found!"));
	}
}

void UNKTerrainMapperComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsMapping && !bIsPaused)
	{
		// Accumulate time (same pattern as discovery)
		TimeAccumulator += DeltaTime;
		ElapsedTime += DeltaTime;
		
		// Only shoot when interval elapsed
		if (TimeAccumulator >= ShotIntervalSeconds)
		{
			PerformMappingShot();  // Move + shoot + record
			TimeAccumulator = 0.0f;  // Reset for next shot
		}
	}
}

void UNKTerrainMapperComponent::StartMapping(float InStartAngle, float InOrbitRadius, float InScanHeight)
{
	if (!LaserTracer || !CameraController)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTerrainMapperComponent::StartMapping - Missing required components!"));
		return;
	}
	
	// Store parameters
	StartAngle = InStartAngle;
	CurrentAngle = InStartAngle;
	OrbitRadius = InOrbitRadius;
	ScanHeight = InScanHeight;
	
	// Reset state
	bIsMapping = true;
	bIsPaused = false;
	TimeAccumulator = 0.0f;
	ElapsedTime = 0.0f;
	PointCount = 0;
	ShotCount = 0;
	ScanData.Empty();
	
	// Calculate expected number of shots
	float Circumference = 2.0f * PI * OrbitRadius;
	float StepSizeCm = StepSizeMeters * 100.0f;
	int32 ExpectedShots = FMath::CeilToInt(Circumference / StepSizeCm);
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("TERRAIN MAPPER: Starting Orbit Mapping"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("Orbit Parameters:"));
	UE_LOG(LogTemp, Warning, TEXT("  Center: (%.1f, %.1f, %.1f)"), 
		OrbitCenter.X / 100.0f, OrbitCenter.Y / 100.0f, OrbitCenter.Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Radius: %.2fm"), OrbitRadius / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Height: %.2fm"), ScanHeight / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Start Angle: %.1f°"), StartAngle);
	UE_LOG(LogTemp, Warning, TEXT("Mapping Configuration:"));
	UE_LOG(LogTemp, Warning, TEXT("  Step Size: %.1fm"), StepSizeMeters);
	UE_LOG(LogTemp, Warning, TEXT("  Shot Interval: %.0fms"), ShotIntervalSeconds * 1000.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Direction: %s"), 
		OrbitDirection == EOrbitDirection::Clockwise ? TEXT("Clockwise") : TEXT("Counter-Clockwise"));
	UE_LOG(LogTemp, Warning, TEXT("  Expected Shots: ~%d"), ExpectedShots);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void UNKTerrainMapperComponent::StopMapping()
{
	bIsMapping = false;
	UE_LOG(LogTemp, Warning, TEXT("Terrain Mapper: Mapping stopped"));
}

void UNKTerrainMapperComponent::PauseMapping()
{
	bIsPaused = true;
	UE_LOG(LogTemp, Warning, TEXT("Terrain Mapper: Mapping paused"));
}

void UNKTerrainMapperComponent::ResumeMapping()
{
	bIsPaused = false;
	UE_LOG(LogTemp, Warning, TEXT("Terrain Mapper: Mapping resumed"));
}

float UNKTerrainMapperComponent::GetMappingProgress() const
{
	if (!bIsMapping)
	{
		return 0.0f;
	}
	
	float AngleTraveled = CurrentAngle - StartAngle;
	if (AngleTraveled < 0.0f)
	{
		AngleTraveled += 360.0f;
	}
	
	return FMath::Clamp(AngleTraveled / 360.0f * 100.0f, 0.0f, 100.0f);
}

void UNKTerrainMapperComponent::ClearScanData()
{
	ScanData.Empty();
	PointCount = 0;
	UE_LOG(LogTemp, Log, TEXT("Terrain Mapper: Scan data cleared"));
}

bool UNKTerrainMapperComponent::SaveToJSON(const FString& FilePath)
{
	// TODO: Implement JSON serialization
	UE_LOG(LogTemp, Warning, TEXT("Terrain Mapper: SaveToJSON not yet implemented"));
	return false;
}

bool UNKTerrainMapperComponent::LoadFromJSON(const FString& FilePath)
{
	// TODO: Implement JSON deserialization
	UE_LOG(LogTemp, Warning, TEXT("Terrain Mapper: LoadFromJSON not yet implemented"));
	return false;
}

void UNKTerrainMapperComponent::PerformMappingShot()
{
	if (!LaserTracer || !CameraController)
	{
		return;
	}
	
	// Calculate angular step from linear step size
	float AngularStep = CalculateAngularStep();
	
	// Update angle based on direction
	float DirectionMultiplier = (OrbitDirection == EOrbitDirection::Clockwise) ? -1.0f : 1.0f;
	CurrentAngle += AngularStep * DirectionMultiplier;
	
	// Normalize angle to 0-360 range
	while (CurrentAngle >= 360.0f) CurrentAngle -= 360.0f;
	while (CurrentAngle < 0.0f) CurrentAngle += 360.0f;
	
	// Calculate new orbit position
	float AngleRadians = FMath::DegreesToRadians(CurrentAngle);
	FVector NewPosition = FVector(
		OrbitCenter.X + OrbitRadius * FMath::Cos(AngleRadians),
		OrbitCenter.Y + OrbitRadius * FMath::Sin(AngleRadians),
		ScanHeight
	);
	
	// Move camera to new position
	CameraController->SetPosition(NewPosition);
	
	// Look at target center
	FVector DirectionToCenter = (OrbitCenter - NewPosition).GetSafeNormal();
	FRotator LookAtRotation = DirectionToCenter.Rotation();
	CameraController->SetRotation(LookAtRotation);
	
	// Shoot laser
	FHitResult HitResult;
	bool bHit = LaserTracer->PerformTrace(HitResult);
	
	ShotCount++;
	
	// Record data if hit
	if (bHit)
	{
		FScanDataPoint DataPoint;
		DataPoint.WorldPosition = HitResult.Location;
		DataPoint.Normal = HitResult.Normal;
		DataPoint.OrbitAngle = CurrentAngle;
		DataPoint.ScanHeight = ScanHeight;
		DataPoint.DistanceFromCamera = FVector::Dist(NewPosition, HitResult.Location);
		DataPoint.HitActor = HitResult.GetActor();
		DataPoint.TimeStamp = ElapsedTime;
		DataPoint.ComponentName = HitResult.Component.IsValid() ? HitResult.Component->GetFName() : NAME_None;
		
		ScanData.Add(DataPoint);
		PointCount++;
	}
	
	// Fire progress event every 10 shots
	if (ShotCount % 10 == 0)
	{
		float Progress = GetMappingProgress();
		OnMappingProgress.Broadcast(Progress, PointCount);
		
		UE_LOG(LogTemp, Log, TEXT("Mapping Progress: %.1f%% | Angle: %.1f° | Points: %d | Shots: %d"),
			Progress, CurrentAngle, PointCount, ShotCount);
	}
	
	// Check if completed full orbit
	float AngleTraveled = CurrentAngle - StartAngle;
	if (AngleTraveled < 0.0f)
	{
		AngleTraveled += 360.0f;
	}
	
	if (AngleTraveled >= 360.0f)
	{
		CompleteMapping();
	}
}

void UNKTerrainMapperComponent::CompleteMapping()
{
	bIsMapping = false;
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("TERRAIN MAPPER: Mapping Complete!"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("Statistics:"));
	UE_LOG(LogTemp, Warning, TEXT("  Total Shots: %d"), ShotCount);
	UE_LOG(LogTemp, Warning, TEXT("  Points Recorded: %d"), PointCount);
	UE_LOG(LogTemp, Warning, TEXT("  Hit Rate: %.1f%%"), (float)PointCount / (float)ShotCount * 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Elapsed Time: %.2f seconds"), ElapsedTime);
	UE_LOG(LogTemp, Warning, TEXT("  Average Speed: %.1f points/sec"), (float)PointCount / ElapsedTime);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Fire completion event
	OnMappingComplete.Broadcast(PointCount);
}

float UNKTerrainMapperComponent::CalculateAngularStep() const
{
	// Convert step size to cm
	float StepSizeCm = StepSizeMeters * 100.0f;
	
	// Calculate circumference
	float Circumference = 2.0f * PI * OrbitRadius;
	
	// Calculate angular step in degrees
	float AngularStep = (StepSizeCm / Circumference) * 360.0f;
	
	return AngularStep;
}
