// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "NKObserverCamera.generated.h"

/**
 * Observer camera for watching scanning operations from above
 * Positions itself 100m above the target's highest point, centered over bounding box
 */
UCLASS()
class TPCPP_API ANKObserverCamera : public ACineCameraActor
{
	GENERATED_BODY()

public:
	ANKObserverCamera(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

public:
	// ===== Configuration =====
	
	/**
	 * Target actor to observe
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer Camera")
	AActor* TargetActor;
	
	/**
	 * Height above target's highest point (in meters)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer Camera", 
		meta = (ClampMin = "10", ClampMax = "500"))
	float HeightAboveTargetMeters = 100.0f;
	
	/**
	 * Auto-position camera on BeginPlay
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer Camera")
	bool bAutoPositionOnBeginPlay = true;
	
	/**
	 * Camera pitch angle (0 = horizontal, -90 = straight down)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Observer Camera",
		meta = (ClampMin = "-90", ClampMax = "0"))
	float CameraPitchDegrees = -90.0f;
	
	// ===== Methods =====
	
	/**
	 * Position camera above target
	 */
	UFUNCTION(BlueprintCallable, Category = "Observer Camera")
	void PositionAboveTarget();
	
	/**
	 * Get current observer height
	 */
	UFUNCTION(BlueprintPure, Category = "Observer Camera")
	float GetCurrentHeight() const;
	
	/**
	 * Calculate optimal Z height for observer camera
	 * Based on orbit radius and FOV to ensure entire orbit circle is visible
	 * Always positioned above the target's highest point
	 */
	UFUNCTION(BlueprintCallable, Category = "Observer Camera")
	float CalculateOptimalHeight() const;

private:
	/**
	 * Wide-angle FOV for Observer Camera (in degrees)
	 * 90° is a good balance between coverage and minimal distortion
	 */
	static constexpr float ObserverCameraFOV = 90.0f;
	
	/**
	 * Safety margin multiplier for height calculation
	 * 1.5 means camera will be 50% higher than minimum required
	 */
	static constexpr float HeightSafetyMargin = 1.5f;
};
