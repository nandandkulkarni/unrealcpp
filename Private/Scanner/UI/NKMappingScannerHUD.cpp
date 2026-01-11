// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/UI/NKMappingScannerHUD.h"
#include "Scanner/NKMappingCamera.h"
#include "Scanner/NKScannerPlayerController.h"
#include "Scanner/Utilities/NKScannerLogger.h"
#include "Engine/Canvas.h"
#include "Kismet/GameplayStatics.h"

ANKMappingScannerHUD::ANKMappingScannerHUD()
	: MappingCamera(nullptr)
	, bUIMode(false)  // Start with Input Controls Disabled
{
	// Initialize buttons
	StartDiscoveryButton.ButtonText = TEXT("Start Discovery");
	StartDiscoveryButton.Size = FVector2D(180.0f, 50.0f);
	StartDiscoveryButton.NormalColor = HUDColors::ButtonNormal;
	StartDiscoveryButton.HoverColor = HUDColors::ButtonHover;
	
	ClearLinesButton.ButtonText = TEXT("Clear Discovery Lines");
	ClearLinesButton.Size = FVector2D(200.0f, 40.0f);
	ClearLinesButton.NormalColor = HUDColors::ButtonDelete;
	ClearLinesButton.HoverColor = HUDColors::ButtonDeleteHover;
}

void ANKMappingScannerHUD::BeginPlay()
{
	Super::BeginPlay();
	FindMappingCamera();
	
	// Initialize camera buttons
	UpdateCameraButtons();
}

void ANKMappingScannerHUD::DrawHUD()
{
	Super::DrawHUD();
	
	if (!MappingCamera)
	{
		FindMappingCamera();
		if (!MappingCamera)
		{
			// Draw instructions for setting up the scanner
			DrawText(TEXT("SCANNER SETUP REQUIRED"), 
				HUDColors::Warning, 100, 100, nullptr, 2.0f);
			DrawText(TEXT("1. Place ANKMappingCamera actor in your level"), 
				HUDColors::Info, 100, 140, nullptr, 1.2f);
			DrawText(TEXT("2. The GameMode will automatically use this HUD"), 
				HUDColors::Info, 100, 165, nullptr, 1.2f);
			DrawText(TEXT("3. Press Tab to toggle between Camera and UI modes"), 
				HUDColors::Info, 100, 190, nullptr, 1.2f);
			return;
		}
	}
	
	// Sync UI mode with PlayerController's mouse state
	APlayerController* PC = GetOwningPlayerController();
	if (PC)
	{
		bUIMode = PC->bShowMouseCursor;
	}
	
	// Update camera buttons if needed (PlayerController might not have been ready in BeginPlay)
	ANKScannerPlayerController* ScannerPC = Cast<ANKScannerPlayerController>(PC);
	if (ScannerPC && CameraButtons.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("HUD: Camera buttons empty, refreshing..."));
		UpdateCameraButtons();
	}
	
	// Update button hover states
	UpdateButtonHover();
	
	// Draw left side info
	float YPos = TopMargin;
	DrawLeftSideInfo(YPos);
	
	// Draw right side buttons
	DrawRightSideButtons();
}

void ANKMappingScannerHUD::FindMappingCamera()
{
	MappingCamera = Cast<ANKMappingCamera>(
		UGameplayStatics::GetActorOfClass(GetWorld(), ANKMappingCamera::StaticClass()));
}

