// Fill out your copyright notice in the Description page of Project Settings.

#include "NKScannerCameraActor.h"
#include "CineCameraComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "JsonObjectConverter.h"
#include "HAL/PlatformFileManager.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"


ANKScannerCameraActor::ANKScannerCameraActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// ===== CAMERA VISIBILITY =====
	// Ensure camera is visible during gameplay (important for visualization)
	SetActorHiddenInGame(false);
	
	// Initialize logging properties
	bEnableVerboseLogging = true;
	bLogToFile = true;
	bLogEveryFrame = false;
	CustomLogFilePath = TEXT("");
	bLogFileInitialized = false;

	// Initialize scanner properties
	ScanRange = 1000.0f;
	ScanSpeed = 1.0f;
	bScannerEnabled = true;
	ScanProgress = 0.0f;
	bIsScanning = false;

	// Initialize laser properties
	LaserMaxRange = 10000.0f;
	bShowLaserBeam = true;
	LaserColor = FColor::Red;
	LaserThickness = 2.0f;
	bContinuousLaserShoot = false;
	LaserTraceChannel = ECC_Visibility;

	// Initialize hit result properties
	LastHitLocation = FVector::ZeroVector;
	LastHitNormal = FVector::ZeroVector;
	LastHitActor = nullptr;
	LastHitComponent = nullptr;
	LastHitDistance = 0.0f;
	bLastShotHit = false;
	LastHitPhysicalMaterial = NAME_None;

	// Initialize cinematic scan properties
	CinematicTargetLandscape = nullptr;
	CinematicHeightPercent = 50.0f;
	CinematicDistanceMeters = 50.0f;
	CinematicAngularStepDegrees = 10.0f;  // Increased from 1.0 to 10.0 for better performance
	ValidationAngularStepDegrees = 30.0f;  // Step 3 validation uses 30° (only 12 traces!)
	MappingUpdateInterval = 0.1f;  // Update every 100ms (10 times per second)
	CinematicJSONOutputPath = FString();

	// Initialize playback properties
	PlaybackSpeedMultiplier = 1.0f;
	bLoopPlayback = false;
	
	// Initialize HUD settings
	bShowDebugHUD = true;  // Show HUD by default
	
	// Initialize automation settings
	bAutoStartDiscovery = false;  // Manual start by default - user must click button
	bAutoStartMapping = false;     // Manual start mapping - user must click button
	bAutoResetAfterMapping = false; // Don't auto-reset by default
	
	// Initialize debug visualization settings
	bShowScanPointSpheres = true;   // Show spheres at scan points
	bShowScanLines = true;           // Show lines from camera to points
	bShowOrbitPath = true;           // Show orbit circle
	bShowTargetBoundingBox = true;   // Show target bounding box
	ScanPointSphereSize = 15.0f;     // 15cm spheres
	DebugVisualsLifetime = 60.0f;    // Keep visible for 60 seconds
	ScanPointColor = FColor::Cyan;   // Cyan spheres
	ScanLineColor = FColor::Yellow;  // Yellow lines
	BoundingBoxColor = FColor::Orange; // Orange bounding box

	// Initialize internal state
	bIsCinematicScanActive = false;
	CurrentOrbitAngle = 0.0f;
	CinematicLookAtTarget = FVector::ZeroVector;
	CinematicOrbitCenter = FVector::ZeroVector;
	CinematicOrbitRadius = 0.0f;
	CinematicOrbitHeight = 0.0f;
	CurrentScanFrameNumber = 0;
	CinematicScanElapsedTime = 0.0f;
	CinematicScanUpdateAccumulator = 0.0f;
	
	// Initialize scanner state machine
	ScannerState = EScannerState::Idle;
	
	// Initialize validation state
	bIsValidating = false;
	CurrentValidationAngle = 0.0f;
	ValidationAttempts = 0;
	FirstHitAngle = -1.0f;
	FirstHitResult = FHitResult();

	// Initialize audio properties
	bEnableAudioFeedback = true;
	ValidationSound = nullptr;
	MappingSound = nullptr;
	TargetFoundSound = nullptr;
	ValidationFailedSound = nullptr;
	ValidationSoundInterval = 0.2f;  // Fast beeping during discovery (5 beeps per second)
	MappingSoundInterval = 1.0f;     // Slower beeping during mapping (1 beep per second)
	AudioVolumeMultiplier = 1.0f;
	ScannerAudioComponent = nullptr;
	AudioTimeSinceLastPlay = 0.0f;
	LastAudioState = EScannerState::Idle;
	
	// Attempt to load default scanner sounds from Content/Audio/Scanner/ if they exist
	// You can create/import sounds at these paths in the editor and they'll be auto-assigned
	static ConstructorHelpers::FObjectFinder<USoundBase> ValidationSoundAsset(TEXT("/Game/Audio/Scanner/ScannerBeep_Fast"));
	if (ValidationSoundAsset.Succeeded())
	{
		ValidationSound = ValidationSoundAsset.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<USoundBase> MappingSoundAsset(TEXT("/Game/Audio/Scanner/ScannerBeep_Slow"));
	if (MappingSoundAsset.Succeeded())
	{
		MappingSound = MappingSoundAsset.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<USoundBase> TargetFoundSoundAsset(TEXT("/Game/Audio/Scanner/ScannerBeep_Success"));
	if (TargetFoundSoundAsset.Succeeded())
	{
		TargetFoundSound = TargetFoundSoundAsset.Object;
	}
	
	static ConstructorHelpers::FObjectFinder<USoundBase> ValidationFailedSoundAsset(TEXT("/Game/Audio/Scanner/ScannerBeep_Error"));
	if (ValidationFailedSoundAsset.Succeeded())
	{
		ValidationFailedSound = ValidationFailedSoundAsset.Object;
	}

	
	// Initialize Text-to-Speech properties
	bEnableTextToSpeech = true;
	TTSVolume = 0.8f;  // 80% volume for clear announcements
	TTSRate = 1.0f;    // Normal speech rate

	LogMessage(TEXT("Constructor: NKScanner Camera Actor created"), true);
}

void ANKScannerCameraActor::BeginPlay()
{
	Super::BeginPlay();
	
	// ===== ENSURE CAMERA IS VISIBLE DURING GAMEPLAY =====
	SetActorHiddenInGame(false);
	LogMessage(TEXT("BeginPlay: Camera visibility set to VISIBLE for gameplay"), true);
	
	LogMessage(FString::Printf(TEXT("BeginPlay: NKScanner Camera Actor '%s' initialized at location: %s"), 
		*GetName(), *GetActorLocation().ToString()), true);

	InitializeLogFile();
	
	// ===== INITIALIZE AUDIO COMPONENT =====
	if (bEnableAudioFeedback)
	{
		ScannerAudioComponent = NewObject<UAudioComponent>(this, UAudioComponent::StaticClass());
		if (ScannerAudioComponent)
		{
			ScannerAudioComponent->bAutoActivate = false;
			ScannerAudioComponent->bStopWhenOwnerDestroyed = true;
			ScannerAudioComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
			ScannerAudioComponent->RegisterComponent();
			LogMessage(TEXT("BeginPlay: Audio component initialized successfully"), true);
		}
		else
		{
			LogMessage(TEXT("BeginPlay: WARNING - Failed to create audio component!"), true);
		}
	}
	else
	{
		LogMessage(TEXT("BeginPlay: Audio feedback is disabled"), true);
	}

	LogMessage(FString::Printf(TEXT("BeginPlay: Scanner Configuration - Range: %.2f, Speed: %.2f, Enabled: %s"),
		ScanRange, ScanSpeed, bScannerEnabled ? TEXT("true") : TEXT("false")), true);

	LogMessage(FString::Printf(TEXT("BeginPlay: Laser Configuration - MaxRange: %.2f, ShowBeam: %s, Continuous: %s"),
		LaserMaxRange, bShowLaserBeam ? TEXT("true") : TEXT("false"), bContinuousLaserShoot ? TEXT("true") : TEXT("false")), true);
	
	// ===== AUTO-START TERRAIN MAPPING ON PLAY =====
	if (CinematicTargetLandscape && bScannerEnabled && bAutoStartDiscovery)
	{
		LogMessage(TEXT("BeginPlay: Auto-starting terrain mapping..."), true);
		LogMessage(FString::Printf(TEXT("BeginPlay: Target already set to '%s' - initiating 4-step workflow"), 
			*CinematicTargetLandscape->GetName()), true);
		
		// Start the 4-step terrain mapping workflow automatically
		StartCinematicScan(
			CinematicTargetLandscape,
			CinematicHeightPercent,
			CinematicDistanceMeters,
			CinematicJSONOutputPath
		);
	}
	else
	{
		if (!CinematicTargetLandscape)
		{
			LogMessage(TEXT("BeginPlay: No target landscape set - terrain mapping will NOT auto-start"), true);
			LogMessage(TEXT("BeginPlay: Set 'Cinematic Target Landscape' in Details panel to enable auto-mapping"), true);
		}
		
		if (!bScannerEnabled)
		{
			LogMessage(TEXT("BeginPlay: Scanner is disabled - terrain mapping will NOT auto-start"), true);
		}
		
		if (!bAutoStartDiscovery)
		{
			LogMessage(TEXT("BeginPlay: Auto-Discovery is disabled - click 'Start Discovery' button to begin"), true);
		}
	}
}

void ANKScannerCameraActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bLogEveryFrame && bEnableVerboseLogging)
	{
		LogMessage(FString::Printf(TEXT("Tick: DeltaTime: %.4f, ScanProgress: %.2f, IsScanning: %s, IsCinematicScan: %s, IsPlayback: %s"),
			DeltaTime, ScanProgress, bIsScanning ? TEXT("Y") : TEXT("N"), 
			bIsCinematicScanActive ? TEXT("Y") : TEXT("N"), bIsPlayingBack ? TEXT("Y") : TEXT("N")));
	}

	// Update scan progress if scanning is active
	if (bIsScanning && bScannerEnabled)
	{
		ScanProgress += DeltaTime * ScanSpeed;
		
		if (ScanProgress >= 1.0f)
		{
			ScanProgress = 1.0f;
			bIsScanning = false;
			LogMessage(TEXT("Tick: Scanning completed - progress reached 100%"), true);
		}
	}
	
	// Update audio feedback based on current state
	if (bEnableAudioFeedback && ScannerAudioComponent)
	{
		UpdateAudioFeedback(DeltaTime);
	}

	// Update target finder state (Step 3 - incremental target discovery)
	if (bIsValidating)
	{
		UpdateTargetFinder(DeltaTime);
	}

	// Update cinematic scan (autonomous navigation)
	if (bIsCinematicScanActive)
	{
		// Accumulate time since last update
		CinematicScanUpdateAccumulator += DeltaTime;
		
		// Only update when accumulator reaches the update interval
		if (CinematicScanUpdateAccumulator >= MappingUpdateInterval)
		{
			// Update using accumulated time for smooth motion
			UpdateCinematicScan(CinematicScanUpdateAccumulator);
			CinematicScanUpdateAccumulator = 0.0f;  // Reset accumulator
		}
	}

	// Update playback
	if (bIsPlayingBack)
	{
		UpdatePlayback(DeltaTime);
	}

	// Continuous laser shooting if enabled
	if (bContinuousLaserShoot && bScannerEnabled)
	{
		ShootLaser();
	}
}

