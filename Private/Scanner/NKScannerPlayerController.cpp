// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKScannerPlayerController.h"
#include "Scanner/NKMappingCamera.h"
#include "Scanner/NKOverheadCamera.h"
#include "Scanner/NKObserverCamera.h"
#include "Camera/CameraActor.h"
#include "CineCameraActor.h"
#include "Kismet/GameplayStatics.h"

ANKScannerPlayerController::ANKScannerPlayerController()
	: CurrentCameraIndex(0), YawRotationSpeed(10.0f), bAutoSpawnObserverCamera(true), 
	  ObserverCameraTarget(nullptr), ObserverCameraHeight(100.0f), SpawnedObserverCamera(nullptr)
{
}

void ANKScannerPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Set default input mode to Game AND UI Mode (mouse visible, can interact with both)
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	bShowMouseCursor = true;  // Show cursor by default
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	
	UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Starting in UI MODE (mouse visible - Press Tab to toggle)"));
	
	// Auto-spawn Observer Camera if enabled
	if (bAutoSpawnObserverCamera)
	{
		SpawnObserverCamera();
	}
	
	// Find all cameras in the level
	FindAllCameras();
	
	// Automatically switch to first camera (usually the MappingCamera)
	if (AvailableCameras.Num() > 0)
	{
		SwitchToCamera(0);
	}
}

void ANKScannerPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Destroy spawned Observer Camera when play ends
	if (SpawnedObserverCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Destroying auto-spawned Observer Camera"));
		SpawnedObserverCamera->Destroy();
		SpawnedObserverCamera = nullptr;
	}
	
	Super::EndPlay(EndPlayReason);
}

void ANKScannerPlayerController::FindAllCameras()
{
	AvailableCameras.Empty();
	
	// Find mapping camera
	ANKMappingCamera* MappingCamera = Cast<ANKMappingCamera>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANKMappingCamera::StaticClass()));
	
	if (MappingCamera)
	{
		AvailableCameras.Add(MappingCamera);
		UE_LOG(LogTemp, Log, TEXT("[1] Found Mapping Camera: %s"), *MappingCamera->GetName());
		
		// Add its overhead camera if it exists
		if (MappingCamera->GetOverheadCamera())
		{
			AvailableCameras.Add(MappingCamera->GetOverheadCamera());
			UE_LOG(LogTemp, Log, TEXT("[3] Found Overhead Camera: %s"), 
				*MappingCamera->GetOverheadCamera()->GetName());
		}
	}
	
	// Find observer camera (including the auto-spawned one)
	UE_LOG(LogTemp, Warning, TEXT("Searching for Observer Camera..."));
	ANKObserverCamera* ObserverCamera = Cast<ANKObserverCamera>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANKObserverCamera::StaticClass()));
	
	if (ObserverCamera)
	{
		AvailableCameras.Add(ObserverCamera);
		UE_LOG(LogTemp, Log, TEXT("[2] Found Observer Camera: %s"), *ObserverCamera->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No Observer Camera found in level!"));
	}
	
	// Find all CameraActor instances
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);
	
	UE_LOG(LogTemp, Warning, TEXT("Found %d CameraActor instances"), FoundCameras.Num());
	
	for (AActor* Camera : FoundCameras)
	{
		// Skip if already added
		if (!AvailableCameras.Contains(Camera))
		{
			AvailableCameras.Add(Camera);
			UE_LOG(LogTemp, Log, TEXT("[4+] Found Camera: %s (Type: %s)"), 
				*Camera->GetName(), *Camera->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Skipping already added camera: %s"), *Camera->GetName());
		}
	}
	
	// Find all CineCameraActor instances (separate from CameraActor)
	TArray<AActor*> FoundCineCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACineCameraActor::StaticClass(), FoundCineCameras);
	
	UE_LOG(LogTemp, Warning, TEXT("Found %d CineCameraActor instances"), FoundCineCameras.Num());
	
	for (AActor* Camera : FoundCineCameras)
	{
		// Skip if already added (Observer camera inherits from CineCameraActor)
		if (!AvailableCameras.Contains(Camera))
		{
			AvailableCameras.Add(Camera);
			UE_LOG(LogTemp, Log, TEXT("[4+] Found CineCamera: %s (Type: %s)"), 
				*Camera->GetName(), *Camera->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Log, TEXT("Skipping already added cine camera: %s"), *Camera->GetName());
		}
	}
	
	// Add player pawn as a camera option
	if (GetPawn())
	{
		AvailableCameras.Add(GetPawn());
		UE_LOG(LogTemp, Log, TEXT("Added Player Pawn as camera: %s"), *GetPawn()->GetName());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Total cameras found: %d"), AvailableCameras.Num());
	UE_LOG(LogTemp, Warning, TEXT("Press 1=Mapping, 2=Observer, 3=Overhead, C=Cycle cameras"));
}

