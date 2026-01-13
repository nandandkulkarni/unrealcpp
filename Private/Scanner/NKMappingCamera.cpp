// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKMappingCamera.h"
#include "Scanner/Components/NKTargetFinderComponent.h"
#include "Scanner/Components/NKLaserTracerComponent.h"
#include "Scanner/Components/NKCameraControllerComponent.h"
#include "Scanner/Components/NKOrbitMapperComponent.h"
#include "Scanner/Components/NKRecordingCameraComponent.h"
#include "Scanner/NKOverheadCamera.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"

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
	OrbitMapperComponent = CreateDefaultSubobject<UNKOrbitMapperComponent>(TEXT("OrbitMapperComponent"));
	RecordingCameraComponent = CreateDefaultSubobject<UNKRecordingCameraComponent>(TEXT("RecordingCameraComponent"));
}

void ANKMappingCamera::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	// Make camera mesh visible in game so overhead camera can see it
	// ACineCameraActor has a mesh component that represents the camera visually
	TArray<UActorComponent*> MeshComponents;
	GetComponents(UStaticMeshComponent::StaticClass(), MeshComponents);
	
	for (UActorComponent* Component : MeshComponents)
	{
		if (UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Component))
		{
			MeshComp->SetHiddenInGame(false);
			UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Camera mesh '%s' set to visible in game"), 
				*MeshComp->GetName());
		}
	}
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
	
	// Bind to orbit mapper events
	if (OrbitMapperComponent)
	{
		OrbitMapperComponent->OnMappingComplete.AddDynamic(this, &ANKMappingCamera::OnMappingComplete);
		OrbitMapperComponent->OnMappingFailed.AddDynamic(this, &ANKMappingCamera::OnMappingFailed);
	}
	
	// Spawn overhead camera if enabled
	if (bSpawnOverheadCamera)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		OverheadCameraActor = GetWorld()->SpawnActor<ANKOverheadCamera>(
			ANKOverheadCamera::StaticClass(),
			GetActorLocation(),
			FRotator::ZeroRotator,
			SpawnParams
		);
		
		if (OverheadCameraActor)
		{
			// Attach to this actor (follows main camera movements)
			OverheadCameraActor->AttachToActor(this, 
				FAttachmentTransformRules::KeepRelativeTransform);
			
			// Set height offset above main camera
			float HeightOffsetCm = OverheadCameraHeightMeters * 100.0f;
			OverheadCameraActor->SetActorRelativeLocation(FVector(0.0f, 0.0f, HeightOffsetCm));
			
			// Configure overhead camera's height property
			OverheadCameraActor->HeightOffsetMeters = OverheadCameraHeightMeters;
			
			UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Spawned overhead camera at %.1fm above"), 
				OverheadCameraHeightMeters);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("ANKMappingCamera: Failed to spawn overhead camera!"));
		}
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
	
	// Calculate orbit radius and scan height based on camera position mode
	float ScanHeight;
	
	if (CameraPositionMode == ECameraPositionMode::Center)
	{
		// Center mode: Fixed position at world origin (0,0) with specified height
		ScanHeight = CenterModeHeightMeters * 100.0f;  // Convert meters to cm
		
		UE_LOG(LogTemp, Warning, TEXT("Camera Position Mode: CENTER at (0, 0, %.2fm)"), CenterModeHeightMeters);
	}
	else  // Relative mode
	{
		// Relative mode: Auto-position camera at specified distance from target
		
		// Calculate bounding sphere radius (farthest point from center)
		float BoundingSphereRadius = TargetExtent.Size();  // Distance from center to corner
		
		// Calculate desired orbit radius (bounding sphere + clearance)
		float ClearanceDistance = DistanceMeters * 100.0f;  // Convert meters to cm
		float OrbitRadius = BoundingSphereRadius + ClearanceDistance;
		
		// Get current camera position and direction to target
		FVector CurrentCameraPos = GetActorLocation();
		FVector DirectionToCamera = (CurrentCameraPos - TargetCenter);
		
		// Calculate 2D horizontal direction (ignore Z for now)
		FVector2D HorizontalDirection = FVector2D(DirectionToCamera.X, DirectionToCamera.Y);
		float CurrentHorizontalDistance = HorizontalDirection.Size();
		
		if (CurrentHorizontalDistance > 0.1f)  // Avoid division by zero
		{
			HorizontalDirection.Normalize();
		}
		else
		{
			// Camera is directly above/below target, default to North
			HorizontalDirection = FVector2D(0.0f, 1.0f);
		}
		
		// Calculate new XY position at desired orbit radius
		FVector2D TargetCenter2D = FVector2D(TargetCenter.X, TargetCenter.Y);
		FVector2D NewXY = TargetCenter2D + (HorizontalDirection * OrbitRadius);
		
		// Calculate scan height at specified percentage
		ScanHeight = TargetBounds.Min.Z + ((TargetBounds.Max.Z - TargetBounds.Min.Z) * (HeightPercent / 100.0f));
		
		// Set camera to new position
		FVector NewCameraPosition = FVector(NewXY.X, NewXY.Y, ScanHeight);
		SetActorLocation(NewCameraPosition);
		
		UE_LOG(LogTemp, Warning, TEXT("Camera Position Mode: RELATIVE"));
		UE_LOG(LogTemp, Warning, TEXT("  Bounding Sphere Radius: %.2fm"), BoundingSphereRadius / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("  Clearance Distance: %.2fm"), ClearanceDistance / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("  Orbit Radius: %.2fm"), OrbitRadius / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("  Height: %.1f%% (%.2fm)"), HeightPercent, ScanHeight / 100.0f);
		UE_LOG(LogTemp, Warning, TEXT("  Camera moved from (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)"),
			CurrentCameraPos.X / 100.0f, CurrentCameraPos.Y / 100.0f, CurrentCameraPos.Z / 100.0f,
			NewCameraPosition.X / 100.0f, NewCameraPosition.Y / 100.0f, NewCameraPosition.Z / 100.0f);
	}
	
	// ===== Debug Visualization =====
	
	// Draw cyan sphere at farthest bounding box point
	// Calculate a point on the bounding sphere (use current camera direction)
	FVector CurrentCameraPos = GetActorLocation();
	FVector DirectionToCamera = (CurrentCameraPos - TargetCenter).GetSafeNormal();
	float BoundingSphereRadius = TargetExtent.Size();
	FVector FarthestPoint = TargetCenter + (DirectionToCamera * BoundingSphereRadius);
	
	DrawDebugSphere(
		GetWorld(),
		FarthestPoint,
		50.0f,  // 50cm radius sphere
		16,  // Segments
		FColor::Cyan,
		false,  // Not persistent (will fade)
		60.0f,  // 60 second lifetime
		0,  // Depth priority
		5.0f  // Thickness
	);
	
	// Draw orbit circle at scan height
	// Circle has farthest point on its circumference and target center as center
	float OrbitRadius = (CameraPositionMode == ECameraPositionMode::Center) ? 
		FVector::Dist2D(CurrentCameraPos, TargetCenter) : 
		BoundingSphereRadius + (DistanceMeters * 100.0f);
	
	DrawDebugCircle(
		GetWorld(),
		TargetCenter,
		OrbitRadius,
		64,  // Segments for smooth circle
		FColor::Cyan,
		false,  // Not persistent
		60.0f,  // 60 second lifetime
		0,  // Depth priority
		10.0f,  // Thickness
		FVector(0, 1, 0),  // Y-axis (for horizontal circle)
		FVector(1, 0, 0),  // X-axis
		false  // Don't draw axis
	);
	
	UE_LOG(LogTemp, Warning, TEXT("DEBUG VISUALIZATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Cyan sphere at farthest point: (%.1f, %.1f, %.1f)"),
		FarthestPoint.X / 100.0f, FarthestPoint.Y / 100.0f, FarthestPoint.Z / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Cyan circle radius: %.2fm"), OrbitRadius / 100.0f);
	
	// Auto-configure laser tracer based on target type
	bool bIsLandscape = IsTargetLandscape();
	if (LaserTracerComponent)
	{
		if (bIsLandscape)
		{
			// Landscapes require WorldStatic channel
			LaserTracerComponent->TraceChannel = ECC_WorldStatic;
			LaserTracerComponent->bUseComplexCollision = true;
			UE_LOG(LogTemp, Warning, 
				TEXT("Target is LANDSCAPE - configured for WorldStatic trace channel"));
		}
		else
		{
			// Static meshes work with either, but Visibility is faster
			LaserTracerComponent->TraceChannel = ECC_WorldStatic;  // Use WorldStatic for both
			LaserTracerComponent->bUseComplexCollision = true;
			UE_LOG(LogTemp, Warning, 
				TEXT("Target is STATIC MESH - configured for WorldStatic trace channel"));
		}
		
		// Calculate required laser range to guarantee hitting the target
		// Maximum distance from orbit to any point on the target
		FVector TargetSize = TargetBounds.GetSize();
		float MaxTargetDimension = FMath::Max3(TargetSize.X, TargetSize.Y, TargetSize.Z);
		float DiagonalDistance = TargetSize.Size();  // 3D diagonal
		
		// TEST: Set to very large range (effectively infinite) to rule out range issues
		float RequiredRange = 10000000.0f;  // 100km - effectively infinite for testing
		
		LaserTracerComponent->MaxRange = RequiredRange;
		
		UE_LOG(LogTemp, Warning, 
			TEXT("🔬 TEST MODE: Laser range set to %.2fkm (INFINITE for testing)"),
			RequiredRange/100000.0f);
	}
	
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
	
	// Log target collision and physics properties
	UE_LOG(LogTemp, Warning, TEXT("TARGET COLLISION PROPERTIES:"));
	UE_LOG(LogTemp, Warning, TEXT("  Class: %s"), *TargetActor->GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Actor Tags: %d"), TargetActor->Tags.Num());
	
	// Check all components for collision settings
	TArray<UActorComponent*> Components;
	TargetActor->GetComponents(Components);
	
	int32 CollisionComponentCount = 0;
	for (UActorComponent* Component : Components)
	{
		if (UPrimitiveComponent* PrimComp = Cast<UPrimitiveComponent>(Component))
		{
			CollisionComponentCount++;
			UE_LOG(LogTemp, Warning, TEXT("  Component #%d: %s (%s)"), 
				CollisionComponentCount,
				*PrimComp->GetName(),
				*PrimComp->GetClass()->GetName());
			UE_LOG(LogTemp, Warning, TEXT("    Collision Enabled: %s"), 
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionEnabled"), PrimComp->GetCollisionEnabled()));
			UE_LOG(LogTemp, Warning, TEXT("    Collision Object Type: %s"),
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionChannel"), PrimComp->GetCollisionObjectType()));
			UE_LOG(LogTemp, Warning, TEXT("    Simulate Physics: %s"), PrimComp->IsSimulatingPhysics() ? TEXT("YES") : TEXT("NO"));
			UE_LOG(LogTemp, Warning, TEXT("    Generate Overlap Events: %s"), PrimComp->GetGenerateOverlapEvents() ? TEXT("YES") : TEXT("NO"));
			
			// Log collision response for important channels
			UE_LOG(LogTemp, Warning, TEXT("    Collision Responses:"));
			UE_LOG(LogTemp, Warning, TEXT("      WorldStatic: %s"), 
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionResponse"), PrimComp->GetCollisionResponseToChannel(ECC_WorldStatic)));
			UE_LOG(LogTemp, Warning, TEXT("      WorldDynamic: %s"), 
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionResponse"), PrimComp->GetCollisionResponseToChannel(ECC_WorldDynamic)));
			UE_LOG(LogTemp, Warning, TEXT("      Visibility: %s"), 
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionResponse"), PrimComp->GetCollisionResponseToChannel(ECC_Visibility)));
			UE_LOG(LogTemp, Warning, TEXT("      Camera: %s"), 
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionResponse"), PrimComp->GetCollisionResponseToChannel(ECC_Camera)));
			
			// Check if it has collision geometry
			if (UStaticMeshComponent* StaticMeshComp = Cast<UStaticMeshComponent>(PrimComp))
			{
				UE_LOG(LogTemp, Warning, TEXT("    Static Mesh: %s"), 
					StaticMeshComp->GetStaticMesh() ? *StaticMeshComp->GetStaticMesh()->GetName() : TEXT("NULL"));
			}
		}
	}
	
	if (CollisionComponentCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("  ⚠️ NO COLLISION COMPONENTS FOUND ON TARGET!"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("  Total Collision Components: %d"), CollisionComponentCount);
	}
	
	// Log camera position
	UE_LOG(LogTemp, Warning, TEXT("CAMERA POSITION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Current: X=%.2f Y=%.2f Z=%.2f (%.2fm, %.2fm, %.2fm)"), 
		CameraPosition.X, CameraPosition.Y, CameraPosition.Z,
		CameraPosition.X/100.0f, CameraPosition.Y/100.0f, CameraPosition.Z/100.0f);
	
	// Log shooting parameters
	UE_LOG(LogTemp, Warning, TEXT("SHOOTING PARAMETERS:"));
	UE_LOG(LogTemp, Warning, TEXT("  Camera Mode: %s"), 
		CameraPositionMode == ECameraPositionMode::Center ? TEXT("CENTER") : TEXT("RELATIVE"));
	UE_LOG(LogTemp, Warning, TEXT("  Scan Height: %.2f cm (%.2f m)"), ScanHeight, ScanHeight/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Max Shooting Distance: %.2f cm (%.2f m)"), 
		LaserTracerComponent ? LaserTracerComponent->MaxRange : 0.0f,
		LaserTracerComponent ? LaserTracerComponent->MaxRange/100.0f : 0.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Angular Step: %.1f°"), TargetFinderComponent ? TargetFinderComponent->AngularStepDegrees : 30.0f);
	
	// Calculate and log distance between camera and target center
	float DistanceToTarget = FVector::Dist(CameraPosition, TargetCenter);
	UE_LOG(LogTemp, Warning, TEXT("  Current Camera-to-Target Distance: %.2f cm (%.2f m)"), 
		DistanceToTarget, DistanceToTarget/100.0f);
	
	// Log trace configuration
	UE_LOG(LogTemp, Warning, TEXT("TRACE CONFIGURATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Target Type: %s"), bIsLandscape ? TEXT("LANDSCAPE") : TEXT("STATIC MESH"));
	if (LaserTracerComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("  Primary Channel: %s"), 
			*UEnum::GetValueAsString(TEXT("Engine.ECollisionChannel"), LaserTracerComponent->TraceChannel));
		UE_LOG(LogTemp, Warning, TEXT("  Complex Collision: %s"), 
			LaserTracerComponent->bUseComplexCollision ? TEXT("YES") : TEXT("NO"));
		UE_LOG(LogTemp, Warning, TEXT("  Fallback Enabled: %s"), 
			LaserTracerComponent->bUseFallbackChannel ? TEXT("YES") : TEXT("NO"));
		if (LaserTracerComponent->bUseFallbackChannel)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Fallback Channel: %s"),
				*UEnum::GetValueAsString(TEXT("Engine.ECollisionChannel"), LaserTracerComponent->FallbackTraceChannel));
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Start discovery - camera stays in place and rotates 360°
	TargetFinderComponent->StartDiscovery(TargetActor, ScanHeight);
	
	TransitionToState(EMappingScannerState::Discovering);
}

void ANKMappingCamera::Stop()
{
	if (TargetFinderComponent && CurrentState == EMappingScannerState::Discovering)
	{
		TargetFinderComponent->StopDiscovery();
		TransitionToState(EMappingScannerState::DiscoveryCancelled);
	}
	else if (OrbitMapperComponent && CurrentState == EMappingScannerState::Mapping)
	{
		OrbitMapperComponent->StopMapping();
		UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Mapping stopped"));
	}
}

void ANKMappingCamera::ClearDiscoveryLines()
{
	if (LaserTracerComponent)
	{
		LaserTracerComponent->ClearLaserVisuals();
		UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera: Discovery lines cleared (configuration preserved)"));
	}
}

void ANKMappingCamera::StartMapping()
{
	if (!DiscoveryConfig.IsValid())
	{
		UE_LOG(LogTemp, Error, 
			TEXT("ANKMappingCamera::StartMapping - No valid discovery configuration! Run discovery first."));
		return;
	}
	
	if (CurrentState != EMappingScannerState::Discovered)
	{
		UE_LOG(LogTemp, Warning, 
			TEXT("ANKMappingCamera::StartMapping - Not in Discovered state (current: %d)"), (int32)CurrentState);
		return;
	}
	
	if (!OrbitMapperComponent || !LaserTracerComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("ANKMappingCamera::StartMapping - Missing required components!"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("Starting ASYNC Mapping Phase"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("REUSING DISCOVERY CONFIGURATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Target: %s"), *DiscoveryConfig.TargetActor->GetName());
	UE_LOG(LogTemp, Warning, TEXT("  Type: %s"), 
		DiscoveryConfig.bIsLandscape ? TEXT("LANDSCAPE") : TEXT("STATIC MESH"));
	UE_LOG(LogTemp, Warning, TEXT("  Trace Channel: %s"), 
		*UEnum::GetValueAsString(TEXT("Engine.ECollisionChannel"), DiscoveryConfig.WorkingTraceChannel));
	UE_LOG(LogTemp, Warning, TEXT("  Complex Collision: %s"), 
		DiscoveryConfig.bUseComplexCollision ? TEXT("YES") : TEXT("NO"));
	UE_LOG(LogTemp, Warning, TEXT("  Scan Height: %.2fm"), DiscoveryConfig.ScanHeight / 100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Starting from First Hit: %.1f°"), DiscoveryConfig.FirstHitAngle);
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	
	// Apply the SAME configuration that worked in discovery
	if (LaserTracerComponent)
	{
		LaserTracerComponent->TraceChannel = DiscoveryConfig.WorkingTraceChannel;
		LaserTracerComponent->bUseComplexCollision = DiscoveryConfig.bUseComplexCollision;
		LaserTracerComponent->MaxRange = DiscoveryConfig.MaxTraceRange;
		UE_LOG(LogTemp, Warning, TEXT("Laser tracer configured with proven settings"));
	}
	
	// Calculate orbit parameters
	FVector TargetCenter = DiscoveryConfig.TargetBounds.GetCenter();
	FVector OrbitCenter = FVector(TargetCenter.X, TargetCenter.Y, DiscoveryConfig.ScanHeight);
	
	// Calculate orbit radius from discovery first hit
	float OrbitRadius = FVector::Dist2D(DiscoveryConfig.CameraPositionAtHit, TargetCenter);
	
	UE_LOG(LogTemp, Warning, TEXT("ORBIT MAPPING CONFIGURATION:"));
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Center: (%.2f, %.2f, %.2f) m"),
		OrbitCenter.X/100.0f, OrbitCenter.Y/100.0f, OrbitCenter.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Orbit Radius: %.2f m"), OrbitRadius/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("  Start Angle: %.1f°"), DiscoveryConfig.FirstHitAngle);
	UE_LOG(LogTemp, Warning, TEXT("  Angular Step: %.1f°"), OrbitMapperComponent->AngularStepDegrees);
	
	// Configure and start orbit mapper
	OrbitMapperComponent->AngularStepDegrees = 5.0f;  // 5 degrees per step
	OrbitMapperComponent->ShotDelay = 0.0f;  // Shoot every tick
	OrbitMapperComponent->bDrawDebugVisuals = true;
	
	OrbitMapperComponent->StartMapping(
		DiscoveryConfig.TargetActor,
		OrbitCenter,
		OrbitRadius,
		DiscoveryConfig.ScanHeight,
		DiscoveryConfig.FirstHitAngle,
		LaserTracerComponent
	);
	
	// Transition to mapping state
	TransitionToState(EMappingScannerState::Mapping);
	
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("Async Orbit Mapping Started!"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
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

int32 ANKMappingCamera::GetMappingShotCount() const
{
	return OrbitMapperComponent ? OrbitMapperComponent->GetShotCount() : 0;
}

float ANKMappingCamera::GetMappingAngle() const
{
	return OrbitMapperComponent ? OrbitMapperComponent->GetCurrentAngle() : 0.0f;
}

float ANKMappingCamera::GetMappingProgress() const
{
	return OrbitMapperComponent ? OrbitMapperComponent->GetProgressPercent() : 0.0f;
}

int32 ANKMappingCamera::GetMappingHitCount() const
{
	return OrbitMapperComponent ? OrbitMapperComponent->GetHitCount() : 0;
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
	
	// 🎯 PERSIST DISCOVERY CONFIGURATION FOR MAPPING
	UE_LOG(LogTemp, Warning, TEXT("  Persisting discovery configuration..."));
	
	DiscoveryConfig.TargetActor = TargetActor;
	DiscoveryConfig.bIsLandscape = IsTargetLandscape();
	DiscoveryConfig.TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	
	// Store working trace configuration
	if (LaserTracerComponent)
	{
		DiscoveryConfig.WorkingTraceChannel = LaserTracerComponent->TraceChannel;
		DiscoveryConfig.bUseComplexCollision = LaserTracerComponent->bUseComplexCollision;
		DiscoveryConfig.MaxTraceRange = LaserTracerComponent->MaxRange;
	}
	
	// Store orbit parameters (recalculate from current setup)
	FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	
	DiscoveryConfig.OrbitRadius = 0.0f;  // Not used - camera stays in place
	DiscoveryConfig.OrbitCenter = FVector(TargetCenter.X, TargetCenter.Y, 
		CenterModeHeightMeters * 100.0f);  // Use configured height
	DiscoveryConfig.ScanHeight = CameraPositionMode == ECameraPositionMode::Center ? 
		CenterModeHeightMeters * 100.0f : 
		TargetBounds.Min.Z + ((TargetBounds.Max.Z - TargetBounds.Min.Z) * (HeightPercent / 100.0f));
	
	// Store first hit data
	DiscoveryConfig.FirstHitLocation = HitResult.Location;
	DiscoveryConfig.FirstHitAngle = FirstHitAngle;
	DiscoveryConfig.CameraPositionAtHit = FirstHitCameraPosition;
	DiscoveryConfig.CameraRotationAtHit = FirstHitCameraRotation;
	
	UE_LOG(LogTemp, Warning, TEXT("  ✅ Configuration persisted for mapping phase"));
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

void ANKMappingCamera::OnMappingComplete()
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("ANKMappingCamera::OnMappingComplete CALLED"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("  Total Shots: %d"), GetMappingShotCount());
	UE_LOG(LogTemp, Warning, TEXT("  Total Hits: %d"), GetMappingHitCount());
	
	TransitionToState(EMappingScannerState::Complete);
	
	UE_LOG(LogTemp, Warning, TEXT("  State transitioned to Complete"));
	UE_LOG(LogTemp, Warning, TEXT("  Recording playback can now be started manually via HUD button"));
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void ANKMappingCamera::StartRecordingPlayback()
{
	if (!RecordingCameraComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("NKMappingCamera: No RecordingCameraComponent!"));
		return;
	}
	
	if (!OrbitMapperComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("NKMappingCamera: No OrbitMapperComponent!"));
		return;
	}
	
	// Get hit points from orbit mapper using the getter method
	const TArray<FVector>& HitPoints = OrbitMapperComponent->GetMappingHitPoints();
	
	if (HitPoints.Num() < 2)
	{
		UE_LOG(LogTemp, Error, TEXT("NKMappingCamera: Not enough hit points for recording playback (need at least 2, have %d)"), HitPoints.Num());
		return;
	}
	
	// Configure recording camera
	RecordingCameraComponent->RecordingTargetActor = TargetActor;
	
	// Start playback
	RecordingCameraComponent->StartPlayback(HitPoints);
	
	UE_LOG(LogTemp, Warning, TEXT("✅ Recording playback started with %d hit points"), HitPoints.Num());
}

void ANKMappingCamera::StopRecordingPlayback()
{
	if (RecordingCameraComponent)
	{
		RecordingCameraComponent->StopPlayback();
	}
}

float ANKMappingCamera::GetRecordingProgress() const
{
	if (RecordingCameraComponent)
	{
		return RecordingCameraComponent->GetProgress();
	}
	return 0.0f;
}

bool ANKMappingCamera::IsRecordingPlaying() const
{
	if (RecordingCameraComponent)
	{
		return RecordingCameraComponent->IsPlaying();
	}
	return false;
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

bool ANKMappingCamera::IsTargetLandscape() const
{
	if (!TargetActor)
	{
		return false;
	}
	
	// Check if actor class name contains "Landscape"
	FString ClassName = TargetActor->GetClass()->GetName();
	return ClassName.Contains(TEXT("Landscape"));
}
