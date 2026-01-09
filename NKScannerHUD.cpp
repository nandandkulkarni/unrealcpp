// Fill out your copyright notice in the Description page of Project Settings.

#include "NKScannerHUD.h"
#include "NKScannerCameraActor.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "EngineUtils.h"
#include "DrawDebugHelpers.h"

ANKScannerHUD::ANKScannerHUD()
{
	// Initialize HUD settings
	HUDXPosition = 20.0f;
	HUDYPosition = 20.0f;
	LineHeight = 20.0f;
	FontScale = 1.0f;
	ScannerCamera = nullptr;
	
	// Initialize rotation history
	MaxRotationHistory = 10;
	RotationUpdateTimer = 0.0f;
	RotationUpdateInterval = 0.5f;  // Update every 0.5 seconds
	RotationSerialNumber = 0;  // Start at 0
	LastRecordedRotation = FRotator::ZeroRotator;
	bUpdateOnMovement = true;  // TRUE = update on every camera movement, FALSE = update on timer
	
	// Performance settings
	FramesSinceLastHUDUpdate = 0;
	HUDUpdateFrequency = 1;  // 1 = every frame (no skipping), 2 = every other frame, 3 = every 3rd frame
	
	// Initialize mouse input state
	bMouseCursorEnabled = false;
	CurrentMousePosition = FVector2D::ZeroVector;
	
	// Initialize Start Discovery button
	StartDiscoveryButton.ButtonText = TEXT("Start Discovery");
	StartDiscoveryButton.Size = FVector2D(180.0f, 50.0f);
	StartDiscoveryButton.NormalColor = FLinearColor(0.1f, 0.3f, 0.5f, 0.8f);  // Blue-ish
	StartDiscoveryButton.HoverColor = FLinearColor(0.2f, 0.4f, 0.6f, 0.9f);
	StartDiscoveryButton.PressedColor = FLinearColor(0.3f, 0.5f, 0.7f, 1.0f);
	// Position will be set in DrawHUD based on canvas size
	
	// Initialize Start Mapping button
	StartMappingButton.ButtonText = TEXT("Start Mapping");
	StartMappingButton.Size = FVector2D(180.0f, 50.0f);
	StartMappingButton.NormalColor = FLinearColor(0.1f, 0.5f, 0.1f, 0.8f);  // Green
	StartMappingButton.HoverColor = FLinearColor(0.2f, 0.6f, 0.2f, 0.9f);
	StartMappingButton.PressedColor = FLinearColor(0.3f, 0.7f, 0.3f, 1.0f);
	// Position will be set in DrawHUD based on canvas size
	
	// Initialize automation checkboxes
	AutoDiscoveryCheckbox.LabelText = TEXT("Auto-Discovery");
	AutoDiscoveryCheckbox.BoxSize = 20.0f;
	AutoDiscoveryCheckbox.BoxColor = FLinearColor::White;
	AutoDiscoveryCheckbox.CheckColor = FLinearColor(0.0f, 1.0f, 0.0f);  // Green
	AutoDiscoveryCheckbox.TextColor = FLinearColor(0.9f, 0.9f, 0.9f);
	
	AutoMappingCheckbox.LabelText = TEXT("Auto-Mapping");
	AutoMappingCheckbox.BoxSize = 20.0f;
	AutoMappingCheckbox.BoxColor = FLinearColor::White;
	AutoMappingCheckbox.CheckColor = FLinearColor(0.0f, 1.0f, 0.0f);
	AutoMappingCheckbox.TextColor = FLinearColor(0.9f, 0.9f, 0.9f);
	
	AutoResetCheckbox.LabelText = TEXT("Auto-Reset");
	AutoResetCheckbox.BoxSize = 20.0f;
	AutoResetCheckbox.BoxColor = FLinearColor::White;
	AutoResetCheckbox.CheckColor = FLinearColor(0.0f, 1.0f, 0.0f);
	AutoResetCheckbox.TextColor = FLinearColor(0.9f, 0.9f, 0.9f);
}

void ANKScannerHUD::BeginPlay()
{
	Super::BeginPlay();
	FindScannerCamera();
	// Don't enable mouse cursor by default - let player toggle it
	// EnableMouseCursor();  // REMOVED - now toggled with Tab key
}