void ANKMappingScannerHUD::DrawLeftSideInfo(float& YPos)
{
	// ===== DRAW BACKGROUND (if enabled) =====
	if (bShowBackground && Canvas)
	{
		// Calculate background height (approximate based on content)
		float BackgroundHeight = Canvas->SizeY - TopMargin - 20.0f;
		float BackgroundWidth = 500.0f;  // Fixed width for left panel
		
		FVector2D BackgroundPos(LeftMargin - BackgroundPadding, TopMargin - BackgroundPadding);
		FVector2D BackgroundSize(BackgroundWidth, BackgroundHeight);
		
		// Draw semi-transparent background
		FCanvasTileItem BackgroundTile(BackgroundPos, BackgroundSize, BackgroundColor);
		BackgroundTile.BlendMode = SE_BLEND_Translucent;
		Canvas->DrawItem(BackgroundTile);
	}
	
	// ===== MODE DISPLAY (TOP SECTION) =====
	FString ModeName = bUIMode ? TEXT("Input Controls Enabled") : TEXT("Input Controls Disabled");
	FString ModeInstruction = bUIMode ? 
		TEXT("Press Tab to disable input controls") : 
		TEXT("Press Tab to enable input controls");
	FLinearColor ModeColor = bUIMode ? HUDColors::ScanningMode : HUDColors::ControlMode;
	
	DrawLine(FString::Printf(TEXT("MODE: %s"), *ModeName), YPos, ModeColor);
	DrawLine(ModeInstruction, YPos, HUDColors::SubText);
	YPos += LineHeight * 0.5f;
	
	// ===== SCANNER STATE =====
	DrawLine(TEXT("SCANNER STATUS:"), YPos, HUDColors::Header);
	FString StateName = GetStateDisplayName((uint8)MappingCamera->GetScannerState());
	DrawLine(FString::Printf(TEXT("• State: %s"), *StateName), YPos);
	YPos += LineHeight * 0.5f;
	
	// ===== DISCOVERY PROGRESS (only when discovering) =====
	if (MappingCamera->IsDiscovering())
	{
		DrawLine(TEXT("DISCOVERY:"), YPos, HUDColors::Header);
		DrawLine(FString::Printf(TEXT("• Shot %d | Angle %.1f°"), 
			MappingCamera->GetDiscoveryShotCount(),
			MappingCamera->GetDiscoveryAngle()), YPos, HUDColors::Warning);
		DrawLine(FString::Printf(TEXT("• Progress: %.1f%% of 360° sweep"), 
			MappingCamera->GetDiscoveryProgress()), YPos, HUDColors::Progress);
		YPos += LineHeight * 0.5f;
	}
	
	// ===== FIRST HIT DETAILS (only when discovered) =====
	if (MappingCamera->GetScannerState() == EMappingScannerState::Discovered && MappingCamera->HasFirstHit())
	{
		FHitResult Hit = MappingCamera->GetFirstHitResult();
		float Angle = MappingCamera->GetFirstHitAngle();
		FVector CamPos = MappingCamera->GetFirstHitCameraPosition();
		FRotator CamRot = MappingCamera->GetFirstHitCameraRotation();
		
		AActor* HitActor = Hit.GetActor();
		FString HitActorLabel = HitActor ? HitActor->GetActorLabel() : TEXT("None");
		FString HitActorName = HitActor ? HitActor->GetName() : TEXT("None");
		FString ComponentName = Hit.Component.IsValid() ? Hit.Component->GetName() : TEXT("None");
		FString ComponentClass = Hit.Component.IsValid() ? Hit.Component->GetClass()->GetName() : TEXT("None");
		
		DrawLine(TEXT("FIRST HIT DETAILS:"), YPos, HUDColors::Success);
		DrawLine(FString::Printf(TEXT("• Hit Actor: '%s'"), *HitActorLabel), YPos);
		DrawLine(FString::Printf(TEXT("  (%s)"), *HitActorName), YPos, HUDColors::SubText);
		DrawLine(FString::Printf(TEXT("• Component: %s"), *ComponentName), YPos);
		DrawLine(FString::Printf(TEXT("  (%s)"), *ComponentClass), YPos, HUDColors::SubText);
		DrawLine(FString::Printf(TEXT("• Hit Angle: %.1f°"), Angle), YPos);
		DrawLine(FString::Printf(TEXT("• Hit Distance: %.1f cm (%.2f m)"), 
			Hit.Distance, Hit.Distance / 100.0f), YPos);
		DrawLine(FString::Printf(TEXT("• Hit Location:")), YPos);
		DrawLine(FString::Printf(TEXT("  X=%.1f Y=%.1f Z=%.1f"), 
			Hit.Location.X, Hit.Location.Y, Hit.Location.Z), YPos);
		DrawLine(FString::Printf(TEXT("  (%.2fm, %.2fm, %.2fm)"), 
			Hit.Location.X/100.0f, Hit.Location.Y/100.0f, Hit.Location.Z/100.0f), YPos);
		
		YPos += LineHeight * 0.3f;
		DrawLine(TEXT("CAMERA AT HIT:"), YPos, HUDColors::ControlMode);
		DrawLine(FString::Printf(TEXT("• Pos: X=%.1f Y=%.1f Z=%.1f"), 
			CamPos.X, CamPos.Y, CamPos.Z), YPos);
		DrawLine(FString::Printf(TEXT("       (%.2fm, %.2fm, %.2fm)"), 
			CamPos.X/100.0f, CamPos.Y/100.0f, CamPos.Z/100.0f), YPos);
		DrawLine(FString::Printf(TEXT("• Rot: P=%.1f° Y=%.1f° R=%.1f°"), 
			CamRot.Pitch, CamRot.Yaw, CamRot.Roll), YPos);
		YPos += LineHeight * 0.5f;
	}
	
	// ===== TARGET INFO =====
	if (MappingCamera->TargetActor)
	{
		FString TargetLabel = MappingCamera->TargetActor->GetActorLabel();
		FString TargetName = MappingCamera->TargetActor->GetName();
		
		DrawLine(TEXT("TARGET:"), YPos, HUDColors::Header);
		DrawLine(FString::Printf(TEXT("• Name: '%s'"), *TargetLabel), YPos);
		DrawLine(FString::Printf(TEXT("  (%s)"), *TargetName), YPos, HUDColors::SubText);
		
		FBox Bounds = MappingCamera->TargetActor->GetComponentsBoundingBox(true);
		FVector Size = Bounds.GetSize();
		DrawLine(FString::Printf(TEXT("• Size: %.1fm × %.1fm × %.1fm"), 
			Size.X/100.0f, Size.Y/100.0f, Size.Z/100.0f), YPos);
		YPos += LineHeight * 0.5f;
	}
	
	// ===== CAMERA INFO =====
	DrawLine(TEXT("CAMERA:"), YPos, HUDColors::Header);
	FVector CamPos = MappingCamera->GetActorLocation();
	DrawLine(FString::Printf(TEXT("• Pos: X=%.1f Y=%.1f Z=%.1f"), 
		CamPos.X, CamPos.Y, CamPos.Z), YPos);
	DrawLine(FString::Printf(TEXT("       (%.2fm, %.2fm, %.2fm)"), 
		CamPos.X/100.0f, CamPos.Y/100.0f, CamPos.Z/100.0f), YPos);
	
	FRotator CamRot = MappingCamera->GetActorRotation();
	DrawLine(FString::Printf(TEXT("• Rot: P=%.1f° Y=%.1f° R=%.1f°"), 
		CamRot.Pitch, CamRot.Yaw, CamRot.Roll), YPos);
	
	// ===== LOGGING INFO =====
	if (UNKScannerLogger* Logger = UNKScannerLogger::Get(GetWorld()))
	{
		YPos += LineHeight * 0.5f;
		DrawLine(TEXT("LOGGING:"), YPos, HUDColors::Header);
		
		// Logging status
		FString LogStatus = Logger->bEnableLogging ? TEXT("Enabled") : TEXT("Disabled");
		FLinearColor StatusColor = Logger->bEnableLogging ? HUDColors::Success : HUDColors::SubText;
		DrawLine(FString::Printf(TEXT("• Status: %s"), *LogStatus), YPos, StatusColor);
		
		// File logging status and path
		if (Logger->bEnableLogging && Logger->bLogToFile)
		{
			DrawLine(TEXT("• File Logging: Enabled"), YPos, HUDColors::Success);
			
			// Get the full resolved path
			FString ResolvedLogPath = Logger->GetResolvedLogFilePath();
			
			// Split path if too long for display
			if (ResolvedLogPath.Len() > 80)
			{
				DrawLine(TEXT("• Log File:"), YPos, HUDColors::Info);
				
				// Extract filename
				FString FileName = FPaths::GetCleanFilename(ResolvedLogPath);
				FString Directory = FPaths::GetPath(ResolvedLogPath);
				
				DrawLine(FString::Printf(TEXT("  %s"), *FileName), YPos, HUDColors::SubText);
				DrawLine(FString::Printf(TEXT("  in: %s"), *Directory), YPos, HUDColors::SubText);
			}
			else
			{
				DrawLine(FString::Printf(TEXT("• Log File: %s"), *ResolvedLogPath), YPos, HUDColors::Info);
			}
		}
		else if (Logger->bEnableLogging)
		{
			DrawLine(TEXT("• File Logging: Disabled (Output window only)"), YPos, HUDColors::SubText);
		}
	}
}

