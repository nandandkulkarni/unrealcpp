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
	
	// Calculate optimal height dynamically
	float CalculatedHeight = CalculateOptimalHeight();
	
	// Use calculated height instead of the configured HeightAboveTargetMeters
	// (unless user has set a specific height they want to override)
	float HeightToUse = CalculatedHeight;
	
	UE_LOG(LogTemp, Warning, TEXT("Observer Camera Height Selection:"));
	UE_LOG(LogTemp, Warning, TEXT("  Configured Height: %.2f m"), HeightAboveTargetMeters);
	UE_LOG(LogTemp, Warning, TEXT("  Calculated Optimal Height: %.2f m"), CalculatedHeight);
	UE_LOG(LogTemp, Warning, TEXT("  Using: CALCULATED (%.2f m)"), HeightToUse);
	
	// Get target bounding box
	FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	FVector TargetMin = TargetBounds.Min;
	FVector TargetMax = TargetBounds.Max;
	
	// Calculate observer position
	float HighestPoint = TargetMax.Z;
	float HeightAboveTarget = HeightToUse * 100.0f;  // Convert to cm
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
		
		// Set wide-angle FOV for better coverage
		// Set focal length to achieve desired FOV (shorter focal length = wider FOV)
		// For 90° FOV with 24.89mm sensor: focal length ≈ 14mm
		CineCamera->CurrentFocalLength = 14.0f;  // Ultra-wide lens
		
		UE_LOG(LogTemp, Warning, TEXT("Observer Camera Lens Settings:"));
		UE_LOG(LogTemp, Warning, TEXT("  Focal Length: %.1f mm (ultra-wide)"), CineCamera->CurrentFocalLength);
		UE_LOG(LogTemp, Warning, TEXT("  Approximate FOV: ~90-100°"));
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
		true,  // Persistent - never disappears
		-1.0f,  // Infinite lifetime
		0,
		5.0f
	);
	
	// Draw line from observer to target center
	DrawDebugLine(
		GetWorld(),
		ObserverPosition,
		TargetCenter,
		FColor::Yellow,
		true,  // Persistent
		-1.0f,  // Infinite lifetime
		0,
		3.0f
	);
	
	// Draw bounding box
	DrawDebugBox(
		GetWorld(),
		TargetCenter,
		TargetBounds.GetExtent(),
		FColor::Green,
		true,  // Persistent
		-1.0f,  // Infinite lifetime
		0,
		3.0f
	);
	
	// Draw circle at the orbit height (where mapping camera travels)
	// Calculate bounding sphere radius (farthest point from center)
	FVector TargetExtent = TargetBounds.GetExtent();
	float BoundingSphereRadius = TargetExtent.Size();  // 3D diagonal distance from center to corner
	float HorizontalRadius = FVector2D(TargetExtent.X, TargetExtent.Y).Size();  // 2D horizontal radius
	
	// Calculate the orbit height (same as mapping camera scan height)
	// Default to 50% of target height if not specified
	float OrbitHeight = TargetCenter.Z;  // Use target center height as orbit level
	
	// Draw cyan circle at ORBIT HEIGHT (where mapping camera will travel)
	// This circle represents the orbit path the mapping camera will take
	FVector CircleCenter = FVector(TargetCenter.X, TargetCenter.Y, OrbitHeight);
	
	DrawDebugCircle(
		GetWorld(),
		CircleCenter,
		BoundingSphereRadius,  // Radius is the bounding sphere radius (mapping camera orbit)
		64,  // Segments for smooth circle
		FColor::Cyan,
		true,  // Persistent - never disappears
		-1.0f,  // Infinite lifetime
		0,  // Depth priority
		8.0f,  // Thickness
		FVector(0, 1, 0),  // Y-axis (for horizontal circle)
		FVector(1, 0, 0),  // X-axis
		false  // Don't draw axis
	);
	
	// Also draw a smaller circle at horizontal radius only (ignoring Z extent)
	DrawDebugCircle(
		GetWorld(),
		CircleCenter,
		HorizontalRadius,  // Just the XY radius (horizontal footprint)
		64,  // Segments
		FColor::Orange,
		true,  // Persistent - never disappears
		-1.0f,  // Infinite lifetime
		0,
		5.0f,
		FVector(0, 1, 0),
		FVector(1, 0, 0),
		false
	);
	
	UE_LOG(LogTemp, Warning, TEXT("DEBUG VISUALIZATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Yellow sphere at observer position"));
	UE_LOG(LogTemp, Warning, TEXT("  Yellow line from observer to target center"));
	UE_LOG(LogTemp, Warning, TEXT("  Green box showing target bounding box"));
	UE_LOG(LogTemp, Warning, TEXT("  Cyan circle at orbit height (mapping camera orbit): %.2f m radius at %.2f m height"), 
		BoundingSphereRadius / 100.0f, OrbitHeight / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Orange circle at orbit height (horizontal footprint): %.2f m radius"), HorizontalRadius / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Circle center: (%.2f, %.2f, %.2f) m"), 
		CircleCenter.X/100.0f, CircleCenter.Y/100.0f, CircleCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  ALL DEBUG SHAPES ARE PERSISTENT (never disappear)"));
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