void ANKScannerHUD::DrawHUD()
{
	Super::DrawHUD();

	// Find scanner if not found yet
	if (!ScannerCamera)
	{
		FindScannerCamera();
		if (!ScannerCamera)
		{
			return;  // No scanner found, skip drawing
		}
	}

	// Check if HUD should be shown
	if (!ScannerCamera->bShowDebugHUD)
	{
		return;
	}
	
	// Update mouse hover states
	UpdateMouseHover();
	
	// ===== DRAW START DISCOVERY BUTTON (TOP-RIGHT) =====
	if (Canvas)
	{
		float ButtonPadding = 20.0f;
		float ButtonSpacing = 10.0f;  // Space between buttons
		EScannerState CurrentState = ScannerCamera->GetScannerState();
		
		// ===== DRAW START DISCOVERY BUTTON =====
		// Position button in top-right corner (with padding)
		StartDiscoveryButton.Position = FVector2D(
			Canvas->SizeX - StartDiscoveryButton.Size.X - ButtonPadding,
			ButtonPadding
		);
		
		// Update button text based on scanner state
		if (CurrentState == EScannerState::Idle)
		{
			StartDiscoveryButton.ButtonText = TEXT("Start Discovery");
			StartDiscoveryButton.NormalColor = FLinearColor(0.1f, 0.5f, 0.1f, 0.8f);  // Green
		}
		else if (CurrentState == EScannerState::Validating)
		{
			StartDiscoveryButton.ButtonText = TEXT("Cancel Discovery");
			StartDiscoveryButton.NormalColor = FLinearColor(0.7f, 0.3f, 0.0f, 0.8f);  // Orange
		}
		else if (CurrentState == EScannerState::Mapping)
		{
			StartDiscoveryButton.ButtonText = TEXT("Stop Mapping");
			StartDiscoveryButton.NormalColor = FLinearColor(0.7f, 0.3f, 0.0f, 0.8f);  // Orange
		}
		else if (CurrentState == EScannerState::Complete)
		{
			StartDiscoveryButton.ButtonText = TEXT("Reset Scanner");
			StartDiscoveryButton.NormalColor = FLinearColor(0.3f, 0.3f, 0.7f, 0.8f);  // Blue
		}
		
		DrawButton(StartDiscoveryButton);
		
		// Add hit box for button click detection
		AddHitBox(StartDiscoveryButton.Position, StartDiscoveryButton.Size, 
			FName("StartDiscoveryButton"), false, 0);
		
		// ===== DRAW START MAPPING BUTTON (BELOW DISCOVERY BUTTON) =====
		// Only show this button when we need manual mapping start
		// Show when: Discovery complete but mapping hasn't started, and auto-mapping is OFF
		bool bShowMappingButton = (CurrentState == EScannerState::Validating && 
								   ScannerCamera->GetValidationAttempts() > 0 &&
								   !ScannerCamera->bAutoStartMapping);
		
		if (bShowMappingButton)
		{
			StartMappingButton.Position = FVector2D(
				Canvas->SizeX - StartMappingButton.Size.X - ButtonPadding,
				ButtonPadding + StartDiscoveryButton.Size.Y + ButtonSpacing
			);
			
			DrawButton(StartMappingButton);
			
			AddHitBox(StartMappingButton.Position, StartMappingButton.Size,
				FName("StartMappingButton"), false, 0);
		}
		
		// ===== DRAW AUTOMATION CHECKBOXES (BELOW BUTTONS) =====
		float CheckboxYStart = StartDiscoveryButton.Position.Y + StartDiscoveryButton.Size.Y + ButtonSpacing;
		
		// Add extra space if mapping button is showing
		if (bShowMappingButton)
		{
			CheckboxYStart += StartMappingButton.Size.Y + ButtonSpacing;
		}
		
		CheckboxYStart += 5.0f;  // Extra padding before checkboxes
		
		float CheckboxSpacing = 30.0f;
		float CheckboxXPos = Canvas->SizeX - 200.0f;  // Align with button roughly
		
		// Auto-Discovery checkbox
		AutoDiscoveryCheckbox.Position = FVector2D(CheckboxXPos, CheckboxYStart);
		DrawCheckbox(AutoDiscoveryCheckbox, ScannerCamera->bAutoStartDiscovery);
		AddHitBox(AutoDiscoveryCheckbox.Position, FVector2D(AutoDiscoveryCheckbox.BoxSize, AutoDiscoveryCheckbox.BoxSize),
			FName("AutoDiscoveryCheckbox"), false, 0);
		
		// Auto-Mapping checkbox
		AutoMappingCheckbox.Position = FVector2D(CheckboxXPos, CheckboxYStart + CheckboxSpacing);
		DrawCheckbox(AutoMappingCheckbox, ScannerCamera->bAutoStartMapping);
		AddHitBox(AutoMappingCheckbox.Position, FVector2D(AutoMappingCheckbox.BoxSize, AutoMappingCheckbox.BoxSize),
			FName("AutoMappingCheckbox"), false, 0);
		
		// Auto-Reset checkbox
		AutoResetCheckbox.Position = FVector2D(CheckboxXPos, CheckboxYStart + CheckboxSpacing * 2);
		DrawCheckbox(AutoResetCheckbox, ScannerCamera->bAutoResetAfterMapping);
		AddHitBox(AutoResetCheckbox.Position, FVector2D(AutoResetCheckbox.BoxSize, AutoResetCheckbox.BoxSize),
			FName("AutoResetCheckbox"), false, 0);
	}

	float YPos = HUDYPosition;

	// ===== HEADER =====
	DrawSectionHeader(TEXT("=== SCANNER STATUS ==="), YPos);
	YPos += LineHeight * 0.5f;
	
	// ===== INPUT MODE INDICATOR =====
	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		FString ModeText;
		FLinearColor ModeColor;
		
		if (PC->bShowMouseCursor)
		{
			ModeText = TEXT("MODE: UI (Mouse Visible) - Press Tab for Camera");
			ModeColor = FLinearColor(0.0f, 1.0f, 1.0f);  // Cyan
		}
		else
		{
			ModeText = TEXT("MODE: CAMERA (Mouse Hidden) - Press Tab for UI");
			ModeColor = FLinearColor(1.0f, 0.5f, 0.0f);  // Orange
		}
		
		DrawStatusLine(ModeText, YPos, ModeColor);
		YPos += LineHeight * 0.5f;  // Extra space after mode indicator
	}
	
	// ===== CAMERA POSITION =====
	FVector CamPos = ScannerCamera->GetCameraPosition();
	DrawSectionHeader(TEXT("CAMERA:"), YPos);
	DrawStatusLine(FString::Printf(TEXT("Pos: X=%.1f Y=%.1f Z=%.1f"), 
		CamPos.X, CamPos.Y, CamPos.Z), YPos, FLinearColor(0.8f, 0.8f, 1.0f));
	DrawStatusLine(FString::Printf(TEXT("     (%.2fm, %.2fm, %.2fm)"), 
		CamPos.X/100.0f, CamPos.Y/100.0f, CamPos.Z/100.0f), YPos, FLinearColor(0.6f, 0.6f, 0.8f));
	
	// Current rotation
	FRotator CamRot = ScannerCamera->GetCameraRotation();
	DrawStatusLine(FString::Printf(TEXT("Rot: P=%.1f° Y=%.1f° R=%.1f°"), 
		CamRot.Pitch, CamRot.Yaw, CamRot.Roll), YPos, FLinearColor(1.0f, 0.9f, 0.7f));
	
	YPos += LineHeight * 0.3f;

	// ===== SCANNER STATE =====
	FString StateStr;
	switch (ScannerCamera->GetScannerState())
	{
		case EScannerState::Idle:
			StateStr = TEXT("IDLE");
			break;
		case EScannerState::Validating:
			StateStr = TEXT("VALIDATING");
			break;
		case EScannerState::Mapping:
			StateStr = TEXT("MAPPING");
			break;
		case EScannerState::Complete:
			StateStr = TEXT("COMPLETE");
			break;
		default:
			StateStr = TEXT("UNKNOWN");
	}

	DrawStatusLine(FString::Printf(TEXT("State: %s"), *StateStr), YPos, GetStateColor(StateStr));
	DrawStatusLine(FString::Printf(TEXT("Enabled: %s | Progress: %.1f%%"), 
		ScannerCamera->IsScannerEnabled() ? TEXT("YES") : TEXT("NO"),
		ScannerCamera->GetScanProgress() * 100.0f), YPos);

	YPos += LineHeight * 0.3f;

	// ===== TARGET FINDER =====
	if (ScannerCamera->IsValidating())
	{
		DrawSectionHeader(TEXT("TARGET FINDER:"), YPos);
		DrawStatusLine(FString::Printf(TEXT("Attempts: %d | Angle: %.1f°"), 
			ScannerCamera->GetValidationAttempts(),
			ScannerCamera->GetCurrentValidationAngle()), YPos, FLinearColor::Yellow);
		DrawStatusLine(TEXT("Status: Searching..."), YPos, FLinearColor::Yellow);
		YPos += LineHeight * 0.3f;
	}

	// ===== LASER INFO =====
	DrawSectionHeader(TEXT("LASER:"), YPos);
	if (ScannerCamera->GetLastShotHit())
	{
		FString HitActorName = ScannerCamera->GetLastHitActor() ? 
			ScannerCamera->GetLastHitActor()->GetName() : TEXT("Unknown");
		
		DrawStatusLine(FString::Printf(TEXT("Hit: %s (%.0f cm)"), 
			*HitActorName, ScannerCamera->GetLastHitDistance()), YPos, FLinearColor::Green);
		
		FVector HitLoc = ScannerCamera->GetLastHitLocation();
		DrawStatusLine(FString::Printf(TEXT("Loc: X=%.0f Y=%.0f Z=%.0f"), 
			HitLoc.X, HitLoc.Y, HitLoc.Z), YPos);
	}
	else
	{
		DrawStatusLine(TEXT("Hit: NONE"), YPos, FLinearColor::Red);
	}
	DrawStatusLine(FString::Printf(TEXT("Range: %.0f cm"), 
		ScannerCamera->GetLaserMaxRange()), YPos);

	YPos += LineHeight * 0.3f;
	
	// ===== TARGET INFO =====
	AActor* TargetActor = ScannerCamera->GetCinematicTargetLandscape();
	if (TargetActor)
	{
		DrawSectionHeader(TEXT("TARGET:"), YPos);
		
		// Display target label (the name shown in the outliner, like "MyCube")
		FString TargetLabel = TargetActor->GetActorLabel();
		DrawStatusLine(FString::Printf(TEXT("Name: %s"), *TargetLabel), YPos, FLinearColor(1.0f, 1.0f, 0.5f));
		
		// Get target bounds to show Z height range
		FBox TargetBounds = TargetActor->GetComponentsBoundingBox(true);
		float MinHeightMeters = TargetBounds.Min.Z / 100.0f;
		float MaxHeightMeters = TargetBounds.Max.Z / 100.0f;
		
		DrawStatusLine(FString::Printf(TEXT("Z Range: %.2fm to %.2fm"), MinHeightMeters, MaxHeightMeters), 
			YPos, FLinearColor(0.7f, 0.9f, 1.0f));
		
		// Show the scan height (what percentage we're scanning at)
		float ScanHeightMeters = ScannerCamera->GetCinematicOrbitHeight() / 100.0f;
		DrawStatusLine(FString::Printf(TEXT("Scan Height: %.2fm (%.0f%%)"), 
			ScanHeightMeters, ScannerCamera->GetCinematicHeightPercent()), 
			YPos, FLinearColor(0.5f, 1.0f, 0.5f));
		
		YPos += LineHeight * 0.3f;
	}

	// ===== AUDIO STATUS =====
	DrawSectionHeader(TEXT("AUDIO:"), YPos);
	FString AudioStatus = ScannerCamera->IsAudioEnabled() ? TEXT("Enabled") : TEXT("Disabled");
	FLinearColor AudioColor = ScannerCamera->IsAudioEnabled() ? FLinearColor::Green : FLinearColor::Gray;
	
	if (ScannerCamera->IsAudioEnabled())
	{
		FString StateAudio;
		switch (ScannerCamera->GetScannerState())
		{
			case EScannerState::Validating:
				StateAudio = TEXT("(Fast Beeping)");
				break;
			case EScannerState::Mapping:
				StateAudio = TEXT("(Slow Beeping)");
				break;
			default:
				StateAudio = TEXT("(Silent)");
		}
		AudioStatus += TEXT(" ") + StateAudio;
	}
	DrawStatusLine(AudioStatus, YPos, AudioColor);

	YPos += LineHeight * 0.3f;

	// ===== SCAN DATA =====
	if (ScannerCamera->IsCinematicScanActive() || ScannerCamera->GetRecordedDataCount() > 0)
	{
		DrawSectionHeader(TEXT("SCAN DATA:"), YPos);
		DrawStatusLine(FString::Printf(TEXT("Orbit: %.1f° | Points: %d"), 
			ScannerCamera->GetCurrentOrbitAngle(),
			ScannerCamera->GetRecordedDataCount()), YPos, FLinearColor(0.0f, 1.0f, 1.0f));  // Cyan color
		DrawStatusLine(FString::Printf(TEXT("Time: %.1fs"), 
			ScannerCamera->GetCinematicScanElapsedTime()), YPos);
		
		YPos += LineHeight * 0.3f;
	}
	
	// ===== ROTATION HISTORY (Last 10) =====
	if (RotationHistory.Num() > 0)
	{
		DrawSectionHeader(TEXT("ROTATION HISTORY (Last 10):"), YPos);
		
		// Draw from newest to oldest
		for (int32 i = RotationHistory.Num() - 1; i >= 0; i--)
		{
			const FRotator& Rot = RotationHistory[i];
			int32 SerialNum = RotationSerialNumbers[i];
			
			// Color gradient from bright (newest) to dim (oldest)
			float Alpha = (float)(i + 1) / (float)RotationHistory.Num();
			FLinearColor HistoryColor = FLinearColor(0.7f * Alpha, 0.9f * Alpha, 0.7f * Alpha);
			
			// Format: #00001: P=0.2° Y=90.5° R=0.0°
			DrawStatusLine(FString::Printf(TEXT("#%05d: P=%6.1f° Y=%6.1f° R=%6.1f°"), 
				SerialNum,
				Rot.Pitch, Rot.Yaw, Rot.Roll), 
				YPos, HistoryColor);
		}
	}
	
	// Update rotation history
	UpdateRotationHistory(ScannerCamera->GetCameraRotation(), GetWorld()->GetDeltaSeconds());
}

