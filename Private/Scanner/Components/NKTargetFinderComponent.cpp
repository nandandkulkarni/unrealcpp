// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/Components/NKTargetFinderComponent.h"
#include "Scanner/Interfaces/INKLaserTracerInterface.h"
#include "Scanner/Interfaces/INKCameraControllerInterface.h"

UNKTargetFinderComponent::UNKTargetFinderComponent()
	: bIsDiscovering(false)
	, ShotCount(0)
	, CurrentAngle(0.0f)
	, bHasFoundTarget(false)
	, FirstHitAngle(0.0f)
	, TimeAccumulator(0.0f)
	, TargetActor(nullptr)
	, OrbitRadius(0.0f)
	, ScanHeight(0.0f)
	, LaserTracer(nullptr)
	, CameraController(nullptr)
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNKTargetFinderComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// Get references to other components on the same actor
	AActor* Owner = GetOwner();
	if (Owner)
	{
		// Find laser tracer component
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
}

void UNKTargetFinderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bIsDiscovering)
	{
		// Accumulate time
		TimeAccumulator += DeltaTime;
		
		// Only perform shot when enough time has elapsed
		if (TimeAccumulator >= ShotInterval)
		{
			PerformDiscoveryShot();
			TimeAccumulator = 0.0f;  // Reset timer for next shot
		}
	}
}

void UNKTargetFinderComponent::StartDiscovery(AActor* Target, float InOrbitRadius, float InScanHeight)
{
	if (!Target)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTargetFinderComponent::StartDiscovery - Invalid target"));
		return;
	}
	
	// Store parameters
	TargetActor = Target;
	OrbitRadius = InOrbitRadius;
	ScanHeight = InScanHeight;
	
	// Calculate orbit center
	FBox TargetBounds = Target->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	OrbitCenter = FVector(TargetCenter.X, TargetCenter.Y, InScanHeight);
	
	// Reset state
	bIsDiscovering = true;
	ShotCount = 0;
	CurrentAngle = 0.0f;
	bHasFoundTarget = false;
	TimeAccumulator = 0.0f;  // Reset shot timer
	
	// Position camera at starting point
	PositionCameraAtStart();
	
	UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: Discovery started"));
}

void UNKTargetFinderComponent::StopDiscovery()
{
	bIsDiscovering = false;
	UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: Discovery stopped"));
}

void UNKTargetFinderComponent::PerformDiscoveryShot()
{
	if (!LaserTracer || !CameraController)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTargetFinderComponent: Missing required components"));
		StopDiscovery();
		return;
	}
	
	ShotCount++;
	
	UE_LOG(LogTemp, Log, TEXT("UNKTargetFinderComponent: Shot #%d at angle %.1f°"), ShotCount, CurrentAngle);
	
	// Rotate camera to current angle
	RotateCameraToAngle(CurrentAngle);
	
	// Perform laser trace
	FHitResult HitResult;
	bool bHit = LaserTracer->PerformTrace(HitResult);
	
	UE_LOG(LogTemp, Log, TEXT("UNKTargetFinderComponent: Trace result: %s"), bHit ? TEXT("HIT") : TEXT("MISS"));
	
	// Fire progress event
	OnDiscoveryProgress.Broadcast(ShotCount, CurrentAngle);
	
	if (bHit)
	{
		// Found target!
		bHasFoundTarget = true;
		FirstHit = HitResult;
		FirstHitAngle = CurrentAngle;
		
		UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: ✅ TARGET FOUND at angle %.1f°"), CurrentAngle);
		UE_LOG(LogTemp, Warning, TEXT("  Hit Actor: %s"), HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("NULL"));
		UE_LOG(LogTemp, Warning, TEXT("  Broadcasting OnTargetFound event..."));
		
		OnTargetFound.Broadcast(HitResult);
		
		UE_LOG(LogTemp, Warning, TEXT("  Event broadcast complete, stopping discovery"));
		StopDiscovery();
		return;
	}
	
	// Move to next angle
	CurrentAngle += AngularStepDegrees;
	
	// Check if completed 360° without finding target
	if (CurrentAngle >= 360.0f)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTargetFinderComponent: Discovery failed - no target found after 360°"));
		OnDiscoveryFailed.Broadcast();
		StopDiscovery();
	}
}

void UNKTargetFinderComponent::PositionCameraAtStart()
{
	if (!CameraController)
	{
		return;
	}
	
	// Position camera at angle 0° (South of target)
	CameraController->MoveToOrbitPosition(0.0f, OrbitRadius, OrbitCenter, ScanHeight);
	
	UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: Camera positioned at start (South)"));
}

void UNKTargetFinderComponent::RotateCameraToAngle(float Angle)
{
	if (!CameraController)
	{
		return;
	}
	
	// Calculate direction to look based on angle
	float AngleRadians = FMath::DegreesToRadians(Angle);
	
	// Calculate point on orbit circle to look at
	FVector LookAtPoint;
	LookAtPoint.X = OrbitCenter.X + (OrbitRadius * FMath::Cos(AngleRadians));
	LookAtPoint.Y = OrbitCenter.Y + (OrbitRadius * FMath::Sin(AngleRadians));
	LookAtPoint.Z = ScanHeight;
	
	// Calculate rotation from camera position to look-at point
	FVector CameraPos = CameraController->GetCameraPosition();
	FVector Direction = LookAtPoint - CameraPos;
	Direction.Normalize();
	
	FRotator LookAtRotation = Direction.Rotation();
	CameraController->SetRotation(LookAtRotation);
}
