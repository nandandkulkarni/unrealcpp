#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NKScannerPlayerController.generated.h"

/**
 * Custom PlayerController for scanner system
 * Handles camera switching and input management
 */
UCLASS()
class TPCPP_API ANKScannerPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ANKScannerPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	// ===== Input Mode Toggle =====
	
	/**
	 * Toggle between UI mode (mouse visible, can click buttons) and Game mode (camera control)
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void ToggleUIMode();
	
	/**
	 * Move the ViewTarget camera forward (Arrow Up)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void MoveCameraForward();
	
	/**
	 * Move the ViewTarget camera backward (Arrow Down)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void MoveCameraBackward();
	
	/**
	 * Move the ViewTarget camera left (Arrow Left)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void MoveCameraLeft();
	
	/**
	 * Move the ViewTarget camera right (Arrow Right)
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void MoveCameraRight();
	
	// ===== Camera Management =====
	
	/**
	 * Find all available cameras in the level
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void FindAllCameras();
	
	/**
	 * Switch to next camera in list
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SwitchToNextCamera();
	
	/**
	 * Switch to previous camera in list
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SwitchToPreviousCamera();
	
	/**
	 * Switch to specific camera by index
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SwitchToCamera(int32 CameraIndex);
	
	/**
	 * Switch to mapping camera specifically
	 */
	UFUNCTION(BlueprintCallable, Category = "Camera")
	void SwitchToMappingCamera();
	
	/**
	 * Get current camera index
	 */
	UFUNCTION(BlueprintPure, Category = "Camera")
	int32 GetCurrentCameraIndex() const { return CurrentCameraIndex; }
	
	/**
	 * Get total number of cameras
	 */
	UFUNCTION(BlueprintPure, Category = "Camera")
	int32 GetCameraCount() const { return AvailableCameras.Num(); }
	
	/**
	 * Get name of camera at index
	 */
	UFUNCTION(BlueprintPure, Category = "Camera")
	FString GetCameraName(int32 Index) const;
	
	/**
	 * Get current camera name
	 */
	UFUNCTION(BlueprintPure, Category = "Camera")
	FString GetCurrentCameraName() const;

private:
	// ===== Camera Data =====
	
	UPROPERTY()
	TArray<AActor*> AvailableCameras;
	
	int32 CurrentCameraIndex;
	
	UPROPERTY(EditAnywhere, Category = "Camera")
	float CameraBlendTime = 0.5f;
	
	// ===== Helper Methods =====
	
	void PerformCameraSwitch(AActor* NewCamera);
};