void ANKScannerHUD::DrawStatusLine(const FString& Text, float& YPos, FLinearColor Color)
{
	if (!Canvas)
	{
		return;
	}

	FCanvasTextItem TextItem(FVector2D(HUDXPosition, YPos), FText::FromString(Text), GEngine->GetSmallFont(), Color);
	TextItem.Scale = FVector2D(FontScale, FontScale);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);

	YPos += LineHeight;
}

void ANKScannerHUD::DrawSectionHeader(const FString& Text, float& YPos)
{
	DrawStatusLine(Text, YPos, FLinearColor(1.0f, 0.8f, 0.2f));  // Orange/yellow color
}

FLinearColor ANKScannerHUD::GetStateColor(const FString& State)
{
	if (State == TEXT("IDLE"))
	{
		return FLinearColor::Gray;
	}
	else if (State == TEXT("VALIDATING"))
	{
		return FLinearColor::Yellow;
	}
	else if (State == TEXT("MAPPING"))
	{
		return FLinearColor(0.0f, 1.0f, 1.0f);  // Cyan color (R=0, G=1, B=1)
	}
	else if (State == TEXT("COMPLETE"))
	{
		return FLinearColor::Green;
	}
	
	return FLinearColor::White;
}

void ANKScannerHUD::FindScannerCamera()
{
	if (GetWorld())
	{
		for (TActorIterator<ANKScannerCameraActor> It(GetWorld()); It; ++It)
		{
			ScannerCamera = *It;
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Found scanner camera: %s"), *ScannerCamera->GetName());
			break;  // Use the first one found
		}

		if (!ScannerCamera)
		{
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: No scanner camera found in level!"));
		}
	}
}