// ========================================================================
// LOGGING FUNCTIONS
// ========================================================================

void ANKScannerCameraActor::LogMessage(const FString& Message, bool bForceLog)
{
	if (!bEnableVerboseLogging && !bForceLog)
	{
		return;
	}

	FString FormattedMessage = FString::Printf(TEXT("[NKScanner][%s][Frame:%d] %s"), 
		*FDateTime::Now().ToString(), GFrameCounter, *Message);

	// Always log to UE_LOG
	UE_LOG(LogTemp, Warning, TEXT("%s"), *FormattedMessage);

	// Log to custom file if enabled
	if (bLogToFile && bLogFileInitialized)
	{
		FFileHelper::SaveStringToFile(FormattedMessage + TEXT("\n"), *ActualLogFilePath, 
			FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
	}
}

void ANKScannerCameraActor::InitializeLogFile()
{
	if (!bLogToFile)
	{
		LogMessage(TEXT("InitializeLogFile: File logging is disabled"), true);
		return;
	}

	// Determine log file path
	if (CustomLogFilePath.IsEmpty())
	{
		ActualLogFilePath = FPaths::ProjectSavedDir() / TEXT("Logs") / TEXT("NKScanner.log");
	}
	else
	{
		ActualLogFilePath = CustomLogFilePath;
	}

	// Ensure directory exists
	FString LogDir = FPaths::GetPath(ActualLogFilePath);
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*LogDir))
	{
		PlatformFile.CreateDirectoryTree(*LogDir);
	}

	// Write header
	FString Header = FString::Printf(TEXT("======================================\n"));
	Header += FString::Printf(TEXT("NKScanner Camera Actor Log\n"));
	Header += FString::Printf(TEXT("Session Started: %s\n"), *FDateTime::Now().ToString());
	Header += FString::Printf(TEXT("Actor Name: %s\n"), *GetName());
	Header += FString::Printf(TEXT("Log File: %s\n"), *ActualLogFilePath);
	Header += FString::Printf(TEXT("======================================\n\n"));

	FFileHelper::SaveStringToFile(Header, *ActualLogFilePath, FFileHelper::EEncodingOptions::AutoDetect);

	bLogFileInitialized = true;
	LogMessage(FString::Printf(TEXT("InitializeLogFile: Log file initialized at: %s"), *ActualLogFilePath), true);
}

