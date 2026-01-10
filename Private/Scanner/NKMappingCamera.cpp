// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKMappingCamera.h"
#include "Scanner/Components/NKTargetFinderComponent.h"
#include "Scanner/Components/NKLaserTracerComponent.h"
#include "Scanner/Components/NKCameraControllerComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

ANKMappingCamera::ANKMappingCamera(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TargetActor(nullptr)
	, CurrentState(EMappingScannerState::Idle)
	, bHasFirstHit(false)
	, FirstHitAngle(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Create components
	TargetFinderComponent = CreateDefaultSubobject<UNKTargetFinderComponent>(TEXT("TargetFinderComponent"));
	LaserTracerComponent = CreateDefaultSubobject<UNKLaserTracerComponent>(TEXT("LaserTracerComponent"));
	CameraControllerComponent = CreateDefaultSubobject<UNKCameraControllerComponent>(TEXT("CameraControllerComponent"));
}

void ANKMappingCamera::BeginPlay()
{
	Super::BeginPlay();
	
	// Bind to target finder events
	if (TargetFinderComponent)
	{
		TargetFinderComponent->GetOnTargetFoundEvent().AddDynamic(this, &ANKMappingCamera::OnTargetFound);
		TargetFinderComponent->GetOnDiscoveryFailedEvent().AddDynamic(this, &ANKMappingCamera::OnDiscoveryFailed);
	}
	
	TransitionToState(EMappingScannerState::Idle);
}

void ANKMappingCamera::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ANKMappingCamera::StartDiscovery()
{
	if (!TargetActor)
	{
		UE_LOG(LogTemp, Error, TEXT("ANKMappingCamera::StartDiscovery - No target actor configured"));
		return;
	}
	
	if (CurrentState != EMappingScannerState::Idle)
	{
		UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera::StartDiscovery - Scanner not idle (current state: %d)"), (int32)CurrentState);
		return;
	}
	
	if (!TargetFinderComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ANKMappingCamera::StartDiscovery - TargetFinderComponent missing"));
		return;
	}
	
	// Calculate target bounds and position
	FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	FVector TargetExtent = TargetBounds.GetExtent();
	FVector TargetMin = TargetBounds.Min;
	FVector TargetMax = TargetBounds.Max;
	
	// Calculate scan height
	float TargetHeightAtPercent = TargetBounds.Min.Z + 
		((TargetBounds.Max.Z - TargetBounds.Min.Z) * (HeightPercent / 100.0f));
	
	// Calculate orbit radius (distance from center + clearance)
	float MaxHorizontalExtent = FMath::Max(TargetExtent.X, TargetExtent.Y);
	float ClearanceCm = DistanceMeters * 100.0f;
	float OrbitRadius = MaxHorizontalExtent + ClearanceCm;
	
	// Get camera position
	FVector CameraPosition = GetActorLocation();
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Starting Discovery"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Log target info
	UE_LOG(LogTemp, Warning, TEXT("TARGET ACTOR:"));
	UE_LOG(LogTemp, Warning, TEXT("  Name: %s"), *TargetActor->GetName());
	
	// Log bounding box details
	UE_LOG(LogTemp, Warning, TEXT("BOUNDING BOX:"));
	UE_LOG(LogTemp, Warning, TEXT("  Min: X=%.2f Y=%.2f Z=%.2f (%.2fm, %.2fm, %.2fm)"), 
		TargetMin.X, TargetMin.Y, TargetMin.Z,
		TargetMin.X/100.0f, TargetMin.Y/100.0f, TargetMin.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Max: X=%.2f Y=%.2f Z=%.2f (%.2fm, %.2fm, %.2fm)"), 
		TargetMax.X, TargetMax.Y, TargetMax.Z,
		TargetMax.X/100.0f, TargetMax.Y/100.0f, TargetMax.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Center: X=%.2f Y=%.2f Z=%.2f (%.2fm, %.2fm, %.2fm)"), 
		TargetCenter.X, TargetCenter.Y, TargetCenter.Z,
		TargetCenter.X/100.0f, TargetCenter.Y/100.0f, TargetCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Extent: X=%.2f Y=%.2f Z=%.2f (%.2fm, %.2fm, %.2fm)"), 
		TargetExtent.X, TargetExtent.Y, TargetExtent.Z,
		TargetExtent.X/100.0f, TargetExtent.Y/100.0f, TargetExtent.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Size: %.2fm × %.2fm × %.2fm"), 
		(TargetMax.X - TargetMin.X)/100.0f, 
		(TargetMax.Y - TargetMin.Y)/100.0f, 
		(TargetMax.Z - TargetMin.Z)/100.0f);
	
	// Log camera position
	UE_LOG(LogTemp, Warning, TEXT("CAMERA POSITION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Current: X=%.2f Y=%.2f Z=%.2f (%.2fm, %.2fm, %.2fm)"), 
		CameraPosition.X, CameraPosition.Y, CameraPosition.Z,
		CameraPosition.X/100.0f, CameraPosition.Y/100.0f, CameraPosition.Z/100.0f);
	
	// Log shooting parameters
	UE_LOG(LogTemp, Warning, TEXT("SHOOTING PARAMETERS:"));
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Radius: %.2f cm (%.2f m)"), OrbitRadius, OrbitRadius/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Max Shooting Distance: %.2f cm (%.2f m)"), 
		LaserTracerComponent ? LaserTracerComponent->MaxRange : 0.0f,
		LaserTracerComponent ? LaserTracerComponent->MaxRange/100.0f : 0.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Scan Height: %.2f cm (%.2f m)"), TargetHeightAtPercent, TargetHeightAtPercent/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Height Percent: %.1f%%"), HeightPercent);
	UE_LOG(LogTemp, Warning, TEXT("  Distance from Target Center: %.2f m"), DistanceMeters);
	UE_LOG(LogTemp, Warning, TEXT("  Angular Step: %.1f°"), TargetFinderComponent ? TargetFinderComponent->AngularStepDegrees : 30.0f);
	
	// Calculate and log distance between camera and target center
	float DistanceToTarget = FVector::Dist(CameraPosition, TargetCenter);
	UE_LOG(LogTemp, Warning, TEXT("  Current Camera-to-Target Distance: %.2f cm (%.2f m)"), 
		DistanceToTarget, DistanceToTarget/100.0f);
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Start discovery
	TargetFinderComponent->StartDiscovery(TargetActor, OrbitRadius, TargetHeightAtPercent);
	
	TransitionToState(EMappingScannerState::Discovering);
}

