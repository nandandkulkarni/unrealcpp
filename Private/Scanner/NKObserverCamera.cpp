// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKObserverCamera.h"
#include "DrawDebugHelpers.h"
#include "CineCameraComponent.h"

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
	// Pitch -90 = looking straight down
	FRotator LookAtRotation = FRotator(CameraPitchDegrees, 0.0f, 0.0f);
	SetActorRotation(LookAtRotation);
	
	// Also set the camera component's rotation to ensure it's looking down
	if (UCineCameraComponent* CineCamera = GetCineCameraComponent())
	{
		CineCamera->SetWorldRotation(LookAtRotation);
	}
	
	// Store last target center for movement tracking
	LastTargetCenter = TargetCenter;
	
	// ===== COMPREHENSIVE LOGGING =====
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("OBSERVER CAMERA POSITIONING DEBUG"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Target information
	UE_LOG(LogTemp, Warning, TEXT("TARGET ACTOR:"));
	UE_LOG(LogTemp, Warning, TEXT("  Name: %s"), *TargetActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Class: %s"), *TargetActor->GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Location: (%.2f, %.2f, %.2f) cm"), 
		TargetActor->GetActorLocation().X, TargetActor->GetActorLocation().Y, TargetActor->GetActorLocation().Z);
	
	// Bounding box details
	UE_LOG(LogTemp, Warning, TEXT("TARGET BOUNDING BOX (in cm):"));
	UE_LOG(LogTemp, Warning, TEXT("  Min: (%.2f, %.2f, %.2f)"), TargetMin.X, TargetMin.Y, TargetMin.Z);
	UE_LOG(LogTemp, Warning, TEXT("  Max: (%.2f, %.2f, %.2f)"), TargetMax.X, TargetMax.Y, TargetMax.Z);
	UE_LOG(LogTemp, Warning, TEXT("  Center: (%.2f, %.2f, %.2f)"), TargetCenter.X, TargetCenter.Y, TargetCenter.Z);
	UE_LOG(LogTemp, Warning, TEXT("  Size: (%.2f x %.2f x %.2f) cm"), 
		TargetMax.X - TargetMin.X, TargetMax.Y - TargetMin.Y, TargetMax.Z - TargetMin.Z);
	
	// Bounding box in meters
	UE_LOG(LogTemp, Warning, TEXT("TARGET BOUNDING BOX (in meters):"));
	UE_LOG(LogTemp, Warning, TEXT("  Min: (%.2f, %.2f, %.2f) m"), TargetMin.X/100.0f, TargetMin.Y/100.0f, TargetMin.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Max: (%.2f, %.2f, %.2f) m"), TargetMax.X/100.0f, TargetMax.Y/100.0f, TargetMax.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Center: (%.2f, %.2f, %.2f) m"), TargetCenter.X/100.0f, TargetCenter.Y/100.0f, TargetCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Highest Point: %.2f m"), HighestPoint / 100.0f);
	
	// Observer camera configuration
	UE_LOG(LogTemp, Warning, TEXT("OBSERVER CAMERA SETTINGS:"));
	UE_LOG(LogTemp, Warning, TEXT("  Height Above Target (setting): %.2f m"), HeightAboveTargetMeters);
	UE_LOG(LogTemp, Warning, TEXT("  Desired Pitch: %.2f degrees"), CameraPitchDegrees);
	
	// Calculated observer position
	UE_LOG(LogTemp, Warning, TEXT("OBSERVER CAMERA CALCULATED POSITION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Position (cm): (%.2f, %.2f, %.2f)"), 
		ObserverPosition.X, ObserverPosition.Y, ObserverPosition.Z);
	UE_LOG(LogTemp, Warning, TEXT("  Position (m): (%.2f, %.2f, %.2f)"), 
		ObserverPosition.X/100.0f, ObserverPosition.Y/100.0f, ObserverPosition.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Height Above Ground: %.2f m"), ObserverPosition.Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Height Above Target Highest: %.2f m"), HeightAboveTarget / 100.0f);
	
	// Actual actor state AFTER setting
	FVector ActualLocation = GetActorLocation();
	FRotator ActualRotation = GetActorRotation();
	
	UE_LOG(LogTemp, Warning, TEXT("OBSERVER CAMERA ACTUAL STATE (Actor):"));
	UE_LOG(LogTemp, Warning, TEXT("  Actual Location (cm): (%.2f, %.2f, %.2f)"), 
		ActualLocation.X, ActualLocation.Y, ActualLocation.Z);
	UE_LOG(LogTemp, Warning, TEXT("  Actual Location (m): (%.2f, %.2f, %.2f)"), 
		ActualLocation.X/100.0f, ActualLocation.Y/100.0f, ActualLocation.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Actual Rotation: Pitch=%.2f, Yaw=%.2f, Roll=%.2f"), 
		ActualRotation.Pitch, ActualRotation.Yaw, ActualRotation.Roll);
	
	// Camera component state
	if (UCineCameraComponent* CineCamera = GetCineCameraComponent())
	{
		FVector CamCompLocation = CineCamera->GetComponentLocation();
		FRotator CamCompRotation = CineCamera->GetComponentRotation();
		FVector CamCompForward = CineCamera->GetForwardVector();
		
		UE_LOG(LogTemp, Warning, TEXT("OBSERVER CAMERA COMPONENT STATE:"));
		UE_LOG(LogTemp, Warning, TEXT("  Component Location (cm): (%.2f, %.2f, %.2f)"), 
			CamCompLocation.X, CamCompLocation.Y, CamCompLocation.Z);
		UE_LOG(LogTemp, Warning, TEXT("  Component Location (m): (%.2f, %.2f, %.2f)"), 
			CamCompLocation.X/100.0f, CamCompLocation.Y/100.0f, CamCompLocation.Z/100.0f);
		UE_LOG(LogTemp, Warning, TEXT("  Component Rotation: Pitch=%.2f, Yaw=%.2f, Roll=%.2f"), 
			CamCompRotation.Pitch, CamCompRotation.Yaw, CamCompRotation.Roll);
		UE_LOG(LogTemp, Warning, TEXT("  Forward Vector: (%.3f, %.3f, %.3f)"), 
			CamCompForward.X, CamCompForward.Y, CamCompForward.Z);
		UE_LOG(LogTemp, Warning, TEXT("  Expected Forward for -90 pitch: (0.000, 0.000, -1.000)"));
	}
	
	// Distance verification
	float DistanceToTargetCenter = FVector::Dist(ActualLocation, TargetCenter);
	float HorizontalDistanceToTarget = FVector::Dist2D(ActualLocation, TargetCenter);
	float VerticalDistanceToTarget = ActualLocation.Z - TargetCenter.Z;
	
	UE_LOG(LogTemp, Warning, TEXT("DISTANCE VERIFICATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  3D Distance to Target Center: %.2f m"), DistanceToTargetCenter / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Horizontal Distance to Target: %.2f m (should be ~0)"), HorizontalDistanceToTarget / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Vertical Distance to Target Center: %.2f m"), VerticalDistanceToTarget / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Distance above Highest Point: %.2f m"), (ActualLocation.Z - HighestPoint) / 100.0f);
	
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
	
	// Draw bounding box
	DrawDebugBox(
		GetWorld(),
		TargetCenter,
		TargetBounds.GetExtent(),
		FColor::Green,
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