void ANKScannerCameraActor::CloseLogFile()
{
	if (!bLogFileInitialized)
	{
		return;
	}

	FString Footer = FString::Printf(TEXT("\n======================================\n"));
	Footer += FString::Printf(TEXT("Session Ended: %s\n"), *FDateTime::Now().ToString());
	Footer += FString::Printf(TEXT("======================================\n"));

	FFileHelper::SaveStringToFile(Footer, *ActualLogFilePath, 
		FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);

	LogMessage(TEXT("CloseLogFile: Log file closed"), true);
	bLogFileInitialized = false;
}

// ========================================================================
// BASIC SCANNER FUNCTIONS
// ========================================================================

void ANKScannerCameraActor::StartScanning()
{
	LogMessage(FString::Printf(TEXT("StartScanning: Attempting to start scan. ScannerEnabled: %s"), 
		bScannerEnabled ? TEXT("true") : TEXT("false")), true);

	if (bScannerEnabled)
	{
		bIsScanning = true;
		ScanProgress = 0.0f;
		LogMessage(FString::Printf(TEXT("StartScanning: Scanner started successfully on camera: %s"), *GetName()), true);
	}
	else
	{
		LogMessage(TEXT("StartScanning: FAILED - Scanner is disabled"), true);
	}
}

void ANKScannerCameraActor::StopScanning()
{
	LogMessage(FString::Printf(TEXT("StopScanning: Stopping scanner. Current progress: %.2f%%"), ScanProgress * 100.0f), true);
	bIsScanning = false;
	LogMessage(TEXT("StopScanning: Scanner stopped successfully"), true);
}