void ANKScannerHUD::UpdateRotationHistory(const FRotator& CurrentRotation, float DeltaTime)
{
	if (bUpdateOnMovement)
	{
		// ===== MODE 1: Update on EVERY camera movement =====
		// Check if rotation has changed since last record
		const float RotationThreshold = 0.05f;  // Degrees - very sensitive to catch small changes
		
		bool bRotationChanged = 
			!FMath::IsNearlyEqual(CurrentRotation.Pitch, LastRecordedRotation.Pitch, RotationThreshold) ||
			!FMath::IsNearlyEqual(CurrentRotation.Yaw, LastRecordedRotation.Yaw, RotationThreshold) ||
			!FMath::IsNearlyEqual(CurrentRotation.Roll, LastRecordedRotation.Roll, RotationThreshold);
		
		if (bRotationChanged)
		{
			// Rotation changed - record it!
			UE_LOG(LogTemp, Warning, TEXT("HUD: Rotation changed! Yaw: %.2f -> %.2f (diff: %.2f)"), 
				LastRecordedRotation.Yaw, CurrentRotation.Yaw, 
				FMath::Abs(CurrentRotation.Yaw - LastRecordedRotation.Yaw));
			
			LastRecordedRotation = CurrentRotation;
			
			// Increment serial number
			RotationSerialNumber++;
			
			// Add new rotation to history
			RotationHistory.Add(CurrentRotation);
			RotationSerialNumbers.Add(RotationSerialNumber);
			
			// Keep only the last MaxRotationHistory entries
			if (RotationHistory.Num() > MaxRotationHistory)
			{
				RotationHistory.RemoveAt(0);
				RotationSerialNumbers.RemoveAt(0);
			}
		}
	}
	else
	{
		// ===== MODE 2: Update on TIMER (original behavior) =====
		RotationUpdateTimer += DeltaTime;
		
		// Use faster update rate during VALIDATING state (discovery phase)
		float CurrentUpdateInterval = RotationUpdateInterval;
		if (ScannerCamera && ScannerCamera->GetScannerState() == EScannerState::Validating)
		{
			CurrentUpdateInterval = 0.1f;  // Update every 100ms during discovery
		}
		
		// Only update when timer reaches the interval
		if (RotationUpdateTimer >= CurrentUpdateInterval)
		{
			RotationUpdateTimer = 0.0f;
			
			UE_LOG(LogTemp, Warning, TEXT("HUD: Timer update! Yaw: %.2f"), CurrentRotation.Yaw);
			
			// Increment serial number
			RotationSerialNumber++;
			
			// Add new rotation to history
			RotationHistory.Add(CurrentRotation);
			RotationSerialNumbers.Add(RotationSerialNumber);
			
			// Keep only the last MaxRotationHistory entries
			if (RotationHistory.Num() > MaxRotationHistory)
			{
				RotationHistory.RemoveAt(0);
				RotationSerialNumbers.RemoveAt(0);
			}
		}
	}
}

