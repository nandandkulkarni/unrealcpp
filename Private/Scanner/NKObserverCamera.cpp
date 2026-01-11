// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKObserverCamera.h"
#include "DrawDebugHelpers.h"

ANKObserverCamera::ANKObserverCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TargetActor(nullptr)
	, LastTargetCenter(FVector::ZeroVector)
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Make camera mesh visible so it can be seen from other cameras
	// (Same as mapping camera)
}

void ANKObserverCamera::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	// Make camera mesh visible in game
	TArray<UActorComponent*> MeshComponents;
	GetComponents(UStaticMeshComponent::StaticClass(), MeshComponents);
	
	for (UActorComponent* Component : MeshComponents)
	{
		if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Component))
		{
			MeshComp->SetHiddenInGame(false);
			UE_LOG(LogTemp, Warning, TEXT("ANKObserverCamera: Camera mesh '%s' set to visible in game"), 
				*MeshComp->GetName());
		}
	}
}

void ANKObserverCamera::BeginPlay()
{
	Super::BeginPlay();
	
	if (bAutoPositionOnBeginPlay)
	{
		PositionAboveTarget();
	}
}

void ANKObserverCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (bTrackTargetMovement && TargetActor)
	{
		TimeSinceLastCheck += DeltaTime;
		
		if (TimeSinceLastCheck >= UpdateCheckInterval)
		{
			TimeSinceLastCheck = 0.0f;
			
			// Check if target has moved
			FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
			FVector CurrentTargetCenter = TargetBounds.GetCenter();
			
			if (!CurrentTargetCenter.Equals(LastTargetCenter, 10.0f))  // 10cm tolerance
			{
				PositionAboveTarget();
			}
		}
	}
}

void ANKObserverCamera::PositionAboveTarget()
{
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("ANKObserverCamera::PositionAboveTarget - No target actor set!"));
		return;
	}
	
	// Get target bounding box
	FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	FVector TargetMin = TargetBounds.Min;
	FVector TargetMax = TargetBounds.Max;
	
	// Calculate observer position
	float HighestPoint = TargetMax.Z;
	float HeightAboveTarget = HeightAboveTargetMeters * 100.0f;  // Convert to cm
	float ObserverHeight = HighestPoint + HeightAboveTarget;
	
	FVector ObserverPosition = FVector(
		TargetCenter.X,  // Centered over target (X)
		TargetCenter.Y,  // Centered over target (Y)
		ObserverHeight   // Height above highest point
	);
	
	// Set camera position
	SetActorLocation(ObserverPosition);
	
	// Set camera rotation (look down at target)
	FRotator LookAtRotation = FRotator(CameraPitchDegrees, 0.0f, 0.0f);
	SetActorRotation(LookAtRotation);
	
	// Store last target center for movement tracking
	LastTargetCenter = TargetCenter;
	
	// Log positioning
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("ANKObserverCamera: Positioned Above Target"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("TARGET INFO:"));
	UE_LOG(LogTemp, Warning, TEXT("  Name: %s"), *TargetActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Center: (%.1f, %.1f, %.1f)"),
		TargetCenter.X / 100.0f, TargetCenter.Y / 100.0f, TargetCenter.Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Highest Point: %.2fm"), HighestPoint / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("OBSERVER CAMERA:"));
	UE_LOG(LogTemp, Warning, TEXT("  Position: (%.1f, %.1f, %.1f)"),
		ObserverPosition.X / 100.0f, ObserverPosition.Y / 100.0f, ObserverPosition.Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Height Above Target: %.2fm"), HeightAboveTarget / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Pitch: %.1f°"), CameraPitchDegrees);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Draw debug visualization
	DrawDebugSphere(
		GetWorld(),
		ObserverPosition,
		100.0f,  // 1m radius sphere
		16,
		FColor::Yellow,
		false,
		10.0f,  // 10 second lifetime
		0,
		5.0f
	);
	
	// Draw line from observer to target center
	DrawDebugLine(
		GetWorld(),
		ObserverPosition,
		TargetCenter,
		FColor::Yellow,
		false,
		10.0f,
		0,
		3.0f
	);
}

float ANKObserverCamera::GetCurrentHeight() const
{
	if (!TargetActor)
	{
		return 0.0f;
	}
	
	FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	float HighestPoint = TargetBounds.Max.Z;
	float CurrentHeight = GetActorLocation().Z;
	
	return (CurrentHeight - HighestPoint) / 100.0f;  // Return in meters
}