float ANKScannerCameraActor::GetScanProgress() const
{
	return ScanProgress;
}

bool ANKScannerCameraActor::ShootLaser()
{
	if (!bScannerEnabled)
	{
		LogMessage(TEXT("ShootLaser: FAILED - Scanner is disabled"));
		return false;
	}

	LogMessage(TEXT("ShootLaser: Initiating laser trace"));

	FHitResult HitResult;
	bool bHit = PerformLaserTrace(HitResult);

	UpdateLaserHitProperties(HitResult, bHit);

	UCineCameraComponent* CineCamComponent = GetCineCameraComponent();
	if (CineCamComponent)
	{
		FVector Start = CineCamComponent->GetComponentLocation();
		FVector Direction = CineCamComponent->GetForwardVector();
		FVector End = Start + (Direction * LaserMaxRange);

		if (bHit)
		{
			End = HitResult.Location;
			LogMessage(FString::Printf(TEXT("ShootLaser: HIT - Actor: %s, Distance: %.2f cm, Location: %s"),
				HitResult.GetActor() ? *HitResult.GetActor()->GetName() : TEXT("None"),
				HitResult.Distance, *HitResult.Location.ToString()), true);
		}
		else
		{
			LogMessage(TEXT("ShootLaser: MISS - No hit detected"));
		}

		if (bShowLaserBeam)
		{
			DrawLaserBeam(Start, End, bHit);
		}
	}
	else
	{
		LogMessage(TEXT("ShootLaser: ERROR - CineCameraComponent not found!"), true);
	}

	return bHit;
}