void ANKScannerHUD::DrawButton(FHUDButton& Button)
{
	if (!Canvas)
	{
		return;
	}

	// Determine button color based on state
	FLinearColor ButtonColor = Button.NormalColor;
	if (Button.bIsPressed)
	{
		ButtonColor = Button.PressedColor;
	}
	else if (Button.bIsHovered)
	{
		ButtonColor = Button.HoverColor;
	}

	// Draw button background (simple rectangle)
	FCanvasTileItem TileItem(Button.Position, Button.Size, ButtonColor);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);

	// Draw button border
	FLinearColor BorderColor = FLinearColor::White;
	DrawDebugCanvas2DLine(Canvas, Button.Position, FVector2D(Button.Position.X + Button.Size.X, Button.Position.Y), BorderColor, 2.0f);
	DrawDebugCanvas2DLine(Canvas, FVector2D(Button.Position.X + Button.Size.X, Button.Position.Y), Button.Position + Button.Size, BorderColor, 2.0f);
	DrawDebugCanvas2DLine(Canvas, Button.Position + Button.Size, FVector2D(Button.Position.X, Button.Position.Y + Button.Size.Y), BorderColor, 2.0f);
	DrawDebugCanvas2DLine(Canvas, FVector2D(Button.Position.X, Button.Position.Y + Button.Size.Y), Button.Position, BorderColor, 2.0f);

	// Draw button text (centered)
	FVector2D TextSize;
	Canvas->TextSize(GEngine->GetMediumFont(), Button.ButtonText, TextSize.X, TextSize.Y);
	FVector2D TextPosition = Button.Position + (Button.Size - TextSize) * 0.5f;
	
	FCanvasTextItem TextItem(TextPosition, FText::FromString(Button.ButtonText), GEngine->GetMediumFont(), FLinearColor::White);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);
}

