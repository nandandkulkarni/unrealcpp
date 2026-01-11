// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/NKScannerPlayerController.h"
#include "Scanner/NKMappingCamera.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"

ANKScannerPlayerController::ANKScannerPlayerController()
	: CurrentCameraIndex(0), YawRotationSpeed(10.0f)
{
}

void ANKScannerPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Set default input mode to Game Mode (camera control)
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	
	UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Starting in GAME Mode (Press Tab to toggle UI)"));
	
	// Find all cameras in the level
	FindAllCameras();
	
	// Automatically switch to first camera (usually the MappingCamera)
	if (AvailableCameras.Num() > 0)
	{
		SwitchToCamera(0);
	}
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
		UE_LOG(LogTemp, Log, TEXT("Found Mapping Camera: %s"), *MappingCamera->GetName());
	}
	
	// Find all other camera actors
	TArray<AActor*> FoundCameras;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACameraActor::StaticClass(), FoundCameras);
	
	for (AActor* Camera : FoundCameras)
	{
		AvailableCameras.Add(Camera);
		UE_LOG(LogTemp, Log, TEXT("Found Camera: %s"), *Camera->GetName());
	}
	
	// Add player pawn as a camera option
	if (GetPawn())
	{
		AvailableCameras.Add(GetPawn());
		UE_LOG(LogTemp, Log, TEXT("Added Player Pawn as camera: %s"), *GetPawn()->GetName());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Total cameras found: %d"), AvailableCameras.Num());
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
		return FString::Printf(TEXT("📷 %s"), *Label);
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
	
	UE_LOG(LogTemp, Warning, TEXT("Switching to camera: %s"), *NewCamera->GetName());
	
	// Smooth blend to new camera
	SetViewTargetWithBlend(NewCamera, CameraBlendTime);
}

void ANKScannerPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();  // This keeps WASD and all default controls!
	
	// Bind Tab key to toggle UI mode
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ANKScannerPlayerController::ToggleUIMode);
		
		// Add arrow keys to move the VIEW TARGET camera (works in both modes)
		InputComponent->BindKey(EKeys::Up, IE_Pressed, this, &ANKScannerPlayerController::MoveCameraForward);
		InputComponent->BindKey(EKeys::Down, IE_Pressed, this, &ANKScannerPlayerController::MoveCameraBackward);
		
		// Note: Left/Right without Shift moves camera sideways
		// We'll check for Shift in the movement functions themselves
		InputComponent->BindKey(EKeys::Left, IE_Pressed, this, &ANKScannerPlayerController::MoveCameraLeft);
		InputComponent->BindKey(EKeys::Right, IE_Pressed, this, &ANKScannerPlayerController::MoveCameraRight);
		
		UE_LOG(LogTemp, Warning, TEXT("ScannerPlayerController: Input bindings set up (Tab, Arrows, Shift+Arrows for yaw)"));
	}
}

// Add these new functions to move the ViewTarget camera
void ANKScannerPlayerController::MoveCameraForward()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector NewLocation = ViewTarget->GetActorLocation() + ViewTarget->GetActorForwardVector() * 100.0f;
		ViewTarget->SetActorLocation(NewLocation);
	}
}

void ANKScannerPlayerController::MoveCameraBackward()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		FVector NewLocation = ViewTarget->GetActorLocation() - ViewTarget->GetActorForwardVector() * 100.0f;
		ViewTarget->SetActorLocation(NewLocation);
	}
}

void ANKScannerPlayerController::MoveCameraLeft()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		// Check if Shift is held - if so, rotate yaw instead of moving
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			RotateCameraYawLeft();
		}
		else
		{
			FVector NewLocation = ViewTarget->GetActorLocation() - ViewTarget->GetActorRightVector() * 100.0f;
			ViewTarget->SetActorLocation(NewLocation);
		}
	}
}

void ANKScannerPlayerController::MoveCameraRight()
{
	if (AActor* ViewTarget = GetViewTarget())
	{
		// Check if Shift is held - if so, rotate yaw instead of moving
		if (IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift))
		{
			RotateCameraYawRight();
		}
		else
		{
			FVector NewLocation = ViewTarget->GetActorLocation() + ViewTarget->GetActorRightVector() * 100.0f;
			ViewTarget->SetActorLocation(NewLocation);
		}
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
		
		UE_LOG(LogTemp, Log, TEXT("Camera yaw rotated left: %.1f°"), NewRotation.Yaw);
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
		
		UE_LOG(LogTemp, Log, TEXT("Camera yaw rotated right: %.1f°"), NewRotation.Yaw);
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