void ANKMappingScannerHUD::DrawRightSideButtons()
{
	if (!Canvas) return;
	
	float ButtonPadding = 20.0f;
	float ButtonSpacing = 10.0f;
	
	// ===== START DISCOVERY BUTTON (state-based) =====
	EMappingScannerState State = MappingCamera->GetScannerState();
	
	// Set button text and color based on scanner state
	if (State == EMappingScannerState::Discovering)
	{
		StartDiscoveryButton.ButtonText = TEXT("Cancel Discovery");
		StartDiscoveryButton.NormalColor = HUDColors::ButtonCancel;
	}
	else if (State == EMappingScannerState::Discovered)
	{
		StartDiscoveryButton.ButtonText = TEXT("Start Mapping");
		StartDiscoveryButton.NormalColor = HUDColors::ButtonNormal;
	}
	else
	{
		StartDiscoveryButton.ButtonText = TEXT("Start Discovery");
		StartDiscoveryButton.NormalColor = HUDColors::ButtonNormal;
	}
	
	StartDiscoveryButton.Position = FVector2D(
		Canvas->SizeX - StartDiscoveryButton.Size.X - ButtonPadding,
		ButtonPadding
	);
	
	DrawButton(StartDiscoveryButton);
	AddHitBox(StartDiscoveryButton.Position, StartDiscoveryButton.Size, 
		FName("StartDiscoveryButton"), false, 0);
	
	// ===== CLEAR LINES BUTTON =====
	ClearLinesButton.Position = FVector2D(
		Canvas->SizeX - ClearLinesButton.Size.X - ButtonPadding,
		ButtonPadding + StartDiscoveryButton.Size.Y + ButtonSpacing
	);
	
	DrawButton(ClearLinesButton);
	AddHitBox(ClearLinesButton.Position, ClearLinesButton.Size,
		FName("ClearLinesButton"), false, 0);
	
	// ===== CAMERA BUTTONS (Dynamic) =====
	float CurrentYPos = ButtonPadding + StartDiscoveryButton.Size.Y + ClearLinesButton.Size.Y + (ButtonSpacing * 2);
	
	ANKScannerPlayerController* ScannerPC = Cast<ANKScannerPlayerController>(GetOwningPlayerController());
	int32 CurrentCameraIndex = ScannerPC ? ScannerPC->GetCurrentCameraIndex() : -1;
	
	for (int32 i = 0; i < CameraButtons.Num(); i++)
	{
		FSimpleHUDButton& CameraButton = CameraButtons[i];
		
		// Highlight current camera
		if (i == CurrentCameraIndex)
		{
			CameraButton.NormalColor = HUDColors::Success;  // Green for active camera
		}
		else
		{
			CameraButton.NormalColor = HUDColors::ButtonNormal;
		}
		
		CameraButton.Position = FVector2D(
			Canvas->SizeX - CameraButton.Size.X - ButtonPadding,
			CurrentYPos
		);
		
		DrawButton(CameraButton);
		AddHitBox(CameraButton.Position, CameraButton.Size,
			FName(*FString::Printf(TEXT("CameraButton_%d"), i)), false, 0);
		
		CurrentYPos += CameraButton.Size.Y + ButtonSpacing;
	}
}

