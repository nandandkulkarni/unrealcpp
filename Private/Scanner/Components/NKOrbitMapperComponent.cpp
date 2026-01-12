// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Components/NKOrbitMapperComponent.h"
#include "Scanner/Components/NKLaserTracerComponent.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

UNKOrbitMapperComponent::UNKOrbitMapperComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;  // Only tick when mapping
}

void UNKOrbitMapperComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UNKOrbitMapperComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (!bIsMapping)
	{
		return;
	}
	
	PerformMappingStep(DeltaTime);
}

void UNKOrbitMapperComponent::StartMapping(
	AActor* InTargetActor,
	FVector InOrbitCenter,
	float InOrbitRadius,
	float InScanHeight,
	float InStartAngle,
	UNKLaserTracerComponent* InLaserTracer)
{
	if (!InTargetActor || !InLaserTracer)
	{
		UE_LOG(LogTemp, Error, TEXT("OrbitMapper: Cannot start mapping - invalid target or laser tracer"));
		OnMappingFailed.Broadcast();
		return;
	}
	
	// Store configuration
	TargetActor = InTargetActor;
	LaserTracer = InLaserTracer;
	OrbitCenter = InOrbitCenter;
	OrbitRadius = InOrbitRadius;
	ScanHeight = InScanHeight;
	StartAngle = InStartAngle;
	CurrentAngle = InStartAngle;
	
	// Reset counters
	ShotCount = 0;
	HitCount = 0;
	TimeSinceLastShot = 0.0f;
	
	// Enable ticking
	bIsMapping = true;
	SetComponentTickEnabled(true);
	
	UE_LOG(LogTemp, Warning, TEXT("?????????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("? ORBIT MAPPER - START MAPPING                          ?"));
	UE_LOG(LogTemp, Warning, TEXT("?????????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("? Target: %s"), *TargetActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("? Orbit Center: (%.2f, %.2f, %.2f) m"),
		OrbitCenter.X/100.0f, OrbitCenter.Y/100.0f, OrbitCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("? Orbit Radius: %.2f m"), OrbitRadius/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("? Scan Height: %.2f m"), ScanHeight/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("? Start Angle: %.1f°"), StartAngle);
	UE_LOG(LogTemp, Warning, TEXT("? Angular Step: %.1f°"), AngularStepDegrees);
	UE_LOG(LogTemp, Warning, TEXT("? Expected Shots: ~%d"), FMath::CeilToInt(360.0f / AngularStepDegrees));
	UE_LOG(LogTemp, Warning, TEXT("?????????????????????????????????????????????????????????"));
}

void UNKOrbitMapperComponent::StopMapping()
{
	if (!bIsMapping)
	{
		return;
	}
	
	bIsMapping = false;
	SetComponentTickEnabled(false);
	
	UE_LOG(LogTemp, Warning, TEXT("OrbitMapper: Mapping stopped - %d shots taken, %d hits"), 
		ShotCount, HitCount);
}

float UNKOrbitMapperComponent::GetProgressPercent() const
{
	if (!bIsMapping)
	{
		return 0.0f;
	}
	
	// Calculate how far we've rotated from start
	float AnglesTraveled = CurrentAngle - StartAngle;
	
	// Normalize to 0-360 range
	while (AnglesTraveled < 0.0f)
	{
		AnglesTraveled += 360.0f;
	}
	while (AnglesTraveled >= 360.0f)
	{
		AnglesTraveled -= 360.0f;
	}
	
	return (AnglesTraveled / 360.0f) * 100.0f;
}

FVector UNKOrbitMapperComponent::CalculateOrbitPosition(float Angle) const
{
	// Convert angle to radians
	float AngleRad = FMath::DegreesToRadians(Angle);
	
	// Calculate position on circle
	float X = OrbitCenter.X + (OrbitRadius * FMath::Cos(AngleRad));
	float Y = OrbitCenter.Y + (OrbitRadius * FMath::Sin(AngleRad));
	float Z = ScanHeight;
	
	return FVector(X, Y, Z);
}

FRotator UNKOrbitMapperComponent::CalculateLookAtRotation(const FVector& FromPosition, const FVector& ToPosition) const
{
	return UKismetMathLibrary::FindLookAtRotation(FromPosition, ToPosition);
}

void UNKOrbitMapperComponent::PerformMappingStep(float DeltaTime)
{
	// Update shot delay timer
	TimeSinceLastShot += DeltaTime;
	
	if (TimeSinceLastShot < ShotDelay)
	{
		return;  // Not time to shoot yet
	}
	
	// Reset timer
	TimeSinceLastShot = 0.0f;
	
	// Calculate current position on orbit
	FVector OrbitPosition = CalculateOrbitPosition(CurrentAngle);
	
	// Move camera to orbit position
	AActor* Owner = GetOwner();
	if (Owner)
	{
		Owner->SetActorLocation(OrbitPosition);
		
		// Look at target center
		FRotator LookAtRotation = CalculateLookAtRotation(OrbitPosition, OrbitCenter);
		Owner->SetActorRotation(LookAtRotation);
	}
	
	// Shoot laser at target using LaserTracer's PerformTrace method
	if (LaserTracer)
	{
		FHitResult HitResult;
		bool bHit = LaserTracer->PerformTrace(HitResult);
		
		ShotCount++;
		
		if (bHit)
		{
			HitCount++;
			
			if (bDrawDebugVisuals)
			{
				// Draw hit point
				DrawDebugSphere(
					GetWorld(),
					HitResult.Location,
					10.0f,  // 10cm radius
					8,
					FColor::Green,
					false,
					2.0f,  // 2 second lifetime
					0,
					2.0f
				);
			}
		}
		
		// Log every 10 shots
		if (ShotCount % 10 == 0)
		{
			UE_LOG(LogTemp, Log, TEXT("OrbitMapper: Shot #%d at angle %.1f° - Progress: %.1f%% - Hits: %d"),
				ShotCount, CurrentAngle, GetProgressPercent(), HitCount);
		}
	}
	
	// Draw debug orbit position
	if (bDrawDebugVisuals)
	{
		DrawDebugSphere(
			GetWorld(),
			OrbitPosition,
			30.0f,  // 30cm radius
			8,
			FColor::Cyan,
			false,
			0.2f,  // Brief lifetime
			0,
			2.0f
		);
	}
	
	// Advance angle
	CurrentAngle += AngularStepDegrees;
	
	// Check if we've completed a full orbit
	if (CurrentAngle >= StartAngle + 360.0f)
	{
		CompletMapping();
	}
}

void UNKOrbitMapperComponent::CompletMapping()
{
	UE_LOG(LogTemp, Warning, TEXT("?????????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("? ORBIT MAPPER - MAPPING COMPLETE                       ?"));
	UE_LOG(LogTemp, Warning, TEXT("?????????????????????????????????????????????????????????"));
	UE_LOG(LogTemp, Warning, TEXT("? Total Shots: %d"), ShotCount);
	UE_LOG(LogTemp, Warning, TEXT("? Total Hits: %d"), HitCount);
	UE_LOG(LogTemp, Warning, TEXT("? Hit Rate: %.1f%%"), HitCount > 0 ? (HitCount / (float)ShotCount * 100.0f) : 0.0f);
	UE_LOG(LogTemp, Warning, TEXT("? Final Angle: %.1f°"), CurrentAngle);
	UE_LOG(LogTemp, Warning, TEXT("?????????????????????????????????????????????????????????"));
	
	bIsMapping = false;
	SetComponentTickEnabled(false);
	
	OnMappingComplete.Broadcast();
}
 