void ANKScannerPlayerController::SwitchToNextCamera()
{
	if (AvailableCameras.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No cameras available"));
		return;
	}
	
	CurrentCameraIndex = (CurrentCameraIndex + 1) % AvailableCameras.Num();
	SwitchToCamera(CurrentCameraIndex);
}

void ANKScannerPlayerController::SwitchToPreviousCamera()
{
	if (AvailableCameras.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No cameras available"));
		return;
	}
	
	CurrentCameraIndex--;
	if (CurrentCameraIndex < 0)
	{
		CurrentCameraIndex = AvailableCameras.Num() - 1;
	}
	
	SwitchToCamera(CurrentCameraIndex);
}

void ANKScannerPlayerController::SwitchToCamera(int32 CameraIndex)
{
	if (!AvailableCameras.IsValidIndex(CameraIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid camera index: %d"), CameraIndex);
		return;
	}
	
	CurrentCameraIndex = CameraIndex;
	AActor* NewCamera = AvailableCameras[CameraIndex];
	
	PerformCameraSwitch(NewCamera);
}

void ANKScannerPlayerController::SwitchToMappingCamera()
{
	// Find mapping camera in list
	for (int32 i = 0; i < AvailableCameras.Num(); i++)
	{
		if (Cast<ANKMappingCamera>(AvailableCameras[i]))
		{
			SwitchToCamera(i);
			return;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Mapping camera not found in available cameras"));
}

void ANKScannerPlayerController::SwitchToObserverCamera()
{
	// Find observer camera in list
	for (int32 i = 0; i < AvailableCameras.Num(); i++)
	{
		if (Cast<ANKObserverCamera>(AvailableCameras[i]))
		{
			SwitchToCamera(i);
			return;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Observer camera not found in available cameras"));
}

void ANKScannerPlayerController::SwitchToOverheadCamera()
{
	// Find overhead camera in list
	for (int32 i = 0; i < AvailableCameras.Num(); i++)
	{
		if (Cast<ANKOverheadCamera>(AvailableCameras[i]))
		{
			SwitchToCamera(i);
			return;
		}
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Overhead camera not found in available cameras"));
}

FString ANKScannerPlayerController::GetCameraName(int32 Index) const
{
	if (!AvailableCameras.IsValidIndex(Index))
	{
		return TEXT("Invalid");
	}
	
	AActor* Camera = AvailableCameras[Index];
	if (!Camera)
	{
		return TEXT("NULL");
	}
	
	// Use actor label if available, otherwise name
	FString Label = Camera->GetActorLabel();
	if (Label.IsEmpty())
	{
		Label = Camera->GetName();
	}
	
	// Add type prefix
	if (Cast<ANKMappingCamera>(Camera))
	{
		return FString::Printf(TEXT("📷 %s (Mapping)"), *Label);
	}
	else if (Cast<ANKObserverCamera>(Camera))
	{
		return FString::Printf(TEXT("🔭 %s (Observer)"), *Label);
	}
	else if (Camera->IsA(ANKOverheadCamera::StaticClass()))
	{
		return FString::Printf(TEXT("📐 %s (Overhead)"), *Label);
	}
	else if (Cast<ACameraActor>(Camera))
	{
		return FString::Printf(TEXT("🎥 %s"), *Label);
	}
	else
	{
		return FString::Printf(TEXT("👤 %s"), *Label);  // Player pawn
	}
}

FString ANKScannerPlayerController::GetCurrentCameraName() const
{
	return GetCameraName(CurrentCameraIndex);
}

void ANKScannerPlayerController::PerformCameraSwitch(AActor* NewCamera)
{
	if (!NewCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("Cannot switch to NULL camera"));
		return;
	}
	
	AActor* OldCamera = GetViewTarget();
	
	UE_LOG(LogTemp, Warning, TEXT("╔═══════════════════════════════════════════════════════╗"));
	UE_LOG(LogTemp, Warning, TEXT("║ CAMERA SWITCH                                         ║"));
	UE_LOG(LogTemp, Warning, TEXT("╠═══════════════════════════════════════════════════════╣"));
	UE_LOG(LogTemp, Warning, TEXT("║ FROM: %s"), OldCamera ? *OldCamera->GetName() : TEXT("NULL"));
	if (OldCamera)
	{
		UE_LOG(LogTemp, Warning, TEXT("║   Location: %s"), *OldCamera->GetActorLocation().ToString());
		UE_LOG(LogTemp, Warning, TEXT("║   Rotation: %s"), *OldCamera->GetActorRotation().ToString());
	}
	UE_LOG(LogTemp, Warning, TEXT("╠═══════════════════════════════════════════════════════╣"));
	UE_LOG(LogTemp, Warning, TEXT("║ TO:   %s"), *NewCamera->GetName());
	UE_LOG(LogTemp, Warning, TEXT("║   Location: %s"), *NewCamera->GetActorLocation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("║   Rotation: %s"), *NewCamera->GetActorRotation().ToString());
	UE_LOG(LogTemp, Warning, TEXT("║   Class: %s"), *NewCamera->GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("╚═══════════════════════════════════════════════════════╝"));
	
	// Smooth blend to new camera
	SetViewTargetWithBlend(NewCamera, CameraBlendTime);
	
	// Log what the view target actually is after the blend is initiated
	UE_LOG(LogTemp, Warning, TEXT("╔═══════════════════════════════════════════════════════╗"));
	UE_LOG(LogTemp, Warning, TEXT("║ VIEW TARGET VERIFICATION (after blend started)        ║"));
	UE_LOG(LogTemp, Warning, TEXT("╠═══════════════════════════════════════════════════════╣"));
	AActor* ActualViewTarget = GetViewTarget();
	UE_LOG(LogTemp, Warning, TEXT("║ GetViewTarget() returns: %s"), ActualViewTarget ? *ActualViewTarget->GetName() : TEXT("NULL"));
	UE_LOG(LogTemp, Warning, TEXT("║ Requested target was:    %s"), *NewCamera->GetName());
	UE_LOG(LogTemp, Warning, TEXT("║ Match? %s"), ActualViewTarget == NewCamera ? TEXT("YES ✅") : TEXT("NO ❌ - BLEND IN PROGRESS"));
	UE_LOG(LogTemp, Warning, TEXT("║ Blend Time: %.2f seconds"), CameraBlendTime);
	UE_LOG(LogTemp, Warning, TEXT("╚═══════════════════════════════════════════════════════╝"));
}

void ANKScannerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();  // This keeps WASD and all default controls!
	
	// Bind Tab key to toggle UI mode
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ANKScannerPlayerController::ToggleUIMode);
		
		// Number keys for quick camera switching
		InputComponent->BindKey(EKeys::One, IE_Pressed, this, &ANKScannerPlayerController::SwitchToMappingCamera);
		InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &ANKScannerPlayerController::SwitchToObserverCamera);
		InputComponent->BindKey(EKeys::Three, IE_Pressed, this, &ANKScannerPlayerController::SwitchToOverheadCamera);
		
		// C key to cycle through cameras
		InputComponent->BindKey(EKeys::C, IE_Pressed, this, &ANKScannerPlayerController::SwitchToNextCamera);
		
		// R key to refresh camera list (useful if Observer camera added after game start)
		InputComponent->BindKey(EKeys::R, IE_Pressed, this, &ANKScannerPlayerController::FindAllCameras);
		
		// Add arrow keys to move the VIEW TARGET camera (works in both modes)
		// Use IE_Repeat so they work continuously when held down
		InputComponent->BindKey(EKeys::Up, IE_Repeat, this, &ANKScannerPlayerController::MoveCameraForward);
		InputComponent->BindKey(EKeys::Down, IE_Repeat, this, &ANKScannerPlayerController::MoveCameraBackward);
		InputComponent->BindKey(EKeys::Left, IE_Repeat, this, &ANKScannerPlayerController::MoveCameraLeft);
		InputComponent->BindKey(EKeys::Right, IE_Repeat, this, &ANKScannerPlayerController::MoveCameraRight);
		
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Hotkeys - 1:Mapping 2:Observer 3:Overhead C:Cycle Tab:UI R:Refresh"));
		UE_LOG(LogTemp, Warning, TEXT("Arrow Keys: Move camera | Shift+Arrows: Up/Down/Rotate"));
	}
}

// Add these new functions to move the ViewTarget camera
void ANKScannerPlayerController::MoveCameraForward()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector LocationBefore = ViewTarget->GetActorLocation();
		
		// Check if Shift is held - if so, move up instead
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			MoveCameraUp();
		}
		else
		{
			FVector NewLocation = ViewTarget->GetActorLocation() + ViewTarget->GetActorForwardVector() * 100.0f;
			ViewTarget->SetActorLocation(NewLocation);
			
			UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA MOVE FORWARD: %s"), *ViewTarget->GetName());
			UE_LOG(LogTemp, Log, TEXT("   Before: %s"), *LocationBefore.ToString());
			UE_LOG(LogTemp, Log, TEXT("   After:  %s"), *NewLocation.ToString());
		}
	}
}