void ANKMappingCamera::Stop()
{
	if (TargetFinderComponent && CurrentState == EMappingScannerState::Discovering)
	{
		TargetFinderComponent->StopDiscovery();
		TransitionToState(EMappingScannerState::DiscoveryCancelled);
	}
}

void ANKMappingCamera::ClearDiscoveryLines()
{
	if (LaserTracerComponent)
	{
		LaserTracerComponent->ClearLaserVisuals();
		UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Discovery lines cleared"));
	}
}

int32 ANKMappingCamera::GetDiscoveryShotCount() const
{
	return TargetFinderComponent ? TargetFinderComponent->GetShotCount() : 0;
}

float ANKMappingCamera::GetDiscoveryAngle() const
{
	return TargetFinderComponent ? TargetFinderComponent->GetCurrentAngle() : 0.0f;
}

float ANKMappingCamera::GetDiscoveryProgress() const
{
	return TargetFinderComponent ? TargetFinderComponent->GetProgressPercent() : 0.0f;
}

void ANKMappingCamera::OnTargetFound(FHitResult HitResult)
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera::OnTargetFound CALLED"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("  Hit Actor: %s"), HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("  Hit Distance: %.2f m"), HitResult.Distance / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Current State BEFORE transition: %d"), (int32)CurrentState);
	
	// Store first hit data
	bHasFirstHit = true;
	FirstHitResult = HitResult;
	FirstHitAngle = TargetFinderComponent ? TargetFinderComponent->GetFirstHitAngle() : 0.0f;
	FirstHitCameraPosition = GetActorLocation();
	FirstHitCameraRotation = GetActorRotation();
	
	UE_LOG(LogTemp, Warning, TEXT("  Hit Angle: %.1f°"), FirstHitAngle);
	UE_LOG(LogTemp, Warning, TEXT("  Camera Pos: %s"), *FirstHitCameraPosition.ToString());
	UE_LOG(LogTemp, Warning, TEXT("  Camera Rot: %s"), *FirstHitCameraRotation.ToString());
	
	UE_LOG(LogTemp, Warning, TEXT("  Calling TransitionToState(Discovered)..."));
	TransitionToState(EMappingScannerState::Discovered);
	UE_LOG(LogTemp, Warning, TEXT("  Current State AFTER transition: %d"), (int32)CurrentState);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void ANKMappingCamera::OnDiscoveryFailed()
{
	UE_LOG(LogTemp, Error, TEXT("ANKMappingCamera: Discovery failed - no target found"));
	
	TransitionToState(EMappingScannerState::DiscoveryFailed);
}

void ANKMappingCamera::TransitionToState(EMappingScannerState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: State transition %d -> %d"), (int32)CurrentState, (int32)NewState);
	
	CurrentState = NewState;
}