void ANKMappingScannerHUD::DrawLine(const FString& Text, float& YPos, FLinearColor Color)
{
	DrawText(Text, Color, LeftMargin, YPos, nullptr, FontScale);
	YPos += LineHeight;
}

void ANKMappingScannerHUD::DrawButton(FSimpleHUDButton& Button)
{
	if (!Canvas) return;
	
	FLinearColor CurrentColor = Button.bIsHovered ? Button.HoverColor : Button.NormalColor;
	
	// Draw button background
	FCanvasTileItem TileItem(Button.Position, Button.Size, CurrentColor);
	TileItem.BlendMode = SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);
	
	// Draw button border
	FCanvasBoxItem BoxItem(Button.Position, Button.Size);
	BoxItem.SetColor(FLinearColor::White);
	Canvas->DrawItem(BoxItem);
	
	// Draw button text (centered)
	float TextX = Button.Position.X + (Button.Size.X / 2.0f) - (Button.ButtonText.Len() * 4.0f);
	float TextY = Button.Position.Y + (Button.Size.Y / 2.0f) - 8.0f;
	DrawText(Button.ButtonText, FLinearColor::White, TextX, TextY, nullptr, 1.0f);
}

bool ANKMappingScannerHUD::IsPointInButton(const FSimpleHUDButton& Button, const FVector2D& Point) const
{
	return Point.X >= Button.Position.X && Point.X <= (Button.Position.X + Button.Size.X) &&
		   Point.Y >= Button.Position.Y && Point.Y <= (Button.Position.Y + Button.Size.Y);
}

