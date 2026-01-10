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
	
	// Calculate scan height
	float TargetHeightAtPercent = TargetBounds.Min.Z + 
		((TargetBounds.Max.Z - TargetBounds.Min.Z) * (HeightPercent / 100.0f));
	
	// Calculate orbit radius (distance from center + clearance)
	float MaxHorizontalExtent = FMath::Max(TargetExtent.X, TargetExtent.Y);
	float ClearanceCm = DistanceMeters * 100.0f;
	float OrbitRadius = MaxHorizontalExtent + ClearanceCm;
	
	UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Starting discovery"));
	UE_LOG(LogTemp, Warning, TEXT("  Target: %s"), *TargetActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Radius: %.2f m"), OrbitRadius / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Scan Height: %.2f m"), TargetHeightAtPercent / 100.0f);
	
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
	UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Target found!"));
	UE_LOG(LogTemp, Warning, TEXT("  Hit Actor: %s"), *HitResult.GetActor()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Hit Distance: %.2f m"), HitResult.Distance / 100.0f);
	
	// Store first hit data
	bHasFirstHit = true;
	FirstHitResult = HitResult;
	FirstHitAngle = TargetFinderComponent ? TargetFinderComponent->GetFirstHitAngle() : 0.0f;
	FirstHitCameraPosition = GetActorLocation();
	FirstHitCameraRotation = GetActorRotation();
	
	UE_LOG(LogTemp, Warning, TEXT("  Hit Angle: %.1f°"), FirstHitAngle);
	UE_LOG(LogTemp, Warning, TEXT("  Camera Pos: %s"), *FirstHitCameraPosition.ToString());
	UE_LOG(LogTemp, Warning, TEXT("  Camera Rot: %s"), *FirstHitCameraRotation.ToString());
	
	TransitionToState(EMappingScannerState::Discovered);
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
