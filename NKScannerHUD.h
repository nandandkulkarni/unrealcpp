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

	// Helper functions for drawing
	void DrawStatusLine(const FString& Text, float& YPos, FLinearColor Color = FLinearColor::White);
	void DrawSectionHeader(const FString& Text, float& YPos);
	FLinearColor GetStateColor(const FString& State);
	
	// Find scanner camera in the level
	void FindScannerCamera();
};