bool ANKScannerHUD::IsPointInButton(const FHUDButton& Button, const FVector2D& Point) const
{
	return Point.X >= Button.Position.X && 
		   Point.X <= Button.Position.X + Button.Size.X &&
		   Point.Y >= Button.Position.Y && 
		   Point.Y <= Button.Position.Y + Button.Size.Y;
}

void ANKScannerHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	
	if (!ScannerCamera)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Hit box clicked: %s"), *BoxName.ToString());
	
	// Handle button click - Start Discovery workflow
	if (BoxName == "StartDiscoveryButton")
	{
		StartDiscoveryButton.bIsPressed = true;
		
		EScannerState CurrentState = ScannerCamera->GetScannerState();
		AActor* Target = ScannerCamera->GetCinematicTargetLandscape();
		
		// Check if we have a target set
		if (!Target)
		{
			UE_LOG(LogTemp, Error, TEXT("NKScannerHUD: Cannot start discovery - no target landscape set!"));
			UE_LOG(LogTemp, Error, TEXT("NKScannerHUD: Please set 'Cinematic Target Landscape' in the Details panel"));
			return;
		}
		
		// State-based button behavior
		if (CurrentState == EScannerState::Idle)
		{
			// Start the discovery workflow
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Starting discovery workflow for target '%s'"), 
				*Target->GetActorLabel());
			
			ScannerCamera->StartCinematicScan(
				Target,
				ScannerCamera->CinematicHeightPercent,
				ScannerCamera->CinematicDistanceMeters,
				ScannerCamera->CinematicJSONOutputPath
			);
		}
		else if (CurrentState == EScannerState::Validating || CurrentState == EScannerState::Mapping)
		{
			// Cancel ongoing scan
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Cancelling active scan (state: %d)"), (int32)CurrentState);
			ScannerCamera->StopCinematicScan();
		}
		else if (CurrentState == EScannerState::Complete)
		{
			// Reset to idle to allow restart
			UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Resetting scanner to Idle state"));
			// TODO: Add a ResetScanner() function to properly reset state
		}
		
		return;
	}
	
	// Handle Start Mapping button click
	if (BoxName == "StartMappingButton")
	{
		StartMappingButton.bIsPressed = true;
		
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Start Mapping button clicked!"));
		
		// Manually trigger the transition from discovery to mapping
		// This would normally happen automatically if bAutoStartMapping was true
		// TODO: Add a method in Scanner Camera to manually start mapping phase
		// For now, we can enable auto-mapping temporarily to trigger the transition
		
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Transitioning from Discovery to Mapping phase"));
		
		// The scanner's UpdateTargetFinder should handle this transition
		// We just need to signal that we want to proceed
		
		return;
	}
	
	// Handle checkbox clicks - toggle the values
	if (BoxName == "AutoDiscoveryCheckbox")
	{
		ScannerCamera->bAutoStartDiscovery = !ScannerCamera->bAutoStartDiscovery;
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Auto-Discovery toggled to %s"), 
			ScannerCamera->bAutoStartDiscovery ? TEXT("ON") : TEXT("OFF"));
		return;
	}
	
	if (BoxName == "AutoMappingCheckbox")
	{
		ScannerCamera->bAutoStartMapping = !ScannerCamera->bAutoStartMapping;
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Auto-Mapping toggled to %s"), 
			ScannerCamera->bAutoStartMapping ? TEXT("ON") : TEXT("OFF"));
		return;
	}
	
	if (BoxName == "AutoResetCheckbox")
	{
		ScannerCamera->bAutoResetAfterMapping = !ScannerCamera->bAutoResetAfterMapping;
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Auto-Reset toggled to %s"), 
			ScannerCamera->bAutoResetAfterMapping ? TEXT("ON") : TEXT("OFF"));
		return;
	}
}

