// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NKMappingScannerHUD.generated.h"

// Forward declarations
class ANKMappingCamera;

/**
 * HUD Color Constants
 */
namespace HUDColors
{
	// Mode colors
	constexpr FLinearColor ControlMode = FLinearColor(0.0f, 1.0f, 1.0f, 1.0f);      // Cyan
	constexpr FLinearColor ScanningMode = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);     // Green
	
	// UI element colors
	constexpr FLinearColor Header = FLinearColor(1.0f, 1.0f, 0.5f, 1.0f);           // Yellow-ish
	constexpr FLinearColor Success = FLinearColor(0.0f, 1.0f, 0.0f, 1.0f);          // Green
	constexpr FLinearColor Warning = FLinearColor(1.0f, 1.0f, 0.0f, 1.0f);          // Yellow
	constexpr FLinearColor Error = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);            // Red
	constexpr FLinearColor Info = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);             // White
	constexpr FLinearColor SubText = FLinearColor(0.7f, 0.7f, 0.7f, 1.0f);          // Gray
	
	// Progress colors
	constexpr FLinearColor Progress = FLinearColor(1.0f, 0.8f, 0.0f, 1.0f);         // Orange-Yellow
	
	// Button colors
	constexpr FLinearColor ButtonNormal = FLinearColor(0.2f, 0.4f, 0.6f, 0.8f);     // Blue
	constexpr FLinearColor ButtonHover = FLinearColor(0.3f, 0.5f, 0.7f, 0.9f);      // Lighter Blue
	constexpr FLinearColor ButtonCancel = FLinearColor(0.7f, 0.3f, 0.0f, 0.8f);     // Orange
	constexpr FLinearColor ButtonDelete = FLinearColor(0.6f, 0.2f, 0.1f, 0.8f);     // Red-Orange
	constexpr FLinearColor ButtonDeleteHover = FLinearColor(0.7f, 0.3f, 0.2f, 0.9f); // Lighter Red-Orange
}

/**
 * Simple button structure for HUD buttons
 */
USTRUCT()
struct FSimpleHUDButton
{
	GENERATED_BODY()

	FString ButtonText;
	FVector2D Position;
	FVector2D Size;
	FLinearColor NormalColor;
	FLinearColor HoverColor;
	bool bIsHovered;

	FSimpleHUDButton()
		: ButtonText(TEXT("Button"))
		, Position(FVector2D::ZeroVector)
		, Size(FVector2D(180.0f, 50.0f))
		, NormalColor(HUDColors::ButtonNormal)
		, HoverColor(HUDColors::ButtonHover)
		, bIsHovered(false)
	{
	}
};

/**
 * Simplified HUD for new mapping scanner
 * Only handles discovery phase for now
 */
UCLASS()
class TPCPP_API ANKMappingScannerHUD : public AHUD
{
	GENERATED_BODY()

public:
	ANKMappingScannerHUD();

	virtual void DrawHUD() override;
	virtual void NotifyHitBoxClick(FName BoxName) override;

protected:
	virtual void BeginPlay() override;

private:
	// ===== Scanner Reference =====
	
	UPROPERTY()
	ANKMappingCamera* MappingCamera;
	
	// ===== HUD Settings =====
	
	UPROPERTY(EditAnywhere, Category = "HUD")
	float LeftMargin = 20.0f;
	
	UPROPERTY(EditAnywhere, Category = "HUD")
	float TopMargin = 20.0f;
	
	UPROPERTY(EditAnywhere, Category = "HUD")
	float LineHeight = 20.0f;
	
	UPROPERTY(EditAnywhere, Category = "HUD")
	float FontScale = 1.0f;
	
	// Background settings
	UPROPERTY(EditAnywhere, Category = "HUD|Background")
	bool bShowBackground = true;  // Toggle background on/off
	
	UPROPERTY(EditAnywhere, Category = "HUD|Background", meta = (EditCondition = "bShowBackground"))
	FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);  // Semi-transparent black
	
	UPROPERTY(EditAnywhere, Category = "HUD|Background", meta = (EditCondition = "bShowBackground"))
	float BackgroundPadding = 10.0f;  // Padding around text
	
	// ===== Mode State =====
	
	bool bUIMode;  // true = Input Controls Enabled (mouse visible, can click UI), false = Input Controls Disabled (camera movement, mouse hidden)
	
	// ===== Buttons =====
	
	FSimpleHUDButton StartDiscoveryButton;
	FSimpleHUDButton ClearLinesButton;
	
	// Dynamic camera buttons (created based on available cameras)
	TArray<FSimpleHUDButton> CameraButtons;
	
	// ===== Helper Methods =====
	
	void FindMappingCamera();
	void DrawLeftSideInfo(float& YPos);
	void DrawRightSideButtons();
	void DrawLine(const FString& Text, float& YPos, FLinearColor Color = FLinearColor::White);
	void DrawButton(FSimpleHUDButton& Button);
	bool IsPointInButton(const FSimpleHUDButton& Button, const FVector2D& Point) const;
	void UpdateButtonHover();
	
	void UpdateCameraButtons();  // Rebuild camera button list
	void DrawCameraInfo(float& YPos);  // Display current camera info
	
	FString GetStateDisplayName(uint8 State) const;
};