void ANKScannerPlayerController::MoveCameraBackward()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector LocationBefore = ViewTarget->GetActorLocation();
		
		// Check if Shift is held - if so, move down instead
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			MoveCameraDown();
		}
		else
		{
			FVector NewLocation = ViewTarget->GetActorLocation() - ViewTarget->GetActorForwardVector() * 100.0f;
			ViewTarget->SetActorLocation(NewLocation);
			
			UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA MOVE BACKWARD: %s"), *ViewTarget->GetName());
			UE_LOG(LogTemp, Log, TEXT("   Before: %s"), *LocationBefore.ToString());
			UE_LOG(LogTemp, Log, TEXT("   After:  %s"), *NewLocation.ToString());
		}
	}
}

void ANKScannerPlayerController::MoveCameraLeft()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector LocationBefore = ViewTarget->GetActorLocation();
		
		// Check if Shift is held - if so, rotate yaw instead of moving
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			RotateCameraYawLeft();
		}
		else
		{
			FVector NewLocation = ViewTarget->GetActorLocation() - ViewTarget->GetActorRightVector() * 100.0f;
			ViewTarget->SetActorLocation(NewLocation);
			
			UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA MOVE LEFT: %s"), *ViewTarget->GetName());
			UE_LOG(LogTemp, Log, TEXT("   Before: %s"), *LocationBefore.ToString());
			UE_LOG(LogTemp, Log, TEXT("   After:  %s"), *NewLocation.ToString());
		}
	}
}

