// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKOverheadCamera.generated.h"

/**
 * Overhead camera actor
 * Provides a top-down view positioned above the main mapping camera
 * Automatically follows the parent mapping camera when attached
 */
UCLASS()
class TPCPP_API ANKOverheadCamera : public AActor
{
	GENERATED_BODY()
	
public:	
	ANKOverheadCamera();

protected:
	virtual void BeginPlay() override;

public:	
	// ===== Camera Component =====
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	class UCameraComponent* Camera;
	
	// ===== Configuration =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float HeightOffsetMeters = 50.0f;  // Height above parent camera in meters
	
	/**
	 * Get the camera component
	 */
	UFUNCTION(BlueprintPure, Category = "Camera")
	UCameraComponent* GetCameraComponent() const { return Camera; }
};
