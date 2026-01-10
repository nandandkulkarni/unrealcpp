// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "NKMappingCamera.generated.h"

// Forward declarations
class UNKTargetFinderComponent;
class UNKLaserTracerComponent;
class UNKCameraControllerComponent;

// Scanner state
UENUM(BlueprintType)
enum class EMappingScannerState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Discovering UMETA(DisplayName = "Discovering"),
	Discovered UMETA(DisplayName = "Discovered"),
	DiscoveryFailed UMETA(DisplayName = "Discovery Failed"),
	DiscoveryCancelled UMETA(DisplayName = "Discovery Cancelled"),
	Mapping UMETA(DisplayName = "Mapping"),
	Complete UMETA(DisplayName = "Complete")
};

/**
 * Main scanner camera actor
 * Orchestrates discovery and mapping using component-based architecture
 */
UCLASS()
class TPCPP_API ANKMappingCamera : public ACineCameraActor
{
	GENERATED_BODY()

public:
	ANKMappingCamera(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ===== Configuration =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner|Target")
	AActor* TargetActor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner|Target", meta = (ClampMin = "0", ClampMax = "100"))
	float HeightPercent = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner|Target", meta = (ClampMin = "1"))
	float DistanceMeters = 100.0f;
	
	// ===== High-Level Control =====
	
	/**
	 * Start discovery phase (uses configured target and parameters)
	 */
	UFUNCTION(BlueprintCallable, Category = "Scanner")
	void StartDiscovery();
	
	/**
	 * Stop current operation
	 */
	UFUNCTION(BlueprintCallable, Category = "Scanner")
	void Stop();
	
	/**
	 * Clear discovery laser lines
	 */
	UFUNCTION(BlueprintCallable, Category = "Scanner")
	void ClearDiscoveryLines();
	
	// ===== State Queries =====
	
	UFUNCTION(BlueprintPure, Category = "Scanner|State")
	EMappingScannerState GetScannerState() const { return CurrentState; }
	
	UFUNCTION(BlueprintPure, Category = "Scanner|State")
	bool IsDiscovering() const { return CurrentState == EMappingScannerState::Discovering; }
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	int32 GetDiscoveryShotCount() const;
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	float GetDiscoveryAngle() const;
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	float GetDiscoveryProgress() const;
	
	// ===== First Hit Data (Available after Discovered state) =====
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	bool HasFirstHit() const { return bHasFirstHit; }
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	FHitResult GetFirstHitResult() const { return FirstHitResult; }
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	float GetFirstHitAngle() const { return FirstHitAngle; }
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	FVector GetFirstHitCameraPosition() const { return FirstHitCameraPosition; }
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Discovery")
	FRotator GetFirstHitCameraRotation() const { return FirstHitCameraRotation; }

private:
	// ===== Components =====
	
	UPROPERTY()
	UNKTargetFinderComponent* TargetFinderComponent;
	
	UPROPERTY()
	UNKLaserTracerComponent* LaserTracerComponent;
	
	UPROPERTY()
	UNKCameraControllerComponent* CameraControllerComponent;
	
	// ===== State =====
	
	EMappingScannerState CurrentState;
	
	// ===== First Hit Data =====
	
	bool bHasFirstHit;
	FHitResult FirstHitResult;
	float FirstHitAngle;
	FVector FirstHitCameraPosition;
	FRotator FirstHitCameraRotation;
	
	// ===== Event Handlers =====
	
	UFUNCTION()
	void OnTargetFound(FHitResult HitResult);
	
	UFUNCTION()
	void OnDiscoveryFailed();
	
	// ===== Internal Methods =====
	
	void TransitionToState(EMappingScannerState NewState);
};