float ANKObserverCamera::CalculateOptimalHeight() const
{
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculateOptimalHeight: No target actor - returning default 100m"));
		return 100.0f;  // Default 100m
	}
	
	// Get target bounding box
	FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	FVector TargetExtent = TargetBounds.GetExtent();
	float HighestPoint = TargetBounds.Max.Z;
	
	// Calculate orbit radius (bounding sphere radius)
	// This is the radius of the circle the mapping camera will travel
	float BoundingSphereRadius = TargetExtent.Size();  // 3D diagonal from center to corner
	
	// Calculate minimum height needed to see the entire orbit circle
	// Formula: Height = Radius / tan(FOV/2)
	// With wide FOV (90°), tan(45°) = 1, so Height = Radius
	float HalfFOV = ObserverCameraFOV / 2.0f;  // 45° for 90° FOV
	float HalfFOVRadians = FMath::DegreesToRadians(HalfFOV);
	float TanHalfFOV = FMath::Tan(HalfFOVRadians);
	
	// Minimum height to see orbit circle (from orbit level, not ground)
	float MinHeightAboveOrbit = BoundingSphereRadius / TanHalfFOV;
	
	// Add safety margin
	float RecommendedHeightAboveOrbit = MinHeightAboveOrbit * HeightSafetyMargin;
	
	// Calculate absolute Z position
	// Orbit is at target center Z, camera needs to be above that
	float OrbitHeight = TargetCenter.Z;
	float AbsoluteCameraZ = OrbitHeight + RecommendedHeightAboveOrbit;
	
	// Ensure camera is ALWAYS above the mountain's highest point
	float MinAboveHighestPoint = 1000.0f;  // At least 10m (1000cm) above highest point
	float MinAllowedZ = HighestPoint + MinAboveHighestPoint;
	
	if (AbsoluteCameraZ < MinAllowedZ)
	{
		UE_LOG(LogTemp, Warning, TEXT("CalculateOptimalHeight: Calculated Z (%.2f m) is below highest point + margin, adjusting to %.2f m"),
			AbsoluteCameraZ / 100.0f, MinAllowedZ / 100.0f);
		AbsoluteCameraZ = MinAllowedZ;
	}
	
	// Convert to "height above highest point" for return value
	float HeightAboveHighestPoint = AbsoluteCameraZ - HighestPoint;
	
	// Log the calculation
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("CALCULATE OPTIMAL OBSERVER HEIGHT"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("INPUTS:"));
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Radius (bounding sphere): %.2f m"), BoundingSphereRadius / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Camera FOV: %.1f°"), ObserverCameraFOV);
	UE_LOG(LogTemp, Warning, TEXT("  Target Highest Point: %.2f m"), HighestPoint / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Target Center Z: %.2f m"), TargetCenter.Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("CALCULATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Half FOV: %.1f°"), HalfFOV);
	UE_LOG(LogTemp, Warning, TEXT("  tan(Half FOV): %.3f"), TanHalfFOV);
	UE_LOG(LogTemp, Warning, TEXT("  Min Height Above Orbit: %.2f m"), MinHeightAboveOrbit / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Recommended Height (with %.1fx margin): %.2f m"), HeightSafetyMargin, RecommendedHeightAboveOrbit / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("RESULT:"));
	UE_LOG(LogTemp, Warning, TEXT("  Absolute Camera Z: %.2f m"), AbsoluteCameraZ / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Height Above Highest Point: %.2f m"), HeightAboveHighestPoint / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Can see orbit circle: %s"), AbsoluteCameraZ >= (OrbitHeight + MinHeightAboveOrbit) ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Return height in meters (above highest point)
	return HeightAboveHighestPoint / 100.0f;
}