void ANKScannerHUD::NotifyHitBoxRelease(FName BoxName)
{
	Super::NotifyHitBoxRelease(BoxName);
	
	// Release button pressed states
	if (BoxName == "StartDiscoveryButton")
	{
		StartDiscoveryButton.bIsPressed = false;
	}
	else if (BoxName == "StartMappingButton")
	{
		StartMappingButton.bIsPressed = false;
	}
}

void ANKScannerHUD::DrawCheckbox(FHUDCheckbox& Checkbox, bool bIsChecked)
{
	if (!Canvas)
	{
		return;
	}

	// Draw checkbox box
	FLinearColor BoxDrawColor = Checkbox.bIsHovered ? FLinearColor(1.0f, 1.0f, 0.0f, 1.0f) : Checkbox.BoxColor;
	
	// Draw box background (slightly transparent dark background)
	FCanvasTileItem BoxBg(Checkbox.Position, FVector2D(Checkbox.BoxSize, Checkbox.BoxSize), FLinearColor(0.1f, 0.1f, 0.1f, 0.8f));
	BoxBg.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(BoxBg);
	
	// Draw box border
	DrawDebugCanvas2DLine(Canvas, Checkbox.Position, FVector2D(Checkbox.Position.X + Checkbox.BoxSize, Checkbox.Position.Y), BoxDrawColor, 2.0f);
	DrawDebugCanvas2DLine(Canvas, FVector2D(Checkbox.Position.X + Checkbox.BoxSize, Checkbox.Position.Y), Checkbox.Position + FVector2D(Checkbox.BoxSize, Checkbox.BoxSize), BoxDrawColor, 2.0f);
	DrawDebugCanvas2DLine(Canvas, Checkbox.Position + FVector2D(Checkbox.BoxSize, Checkbox.BoxSize), FVector2D(Checkbox.Position.X, Checkbox.Position.Y + Checkbox.BoxSize), BoxDrawColor, 2.0f);
	DrawDebugCanvas2DLine(Canvas, FVector2D(Checkbox.Position.X, Checkbox.Position.Y + Checkbox.BoxSize), Checkbox.Position, BoxDrawColor, 2.0f);

	// Draw checkmark if checked (simple X pattern)
	if (bIsChecked)
	{
		float CheckPadding = 4.0f;
		FVector2D CheckStart1 = Checkbox.Position + FVector2D(CheckPadding, CheckPadding);
		FVector2D CheckEnd1 = Checkbox.Position + FVector2D(Checkbox.BoxSize - CheckPadding, Checkbox.BoxSize - CheckPadding);
		FVector2D CheckStart2 = Checkbox.Position + FVector2D(Checkbox.BoxSize - CheckPadding, CheckPadding);
		FVector2D CheckEnd2 = Checkbox.Position + FVector2D(CheckPadding, Checkbox.BoxSize - CheckPadding);
		
		DrawDebugCanvas2DLine(Canvas, CheckStart1, CheckEnd1, Checkbox.CheckColor, 3.0f);
		DrawDebugCanvas2DLine(Canvas, CheckStart2, CheckEnd2, Checkbox.CheckColor, 3.0f);
	}

	// Draw label text next to checkbox
	FVector2D TextPosition = Checkbox.Position + FVector2D(Checkbox.BoxSize + 8.0f, 0.0f);
	FCanvasTextItem TextItem(TextPosition, FText::FromString(Checkbox.LabelText), GEngine->GetSmallFont(), Checkbox.TextColor);
	TextItem.EnableShadow(FLinearColor::Black);
	Canvas->DrawItem(TextItem);
}

