// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "NKScannerHUD.generated.h"

class ANKScannerCameraActor;

/**
 * HUD class that displays detailed scanner status information
 * Shows real-time data in the top-left corner of the screen
 */
UCLASS()
class TPCPP_API ANKScannerHUD : public AHUD
{
	GENERATED_BODY()

public:
	ANKScannerHUD();

	// Override DrawHUD to render scanner status
	virtual void DrawHUD() override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	// Reference to the scanner camera actor
	UPROPERTY()
	ANKScannerCameraActor* ScannerCamera;

	// HUD display settings
	UPROPERTY(EditAnywhere, Category = "HUD Settings")
	float HUDXPosition;

	UPROPERTY(EditAnywhere, Category = "HUD Settings")
	float HUDYPosition;

	UPROPERTY(EditAnywhere, Category = "HUD Settings")
	float LineHeight;

	UPROPERTY(EditAnywhere, Category = "HUD Settings")
	float FontScale;
	
	// Camera rotation history (last 10 rotations)
	TArray<FRotator> RotationHistory;
	int32 MaxRotationHistory;
	float RotationUpdateTimer;
	float RotationUpdateInterval;
	int32 RotationSerialNumber;  // Unique serial number for each rotation
	TArray<int32> RotationSerialNumbers;  // Serial numbers matching RotationHistory
	FRotator LastRecordedRotation;  // Track last recorded rotation to detect changes
	bool bUpdateOnMovement;  // If true, update history on every movement instead of timer
	
	// Performance optimization - cache formatted strings
	int32 FramesSinceLastHUDUpdate;
	int32 HUDUpdateFrequency;  // Update HUD every N frames (1 = every frame, 2 = every other frame)

	// Helper functions for drawing
	void DrawStatusLine(const FString& Text, float& YPos, FLinearColor Color = FLinearColor::White);
	void DrawSectionHeader(const FString& Text, float& YPos);
	FLinearColor GetStateColor(const FString& State);
	
	// Find scanner camera in the level
	void FindScannerCamera();
	
	// Update rotation history
	void UpdateRotationHistory(const FRotator& CurrentRotation, float DeltaTime);
};
