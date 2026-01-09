// Copyright Epic Games, Inc. All Rights Reserved.


#include "TPCPPPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "TPCPP.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ATPCPPPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Set default input mode to Game Mode (camera control)
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;
	
	UE_LOG(LogTemp, Warning, TEXT("PlayerController: Starting in CAMERA Mode (Press Tab to toggle UI)"));

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogTPCPP, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}
}

void ATPCPPPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Bind Tab key to toggle UI mode
	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &ATPCPPPlayerController::ToggleUIMode);
		UE_LOG(LogTemp, Warning, TEXT("PlayerController: Tab key bound to ToggleUIMode"));
	}

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

bool ATPCPPPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void ATPCPPPlayerController::ToggleUIMode()
{
	// Toggle mouse cursor visibility
	bShowMouseCursor = !bShowMouseCursor;
	bEnableClickEvents = bShowMouseCursor;
	bEnableMouseOverEvents = bShowMouseCursor;
	
	// Set input mode based on cursor state
	if (bShowMouseCursor)
	{
		// UI Mode: Mouse visible, can click UI
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		
		UE_LOG(LogTemp, Warning, TEXT("UI Mode: Mouse cursor ENABLED (Press Tab to toggle)"));
	}
	else
	{
		// Game Mode: Mouse hidden, can control camera
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		
		UE_LOG(LogTemp, Warning, TEXT("Game Mode: Mouse cursor DISABLED - Camera control active (Press Tab to toggle)"));
	}
}

void ATPCPPPlayerController::ToggleUI()
{
	ToggleUIMode();
}