bool ANKScannerHUD::IsPointInCheckbox(const FHUDCheckbox& Checkbox, const FVector2D& Point) const
{
	return Point.X >= Checkbox.Position.X && 
		   Point.X <= Checkbox.Position.X + Checkbox.BoxSize &&
		   Point.Y >= Checkbox.Position.Y && 
		   Point.Y <= Checkbox.Position.Y + Checkbox.BoxSize;
}

void ANKScannerHUD::EnableMouseCursor()
{
	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
		PC->bEnableMouseOverEvents = true;
		bMouseCursorEnabled = true;
		
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Mouse cursor ENABLED for UI interaction"));
	}
}

void ANKScannerHUD::DisableMouseCursor()
{
	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		PC->bShowMouseCursor = false;
		PC->bEnableClickEvents = false;
		PC->bEnableMouseOverEvents = false;
		bMouseCursorEnabled = false;
		
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Mouse cursor DISABLED for camera control"));
	}
}

void ANKScannerHUD::ToggleMouseCursor()
{
	if (bMouseCursorEnabled)
	{
		DisableMouseCursor();
	}
	else
	{
		EnableMouseCursor();
	}
}

void ANKScannerHUD::UpdateMouseHover()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC)
	{
		return;
	}

	// Get current mouse position
	float MouseX, MouseY;
	if (PC->GetMousePosition(MouseX, MouseY))
	{
		CurrentMousePosition = FVector2D(MouseX, MouseY);
		
		// Update button hover state
		StartDiscoveryButton.bIsHovered = IsPointInButton(StartDiscoveryButton, CurrentMousePosition);
		StartMappingButton.bIsHovered = IsPointInButton(StartMappingButton, CurrentMousePosition);
		
		// Update checkbox hover states
		AutoDiscoveryCheckbox.bIsHovered = IsPointInCheckbox(AutoDiscoveryCheckbox, CurrentMousePosition);
		AutoMappingCheckbox.bIsHovered = IsPointInCheckbox(AutoMappingCheckbox, CurrentMousePosition);
		AutoResetCheckbox.bIsHovered = IsPointInCheckbox(AutoResetCheckbox, CurrentMousePosition);
	}
}

void ANKScannerHUD::HandleMouseClick()
{
	if (!ScannerCamera)
	{
		return;
	}

	// Check button click
	if (IsPointInButton(StartDiscoveryButton, CurrentMousePosition))
	{
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Start Discovery button clicked!"));
		// TODO: Implement button action based on current state
		// For now, just log the click
		return;
	}

	if (IsPointInButton(StartMappingButton, CurrentMousePosition))
	{
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Start Mapping button clicked!"));
		// TODO: Implement button action based on current state
		// For now, just log the click
		return;
	}

	// Check checkbox clicks
	if (IsPointInCheckbox(AutoDiscoveryCheckbox, CurrentMousePosition))
	{
		ScannerCamera->bAutoStartDiscovery = !ScannerCamera->bAutoStartDiscovery;
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Auto-Discovery toggled to %s"), 
			ScannerCamera->bAutoStartDiscovery ? TEXT("ON") : TEXT("OFF"));
		return;
	}

	if (IsPointInCheckbox(AutoMappingCheckbox, CurrentMousePosition))
	{
		ScannerCamera->bAutoStartMapping = !ScannerCamera->bAutoStartMapping;
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Auto-Mapping toggled to %s"), 
			ScannerCamera->bAutoStartMapping ? TEXT("ON") : TEXT("OFF"));
		return;
	}

	if (IsPointInCheckbox(AutoResetCheckbox, CurrentMousePosition))
	{
		ScannerCamera->bAutoResetAfterMapping = !ScannerCamera->bAutoResetAfterMapping;
		UE_LOG(LogTemp, Warning, TEXT("NKScannerHUD: Auto-Reset toggled to %s"), 
			ScannerCamera->bAutoResetAfterMapping ? TEXT("ON") : TEXT("OFF"));
		return;
	}
}

bool ANKScannerHUD::ProcessConsoleExec(const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)
{
	// Intercept left mouse button clicks
	if (FParse::Command(&Cmd, TEXT("LeftMouseClick")))
	{
		HandleMouseClick();
		return true;
	}
	
	return Super::ProcessConsoleExec(Cmd, Ar, Executor);
}
