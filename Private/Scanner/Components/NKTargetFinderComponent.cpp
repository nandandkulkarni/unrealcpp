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

void UNKTargetFinderComponent::StartDiscovery(AActor* Target, float InScanHeight)
{
	if (!Target)
	{
		UE_LOG(LogTemp, Error, TEXT("UNKTargetFinderComponent::StartDiscovery - Invalid target"));
		return;
	}
	
	// Store parameters
	TargetActor = Target;
	ScanHeight = InScanHeight;
	
	// Get target center for reference only
	FBox TargetBounds = Target->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	OrbitCenter = FVector(TargetCenter.X, TargetCenter.Y, InScanHeight);
	
	// Reset state
	bIsDiscovering = true;
	ShotCount = 0;
	CurrentAngle = 0.0f;
	bHasFoundTarget = false;
	TimeAccumulator = 0.0f;
	
	// Position camera at configured height (don't move XY, just set Z if needed)
	if (CameraController)
	{
		FVector CurrentPos = CameraController->GetCameraPosition();
		FVector NewPos = FVector(CurrentPos.X, CurrentPos.Y, InScanHeight);
		CameraController->SetPosition(NewPos);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: Discovery started - Stationary rotation mode"));
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
		// Verify we hit the TARGET actor, not just any collision
		AActor* HitActor = HitResult.GetActor();
		bool bHitTarget = (HitActor == TargetActor);
		
		// Get actor label and mesh name for debugging
		FString HitActorLabel = HitActor ? HitActor->GetActorLabel() : TEXT("NULL");
		FString HitActorName = HitActor ? HitActor->GetName() : TEXT("NULL");
		FString TargetActorLabel = TargetActor ? TargetActor->GetActorLabel() : TEXT("NULL");
		
		// Get mesh/component name if available
		FString ComponentName = HitResult.Component.IsValid() ? HitResult.Component->GetName() : TEXT("NULL");
		FString ComponentClass = HitResult.Component.IsValid() ? HitResult.Component->GetClass()->GetName() : TEXT("NULL");
		
		if (!bHitTarget)
		{
			UE_LOG(LogTemp, Log, TEXT("  Hit WRONG actor at %.1f°:"), CurrentAngle);
			UE_LOG(LogTemp, Log, TEXT("    Hit: '%s' (%s)"), *HitActorLabel, *HitActorName);
			UE_LOG(LogTemp, Log, TEXT("    Component: %s (%s)"), *ComponentName, *ComponentClass);
			UE_LOG(LogTemp, Log, TEXT("    Looking for: '%s'"), *TargetActorLabel);
			UE_LOG(LogTemp, Log, TEXT("    Distance: %.2fm - continuing scan"), HitResult.Distance / 100.0f);
		}
		else
		{
			// Found target!
			bHasFoundTarget = true;
			FirstHit = HitResult;
			FirstHitAngle = CurrentAngle;
			
			UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: ✅ TARGET FOUND at angle %.1f°"), CurrentAngle);
			UE_LOG(LogTemp, Warning, TEXT("  Hit Actor: '%s' (%s)"), *HitActorLabel, *HitActorName);
			UE_LOG(LogTemp, Warning, TEXT("  Component: %s (%s)"), *ComponentName, *ComponentClass);
			UE_LOG(LogTemp, Warning, TEXT("  Distance: %.2fm"), HitResult.Distance / 100.0f);
			UE_LOG(LogTemp, Warning, TEXT("  Broadcasting OnTargetFound event..."));
			
			OnTargetFound.Broadcast(HitResult);
			
			UE_LOG(LogTemp, Warning, TEXT("  Event broadcast complete, stopping discovery"));
			StopDiscovery();
			return;
		}
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
	// Camera is already positioned - just set initial rotation to 0°
	if (CameraController)
	{
		FRotator InitialRotation(0.0f, 0.0f, 0.0f);
		CameraController->SetRotation(InitialRotation);
		UE_LOG(LogTemp, Warning, TEXT("UNKTargetFinderComponent: Camera ready at initial rotation (0°)"));
	}
}

void UNKTargetFinderComponent::RotateCameraToAngle(float Angle)
{
	if (!CameraController)
	{
		return;
	}
	
	// Simple rotation: Just set camera yaw to the angle
	// Camera stays in place, only rotates
	FRotator NewRotation(0.0f, Angle, 0.0f);  // Pitch=0, Yaw=Angle, Roll=0
	CameraController->SetRotation(NewRotation);
	
	UE_LOG(LogTemp, Log, TEXT("Camera rotated to yaw: %.1f°"), Angle);
}