// ========================================================================
// LASER TRACING FUNCTIONS
// ========================================================================

bool ANKScannerCameraActor::PerformLaserTrace(FHitResult& OutHitResult)
{
	LogMessage(TEXT("PerformLaserTrace: Starting trace"));

	UCineCameraComponent* CineCamComponent = GetCineCameraComponent();
	if (!CineCamComponent)
	{
		LogMessage(TEXT("PerformLaserTrace: ERROR - No CineCameraComponent found!"), true);
		return false;
	}

	FVector Start = CineCamComponent->GetComponentLocation();
	FVector Direction = CineCamComponent->GetForwardVector();
	FVector End = Start + (Direction * LaserMaxRange);

	LogMessage(FString::Printf(TEXT("PerformLaserTrace: Start: %s, Direction: %s, MaxRange: %.2f"),
		*Start.ToString(), *Direction.ToString(), LaserMaxRange));

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = true;

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		OutHitResult,
		Start,
		End,
		LaserTraceChannel,
		QueryParams
	);

	LogMessage(FString::Printf(TEXT("PerformLaserTrace: Trace completed. Hit: %s"), bHit ? TEXT("YES") : TEXT("NO")));

	return bHit;
}

void ANKScannerCameraActor::UpdateLaserHitProperties(const FHitResult& HitResult, bool bHit)
{
	LogMessage(TEXT("UpdateLaserHitProperties: Updating hit properties"));

	bLastShotHit = bHit;

	if (bHit)
	{
		LastHitLocation = HitResult.Location;
		LastHitNormal = HitResult.Normal;
		LastHitActor = HitResult.GetActor();
		LastHitComponent = HitResult.GetComponent();
		LastHitDistance = HitResult.Distance;

		if (HitResult.PhysMaterial.IsValid())
		{
			LastHitPhysicalMaterial = HitResult.PhysMaterial->GetFName();
			LogMessage(FString::Printf(TEXT("UpdateLaserHitProperties: Physical Material: %s"), 
				*LastHitPhysicalMaterial.ToString()));
		}
		else
		{
			LastHitPhysicalMaterial = NAME_None;
		}

		LogMessage(FString::Printf(TEXT("UpdateLaserHitProperties: Updated - Location: %s, Normal: %s, Distance: %.2f"),
			*LastHitLocation.ToString(), *LastHitNormal.ToString(), LastHitDistance), true);

		OnLaserHit.Broadcast(LastHitLocation, LastHitActor, LastHitDistance);
	}
	else
	{
		LastHitLocation = FVector::ZeroVector;
		LastHitNormal = FVector::ZeroVector;
		LastHitActor = nullptr;
		LastHitComponent = nullptr;
		LastHitDistance = 0.0f;
		LastHitPhysicalMaterial = NAME_None;

		LogMessage(TEXT("UpdateLaserHitProperties: No hit - properties reset to defaults"));
	}
}

