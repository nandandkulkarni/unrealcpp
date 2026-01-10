// Fill out your copyright notice in the Description page of Project Settings.

#include "Scanner/UI/NKMappingScannerHUD.h"
#include "Scanner/NKMappingCamera.h"
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
		
		DrawLine(TEXT("FIRST HIT DETAILS:"), YPos, HUDColors::Success);
		DrawLine(FString::Printf(TEXT("• Hit Actor: %s"), 
			Hit.GetActor() ? *Hit.GetActor()->GetName() : TEXT("None")), YPos);
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
		DrawLine(TEXT("TARGET:"), YPos, HUDColors::Header);
		DrawLine(FString::Printf(TEXT("• Name: %s"), *MappingCamera->TargetActor->GetName()), YPos);
		
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
}

void ANKMappingScannerHUD::DrawRightSideButtons()
{
	if (!Canvas) return;
	
	float ButtonPadding = 20.0f;
	float ButtonSpacing = 10.0f;
	
	// ===== START DISCOVERY BUTTON (state-based) =====
	EMappingScannerState State = MappingCamera->GetScannerState();
	
	if (State == EMappingScannerState::Discovering)
	{
		StartDiscoveryButton.ButtonText = TEXT("Cancel Discovery");
		StartDiscoveryButton.NormalColor = HUDColors::ButtonCancel;
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
}

void ANKMappingScannerHUD::NotifyHitBoxClick(FName BoxName)
{
	Super::NotifyHitBoxClick(BoxName);
	
	if (!MappingCamera) return;
	
	if (BoxName == "StartDiscoveryButton")
	{
		if (MappingCamera->IsDiscovering())
		{
			MappingCamera->Stop();
		}
		else
		{
			MappingCamera->StartDiscovery();
		}
	}
	else if (BoxName == "ClearLinesButton")
	{
		MappingCamera->ClearDiscoveryLines();
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