void ANKScannerPlayerController::MoveCameraRight()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector LocationBefore = ViewTarget->GetActorLocation();
		
		// Check if Shift is held - if so, rotate yaw instead of moving
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			RotateCameraYawRight();
		}
		else
		{
			FVector NewLocation = ViewTarget->GetActorLocation() + ViewTarget->GetActorRightVector() * 100.0f;
			ViewTarget->SetActorLocation(NewLocation);
			
			UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA MOVE RIGHT: %s"), *ViewTarget->GetName());
			UE_LOG(LogTemp, Log, TEXT("   Before: %s"), *LocationBefore.ToString());
			UE_LOG(LogTemp, Log, TEXT("   After:  %s"), *NewLocation.ToString());
		}
	}
}

void ANKScannerPlayerController::MoveCameraUp()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector NewLocation = ViewTarget->GetActorLocation();
		float ZBefore = NewLocation.Z;
		NewLocation.Z += 100.0f;  // Move up 1 meter
		ViewTarget->SetActorLocation(NewLocation);
		
		UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA MOVE UP: %s"), *ViewTarget->GetName());
		UE_LOG(LogTemp, Log, TEXT("   Z Before: %.2f m → After: %.2f m (Δ +1.00 m)"), ZBefore / 100.0f, NewLocation.Z / 100.0f);
	}
}