void ANKMappingScannerHUD::UpdateButtonHover()
{
	APlayerController* PC = GetOwningPlayerController();
	if (!PC) return;
	
	float MouseX, MouseY;
	PC->GetMousePosition(MouseX, MouseY);
	FVector2D MousePos(MouseX, MouseY);
	
	StartDiscoveryButton.bIsHovered = IsPointInButton(StartDiscoveryButton, MousePos);
	ClearLinesButton.bIsHovered = IsPointInButton(ClearLinesButton, MousePos);
	
	// Update hover for all camera buttons
	for (FSimpleHUDButton& CameraButton : CameraButtons)
	{
		CameraButton.bIsHovered = IsPointInButton(CameraButton, MousePos);
	}
}

void ANKMappingScannerHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	
	if (!MappingCamera) return;
	
	if (BoxName == "StartDiscoveryButton")
	{
		EMappingScannerState State = MappingCamera->GetScannerState();
		
		if (State == EMappingScannerState::Discovering)
		{
			MappingCamera->Stop();
		}
		else if (State == EMappingScannerState::Discovered)
		{
			// ✅ User clicks "Start Mapping" after successful discovery
			MappingCamera->StartMapping();  // Uses persisted config!
		}
		else
		{
			MappingCamera->StartDiscovery();
		}
	}
	else if (BoxName == "ClearLinesButton")
	{
		MappingCamera->ClearDiscoveryLines();
		UE_LOG(LogTemp, Log, TEXT("HUD: Clear Discovery Lines button clicked"));
	}
	else if (BoxName.ToString().StartsWith(TEXT("CameraButton_")))
	{
		// Extract camera index from button name
		FString IndexStr = BoxName.ToString().RightChop(13);  // Remove "CameraButton_"
		int32 CameraIndex = FCString::Atoi(*IndexStr);
		
		UE_LOG(LogTemp, Warning, TEXT("HUD: Camera button %d clicked"), CameraIndex);
		
		// Call PlayerController to switch camera
		ANKScannerPlayerController* ScannerPC = Cast<ANKScannerPlayerController>(GetOwningPlayerController());
		if (ScannerPC)
		{
			UE_LOG(LogTemp, Warning, TEXT("  Switching to camera index %d"), CameraIndex);
			ScannerPC->SwitchToCamera(CameraIndex);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("  PlayerController is not ANKScannerPlayerController!"));
		}
	}
}