void ANKScannerCameraActor::DrawLaserBeam(const FVector& Start, const FVector& End, bool bHit)
{
	if (!GetWorld())
	{
		return;
	}

	FColor BeamColor = bHit ? LaserColor : FColor::Green;
	DrawDebugLine(GetWorld(), Start, End, BeamColor, false, 0.1f, 0, LaserThickness);

	if (bHit)
	{
		DrawDebugSphere(GetWorld(), End, 10.0f, 8, FColor::Yellow, false, 0.1f);
	}
}

// ========================================================================
// CINEMATIC SCANNING FUNCTIONS
// ========================================================================

void ANKScannerCameraActor::StartCinematicScan(AActor* TargetLandscape, float HeightPercent, float DistanceMeters, FString OutputJSONPath)
{
	LogMessage(TEXT("=== TERRAIN MAPPING: 4-STEP WORKFLOW ==="), true);
	
	// ===== STEP 1: Validate Target (Mandatory) =====
	LogMessage(TEXT("STEP 1: Validating target..."), true);
	
	if (!TargetLandscape)
	{
		LogMessage(TEXT("STEP 1 FAILED: Target is NULL - terrain mapping aborted!"), true);
		return;
	}
	
	LogMessage(FString::Printf(TEXT("STEP 1 SUCCESS: Target validated - %s"), 
		*TargetLandscape->GetName()), true);
	
	CinematicTargetLandscape = TargetLandscape;
	CinematicHeightPercent = HeightPercent;
	CinematicDistanceMeters = DistanceMeters;
	CinematicJSONOutputPath = OutputJSONPath.IsEmpty() ? 
		FPaths::ProjectSavedDir() / TEXT("ScanData") / TEXT("CinematicScan.json") : OutputJSONPath;
	
	// ===== STEP 2: Move to Optimal Mapping Position =====
	LogMessage(TEXT("STEP 2: Finding optimal mapping position using spiral search..."), true);
	
	FBox TargetBounds = TargetLandscape->GetComponentsBoundingBox(true);
	FVector TargetCenter = TargetBounds.GetCenter();
	FVector TargetExtent = TargetBounds.GetExtent();
	FVector TargetSize = TargetBounds.GetSize();
	
	LogMessage(FString::Printf(TEXT("STEP 2: Bounding Box Analysis:")), true);
	LogMessage(FString::Printf(TEXT("  Center: %s"), *TargetCenter.ToString()), true);
	LogMessage(FString::Printf(TEXT("  Extent: %s (half-size)"), *TargetExtent.ToString()), true);
	LogMessage(FString::Printf(TEXT("  Size: %.2fm x %.2fm x %.2fm"), 
		TargetSize.X / 100.0f, TargetSize.Y / 100.0f, TargetSize.Z / 100.0f), true);
	LogMessage(FString::Printf(TEXT("  Min: %s"), *TargetBounds.Min.ToString()), true);
	LogMessage(FString::Printf(TEXT("  Max: %s"), *TargetBounds.Max.ToString()), true);
	
	// ===== IMPORTANT: Calculate the height at the specified percentage =====
	float TargetHeightAtPercent = TargetBounds.Min.Z + 
		((TargetBounds.Max.Z - TargetBounds.Min.Z) * (HeightPercent / 100.0f));
	
	LogMessage(FString::Printf(TEXT("STEP 2: Scan height at %.0f%% = %.2f cm (%.2f m)"), 
		HeightPercent, TargetHeightAtPercent, TargetHeightAtPercent / 100.0f), true);
	
	// ===== IMPROVED: Start 100m from FARTHEST BOUNDARY, not from center =====
	// Get the largest horizontal extent (X or Y dimension)
	float MaxHorizontalExtent = FMath::Max(TargetExtent.X, TargetExtent.Y);
	
	// Clearance distance from the farthest edge
	float ClearanceMeters = 100.0f;
	float ClearanceCm = ClearanceMeters * 100.0f;
	
	// Calculate starting distance: from center to farthest edge + clearance
	float SearchDistance = MaxHorizontalExtent + ClearanceCm;
	float DistanceIncrement = 10000.0f;  // Move outward by 100m each attempt if needed
	
	CinematicOrbitHeight = TargetHeightAtPercent;
	CinematicOrbitCenter = FVector(TargetCenter.X, TargetCenter.Y, TargetHeightAtPercent);
	CinematicLookAtTarget = FVector(TargetCenter.X, TargetCenter.Y, TargetHeightAtPercent);
	
	LogMessage(FString::Printf(TEXT("STEP 2: Position Calculation:")), true);
	LogMessage(FString::Printf(TEXT("  Max horizontal extent: %.2f m"), MaxHorizontalExtent / 100.0f), true);
	LogMessage(FString::Printf(TEXT("  Clearance from edge: %.2f m"), ClearanceMeters), true);
	LogMessage(FString::Printf(TEXT("  Starting distance from center: %.2f m"), SearchDistance / 100.0f), true);
	LogMessage(FString::Printf(TEXT("  Distance from farthest edge: %.2f m"), ClearanceCm / 100.0f), true);
	
	// Position camera at search distance in -Y direction (South)
	FVector MappingPosition = CinematicOrbitCenter;
	MappingPosition.Y -= SearchDistance;  // Move south
	
	SetActorLocation(MappingPosition);
	CinematicOrbitRadius = SearchDistance;
	
	LogMessage(FString::Printf(TEXT("STEP 2 SUCCESS: Camera positioned for scanning")), true);
	LogMessage(FString::Printf(TEXT("  Starting Position: X=%.2f, Y=%.2f, Z=%.2f"), 
		MappingPosition.X, MappingPosition.Y, MappingPosition.Z), true);
	LogMessage(FString::Printf(TEXT("  Starting Position (meters): X=%.2fm, Y=%.2fm, Z=%.2fm"), 
		MappingPosition.X / 100.0f, MappingPosition.Y / 100.0f, MappingPosition.Z / 100.0f), true);
	LogMessage(FString::Printf(TEXT("  Orbit radius: %.2f m"), SearchDistance / 100.0f), true);
	LogMessage(FString::Printf(TEXT("  Orbit height: %.2f m"), TargetHeightAtPercent / 100.0f), true);
	
	// Ensure laser range is sufficient
	float DistanceToTarget = FVector::Dist(MappingPosition, CinematicLookAtTarget);
	if (DistanceToTarget > LaserMaxRange)
	{
		LaserMaxRange = DistanceToTarget * 2.0f;
		LogMessage(FString::Printf(TEXT("STEP 2: Auto-adjusted laser range to %.2f cm (%.2f m) to reach target"), 
			LaserMaxRange, LaserMaxRange / 100.0f), true);
	}
	
	// ===== STEP 3: Enter Target Finder State (Spiral Search) =====
	LogMessage(TEXT("STEP 3: Entering spiral search target finder..."), true);
	LogMessage(FString::Printf(TEXT("STEP 3: Will search at %.2f m distance using %.2f° steps"),
		SearchDistance / 100.0f, ValidationAngularStepDegrees), true);
	LogMessage(TEXT("STEP 3: If no hit found, will move outward by 100m and retry"), true);
	LogMessage(TEXT("STEP 3: Camera orbits HORIZONTALLY, laser shoots PARALLEL to ground"), true);
	
	// Enter target finder state - Tick() will handle incremental search
	StartTargetFinderState();
	
	// Draw the orbit path for visualization
	if (bShowOrbitPath && GetWorld())
	{
		DrawOrbitPath();
	}
	
	// The UpdateTargetFinder() in Tick() will handle the spiral search!
	// Once target is found, it will automatically transition to Step 4
}

// ========================================================================
// JSON FUNCTIONS
// ========================================================================