void ANKScannerPlayerController::MoveCameraDown()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector NewLocation = ViewTarget->GetActorLocation();
		float ZBefore = NewLocation.Z;
		NewLocation.Z -= 100.0f;  // Move down 1 meter
		ViewTarget->SetActorLocation(NewLocation);
		
		UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA MOVE DOWN: %s"), *ViewTarget->GetName());
		UE_LOG(LogTemp, Log, TEXT("   Z Before: %.2f m → After: %.2f m (Δ -1.00 m)"), ZBefore / 100.0f, NewLocation.Z / 100.0f);
	}
}

void ANKScannerPlayerController::RotateCameraYawLeft()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FRotator CurrentRotation = ViewTarget->GetActorRotation();
		FRotator NewRotation = CurrentRotation;
		NewRotation.Yaw -= YawRotationSpeed;  // Rotate left (counter-clockwise)
		ViewTarget->SetActorRotation(NewRotation);
		
		UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA ROTATE YAW LEFT: %s"), *ViewTarget->GetName());
		UE_LOG(LogTemp, Log, TEXT("   Yaw Before: %.2f° → After: %.2f° (Δ -%.1f°)"), 
			CurrentRotation.Yaw, NewRotation.Yaw, YawRotationSpeed);
	}
}

void ANKScannerPlayerController::RotateCameraYawRight()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FRotator CurrentRotation = ViewTarget->GetActorRotation();
		FRotator NewRotation = CurrentRotation;
		NewRotation.Yaw += YawRotationSpeed;  // Rotate right (clockwise)
		ViewTarget->SetActorRotation(NewRotation);
		
		UE_LOG(LogTemp, Log, TEXT("🎥 CAMERA ROTATE YAW RIGHT: %s"), *ViewTarget->GetName());
		UE_LOG(LogTemp, Log, TEXT("   Yaw Before: %.2f° → After: %.2f° (Δ +%.1f°)"), 
			CurrentRotation.Yaw, NewRotation.Yaw, YawRotationSpeed);
	}
}

void ANKScannerPlayerController::ToggleUIMode()
{
	// Toggle mouse cursor visibility
	bShowMouseCursor = !bShowMouseCursor;
	
	if (bShowMouseCursor)
	{
		// UI Mode - can click buttons, interact with HUD, AND still use Tab key
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
		
		FInputModeGameAndUI InputMode;  // ← Changed to GameAndUI so Tab works!
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Switched to UI MODE (mouse visible, clicks enabled)"));
	}
	else
	{
		// Game Mode - can control camera, WASD movement
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
		
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Switched to GAME MODE (camera control)"));
	}
}

void ANKScannerPlayerController::SpawnObserverCamera()
{
	if (!GetWorld())
	{
		UE_LOG(LogTemp, Error, TEXT("ScannerPlayerController: Cannot spawn Observer Camera - no world!"));
		return;
	}
	
	// Don't spawn if one already exists in the level
	ANKObserverCamera* ExistingObserver = Cast<ANKObserverCamera>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANKObserverCamera::StaticClass()));
	
	if (ExistingObserver)
	{
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Observer Camera already exists in level, skipping auto-spawn"));
		return;
	}
	
	// Try to find a target if not set
	if (!ObserverCameraTarget)
	{
		// Look for a MappingCamera's target
		ANKMappingCamera* MappingCamera = Cast<ANKMappingCamera>(
			UGameplayStatics::GetActorOfClass(GetWorld(), ANKMappingCamera::StaticClass()));
		
		if (MappingCamera && MappingCamera->TargetActor)
		{
			ObserverCameraTarget = MappingCamera->TargetActor;
			UE_LOG(LogTemp, Log, TEXT("ScannerPlayerController: Using MappingCamera's target: %s"), 
				*ObserverCameraTarget->GetName());
		}
	}
	
	if (!ObserverCameraTarget)
	{
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: No target set for Observer Camera - will need to be set manually"));
		// Don't spawn without a target
		return;
	}
	
	// Spawn the Observer Camera
	FActorSpawnParameters SpawnParams;
	SpawnParams.Name = FName(TEXT("AutoSpawned_ObserverCamera"));
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	SpawnedObserverCamera = GetWorld()->SpawnActor<ANKObserverCamera>(
		ANKObserverCamera::StaticClass(),
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);
	
	if (SpawnedObserverCamera)
	{
		// Configure the Observer Camera BEFORE BeginPlay runs
		// Disable auto-positioning first to prevent the error
		SpawnedObserverCamera->bAutoPositionOnBeginPlay = false;
		SpawnedObserverCamera->TargetActor = ObserverCameraTarget;
		SpawnedObserverCamera->HeightAboveTargetMeters = ObserverCameraHeight;
		
		// Now manually trigger positioning after target is set
		SpawnedObserverCamera->PositionAboveTarget();
		
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Auto-spawned Observer Camera"));
		UE_LOG(LogTemp, Warning, TEXT("  Name: %s"), *SpawnedObserverCamera->GetName());
		UE_LOG(LogTemp, Warning, TEXT("  Target: %s"), ObserverCameraTarget ? *ObserverCameraTarget->GetName() : TEXT("NONE"));
		UE_LOG(LogTemp, Warning, TEXT("  Height: %.1fm"), ObserverCameraHeight);
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ScannerPlayerController: Failed to spawn Observer Camera!"));
	}
}