FString ANKMappingScannerHUD::GetStateDisplayName(uint8 State) const
{
	switch ((EMappingScannerState)State)
	{
	case EMappingScannerState::Idle: return TEXT("Idle");
	case EMappingScannerState::Discovering: return TEXT("Discovering");
	case EMappingScannerState::Discovered: return TEXT("✅ Discovered");
	case EMappingScannerState::DiscoveryFailed: return TEXT("❌ Discovery Failed");
	case EMappingScannerState::DiscoveryCancelled: return TEXT("⏹️ Discovery Cancelled");
	case EMappingScannerState::Mapping: return TEXT("Mapping");
	case EMappingScannerState::Complete: return TEXT("Complete");
	default: return TEXT("Unknown");
	}
}

void ANKMappingScannerHUD::UpdateCameraButtons()
{
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
	UE_LOG(LogTemp, Warning, TEXT("HUD::UpdateCameraButtons() called"));
	
	CameraButtons.Empty();
	
	ANKScannerPlayerController* ScannerPC = Cast<ANKScannerPlayerController>(GetOwningPlayerController());
	if (!ScannerPC)
	{
		UE_LOG(LogTemp, Error, TEXT("  PlayerController is not ANKScannerPlayerController!"));
		UE_LOG(LogTemp, Error, TEXT("  Actual type: %s"), 
			GetOwningPlayerController() ? *GetOwningPlayerController()->GetClass()->GetName() : TEXT("NULL"));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("  PlayerController is correct type"));
	
	int32 CameraCount = ScannerPC->GetCameraCount();
	UE_LOG(LogTemp, Warning, TEXT("  Camera count: %d"), CameraCount);
	
	if (CameraCount == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("  No cameras found! PlayerController hasn't discovered cameras yet."));
		UE_LOG(LogTemp, Warning, TEXT("========================================"));
		return;
	}
	
	for (int32 i = 0; i < CameraCount; i++)
	{
		FSimpleHUDButton CameraButton;
		CameraButton.ButtonText = ScannerPC->GetCameraName(i);
		CameraButton.Size = FVector2D(220.0f, 35.0f);
		CameraButton.NormalColor = HUDColors::ButtonNormal;
		CameraButton.HoverColor = HUDColors::ButtonHover;
		
		CameraButtons.Add(CameraButton);
		
		UE_LOG(LogTemp, Warning, TEXT("  Created button %d: %s"), i, *CameraButton.ButtonText);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("  Total buttons created: %d"), CameraButtons.Num());
	UE_LOG(LogTemp, Warning, TEXT("========================================"));
}

void ANKMappingScannerHUD::DrawCameraInfo(float& YPos)
{
	ANKScannerPlayerController* ScannerPC = Cast<ANKScannerPlayerController>(GetOwningPlayerController());
	if (!ScannerPC)
	{
		return;
	}
	
	YPos += LineHeight * 0.5f;
	DrawLine(TEXT("ACTIVE CAMERA:"), YPos, HUDColors::Header);
	
	FString CurrentCameraName = ScannerPC->GetCurrentCameraName();
	int32 CurrentIndex = ScannerPC->GetCurrentCameraIndex();
	int32 TotalCameras = ScannerPC->GetCameraCount();
	
	DrawLine(FString::Printf(TEXT("• %s"), *CurrentCameraName), YPos, HUDColors::Success);
	DrawLine(FString::Printf(TEXT("• Camera %d of %d"), CurrentIndex + 1, TotalCameras), YPos, HUDColors::SubText);
}