void ANKScannerPlayerController::ShootLaserFromCamera()
{
	AActor* ViewTarget = GetViewTarget();
	if (!ViewTarget || !GetWorld())
	{
		UE_LOG(LogTemp, Warning, TEXT("🔫 Cannot shoot laser - no active camera or world"));
		return;
	}
	
	// Get camera location and forward vector
	FVector CameraLocation = ViewTarget->GetActorLocation();
	FVector CameraForward = ViewTarget->GetActorForwardVector();
	
	// Shoot a very long ray (10km = 1,000,000 cm)
	float LaserRange = 1000000.0f;
	FVector LaserEnd = CameraLocation + (CameraForward * LaserRange);
	
	// Perform line trace
	FHitResult HitResult;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(ViewTarget);  // Don't hit the camera itself
	QueryParams.bTraceComplex = true;
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		CameraLocation,
		LaserEnd,
		ECC_Visibility,
		QueryParams
	);
	
	// Determine actual laser end point
	FVector ActualLaserEnd = bHit ? HitResult.Location : LaserEnd;
	
	// Draw persistent laser line (RED, very thick, never disappears)
	DrawDebugLine(
		GetWorld(),
		CameraLocation,
		ActualLaserEnd,
		FColor::Red,
		true,           // Persistent
		-1.0f,          // Infinite lifetime
		0,
		10.0f           // Very thick line
	);
	
	// Draw a 10cm × 10cm square at the hit point if we hit something
	if (bHit)
	{
		// Draw a flat square on the hit surface
		FVector HitLocation = HitResult.Location;
		FVector HitNormal = HitResult.ImpactNormal;
		
		// Create a coordinate system on the hit surface
		// Use the impact normal as the "up" direction for the square
		FVector Tangent1 = FVector::CrossProduct(HitNormal, FVector::UpVector);
		if (Tangent1.SizeSquared() < 0.001f)  // If normal is parallel to up vector
		{
			Tangent1 = FVector::CrossProduct(HitNormal, FVector::ForwardVector);
		}
		Tangent1.Normalize();
		FVector Tangent2 = FVector::CrossProduct(HitNormal, Tangent1);
		Tangent2.Normalize();
		
		// 10cm square = 5cm from center to edge
		float HalfSize = 5.0f;  // 5cm
		
		// Calculate the 4 corners of the square
		FVector Corner1 = HitLocation + (Tangent1 * HalfSize) + (Tangent2 * HalfSize);
		FVector Corner2 = HitLocation - (Tangent1 * HalfSize) + (Tangent2 * HalfSize);
		FVector Corner3 = HitLocation - (Tangent1 * HalfSize) - (Tangent2 * HalfSize);
		FVector Corner4 = HitLocation + (Tangent1 * HalfSize) - (Tangent2 * HalfSize);
		
		// Draw the 4 edges of the square
		DrawDebugLine(GetWorld(), Corner1, Corner2, FColor::Yellow, true, -1.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Corner2, Corner3, FColor::Yellow, true, -1.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Corner3, Corner4, FColor::Yellow, true, -1.0f, 0, 3.0f);
		DrawDebugLine(GetWorld(), Corner4, Corner1, FColor::Yellow, true, -1.0f, 0, 3.0f);
		
		// Draw diagonals for better visibility
		DrawDebugLine(GetWorld(), Corner1, Corner3, FColor::Orange, true, -1.0f, 0, 2.0f);
		DrawDebugLine(GetWorld(), Corner2, Corner4, FColor::Orange, true, -1.0f, 0, 2.0f);
	}
	
	// Comprehensive logging
	UE_LOG(LogTemp, Warning, TEXT("╔═══════════════════════════════════════════════════════╗"));
	UE_LOG(LogTemp, Warning, TEXT("║ 🔫 LASER SHOT FROM CAMERA                             ║"));
	UE_LOG(LogTemp, Warning, TEXT("╠═══════════════════════════════════════════════════════╣"));
	UE_LOG(LogTemp, Warning, TEXT("║ Camera: %s"), *ViewTarget->GetName());
	UE_LOG(LogTemp, Warning, TEXT("║ Camera Type: %s"), *ViewTarget->GetClass()->GetName());
	UE_LOG(LogTemp, Warning, TEXT("║ Start Position: %s"), *CameraLocation.ToString());
	UE_LOG(LogTemp, Warning, TEXT("║   (%.2f, %.2f, %.2f) m"), 
		CameraLocation.X/100.0f, CameraLocation.Y/100.0f, CameraLocation.Z/100.0f);
	UE_LOG(LogTemp, Warning, TEXT("║ Forward Vector: (%.3f, %.3f, %.3f)"), 
		CameraForward.X, CameraForward.Y, CameraForward.Z);
	UE_LOG(LogTemp, Warning, TEXT("╠═══════════════════════════════════════════════════════╣"));
	
	if (bHit)
	{
		AActor* HitActor = HitResult.GetActor();
		UE_LOG(LogTemp, Warning, TEXT("║ HIT: YES ✅"));
		UE_LOG(LogTemp, Warning, TEXT("║ Hit Actor: %s"), HitActor ? *HitActor->GetName() : TEXT("None"));
		UE_LOG(LogTemp, Warning, TEXT("║ Hit Component: %s"), 
			HitResult.Component.IsValid() ? *HitResult.Component->GetName() : TEXT("None"));
		UE_LOG(LogTemp, Warning, TEXT("║ Hit Location: %s"), *HitResult.Location.ToString());
		UE_LOG(LogTemp, Warning, TEXT("║   (%.2f, %.2f, %.2f) m"), 
			HitResult.Location.X/100.0f, HitResult.Location.Y/100.0f, HitResult.Location.Z/100.0f);
		UE_LOG(LogTemp, Warning, TEXT("║ Distance: %.2f cm (%.2f m)"), HitResult.Distance, HitResult.Distance/100.0f);
		UE_LOG(LogTemp, Warning, TEXT("║ Impact Normal: (%.3f, %.3f, %.3f)"), 
			HitResult.ImpactNormal.X, HitResult.ImpactNormal.Y, HitResult.ImpactNormal.Z);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("║ HIT: NO ❌ (shot into void)"));
		UE_LOG(LogTemp, Warning, TEXT("║ Laser traveled: %.2f m"), LaserRange/100.0f);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("╠═══════════════════════════════════════════════════════╣"));
	UE_LOG(LogTemp, Warning, TEXT("║ VISUALIZATION:"));
	UE_LOG(LogTemp, Warning, TEXT("║ • Red line drawn from camera to %s"), bHit ? TEXT("hit point") : TEXT("max range"));
	if (bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("║ • Yellow 10cm × 10cm square at hit point"));
		UE_LOG(LogTemp, Warning, TEXT("║ • Orange diagonal cross inside square"));
	}
	UE_LOG(LogTemp, Warning, TEXT("║ • Lines are PERSISTENT (never disappear)"));
	UE_LOG(LogTemp, Warning, TEXT("╚═══════════════════════════════════════════════════════╝"));
}
