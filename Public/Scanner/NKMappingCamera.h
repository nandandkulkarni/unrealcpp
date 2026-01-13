// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CineCameraActor.h"
#include "Scanner/ScanDataStructures.h"
#include "NKMappingCamera.generated.h"

// Forward declarations
class UNKTargetFinderComponent;
class UNKLaserTracerComponent;
class UNKCameraControllerComponent;
class UNKOrbitMapperComponent;
class UNKRecordingCameraComponent;
class ANKOverheadCamera;

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

// Camera positioning mode
UENUM(BlueprintType)
enum class ECameraPositionMode : uint8
{
	Center UMETA(DisplayName = "Center (World Origin)"),
	Relative UMETA(DisplayName = "Relative (To Target)")
};

// Discovery configuration (persisted after successful discovery for reuse in mapping)
USTRUCT(BlueprintType)
struct FDiscoveryConfiguration
{
	GENERATED_BODY()
	
	// Target info
	UPROPERTY()
	AActor* TargetActor = nullptr;
	
	UPROPERTY()
	bool bIsLandscape = false;
	
	UPROPERTY()
	FBox TargetBounds;
	
	// Working trace settings (what succeeded in discovery)
	UPROPERTY()
	TEnumAsByte<ECollisionChannel> WorkingTraceChannel = ECC_WorldStatic;
	
	UPROPERTY()
	bool bUseComplexCollision = true;
	
	UPROPERTY()
	float MaxTraceRange = 100000.0f;
	
	// Orbit parameters
	UPROPERTY()
	float OrbitRadius = 0.0f;
	
	UPROPERTY()
	FVector OrbitCenter = FVector::ZeroVector;
	
	UPROPERTY()
	float ScanHeight = 0.0f;
	
	// First hit data
	UPROPERTY()
	FVector FirstHitLocation = FVector::ZeroVector;
	
	UPROPERTY()
	float FirstHitAngle = 0.0f;
	
	UPROPERTY()
	FVector CameraPositionAtHit = FVector::ZeroVector;
	
	UPROPERTY()
	FRotator CameraRotationAtHit = FRotator::ZeroRotator;
	
	// Validation
	bool IsValid() const
	{
		return TargetActor != nullptr;  // OrbitRadius not needed - calculated during mapping
	}
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
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	// ===== Configuration =====
	
	// ===== Target Configuration =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Target")
	AActor* TargetActor;
	
	// ===== Camera Positioning =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Camera Position")
	ECameraPositionMode CameraPositionMode = ECameraPositionMode::Center;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Camera Position", 
		meta = (ClampMin = "1", EditCondition = "CameraPositionMode == ECameraPositionMode::Center", EditConditionHides))
	float CenterModeHeightMeters = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Camera Position", 
		meta = (ClampMin = "0", ClampMax = "100", EditCondition = "CameraPositionMode == ECameraPositionMode::Relative", EditConditionHides))
	float HeightPercent = 50.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Camera Position", 
		meta = (ClampMin = "1", EditCondition = "CameraPositionMode == ECameraPositionMode::Relative", EditConditionHides))
	float DistanceMeters = 100.0f;
	
	// ===== Overhead Camera =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Overhead Camera")
	bool bSpawnOverheadCamera = false;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Overhead Camera", 
		meta = (EditCondition = "bSpawnOverheadCamera", EditConditionHides))
	float OverheadCameraHeightMeters = 100.0f;
	
	// ===== Mapping Settings =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Mapping")
	EMappingMode MappingMode = EMappingMode::Orbit;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Mapping",
		meta = (ClampMin = "0.1", ClampMax = "50.0"))
	float OrbitStepSizeMeters = 10.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Mapping")
	EOrbitDirection OrbitDirection = EOrbitDirection::CounterClockwise;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Mapping",
		meta = (ClampMin = "50", ClampMax = "1000"))
	float OrbitLaserShotIntervalMs = 100.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scanner Settings|Mapping")
	FLinearColor OrbitLaserColor = FLinearColor::Blue;
	
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
	 * Start mapping phase (uses persisted discovery configuration)
	 */
	UFUNCTION(BlueprintCallable, Category = "Scanner")
	void StartMapping();
	
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
	
	// ===== Mapping Progress (Available during Mapping state) =====
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Mapping")
	int32 GetMappingShotCount() const;
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Mapping")
	float GetMappingAngle() const;
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Mapping")
	float GetMappingProgress() const;
	
	UFUNCTION(BlueprintPure, Category = "Scanner|Mapping")
	int32 GetMappingHitCount() const;
	
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
	
	// ===== Overhead Camera =====
	
	/**
	 * Get reference to the overhead camera actor (if spawned)
	 */
	UFUNCTION(BlueprintPure, Category = "Scanner|Camera")
	ANKOverheadCamera* GetOverheadCamera() const { return OverheadCameraActor; }
	
	// ===== Recording Playback =====
	
	/**
	 * Start recording playback after mapping completes
	 */
	UFUNCTION(BlueprintCallable, Category = "Scanner|Recording")
	void StartRecordingPlayback();
	
	/**
	 * Stop recording playback
	 */
	UFUNCTION(BlueprintCallable, Category = "Scanner|Recording")
	void StopRecordingPlayback();
	
	/**
	 * Get recording playback progress (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "Scanner|Recording")
	float GetRecordingProgress() const;
	
	/**
	 * Is recording currently playing?
	 */
	UFUNCTION(BlueprintPure, Category = "Scanner|Recording")
	bool IsRecordingPlaying() const;

private:
	// ===== Components =====
	
	UPROPERTY()
	UNKTargetFinderComponent* TargetFinderComponent;
	
	UPROPERTY()
	UNKLaserTracerComponent* LaserTracerComponent;
	
	UPROPERTY()
	UNKCameraControllerComponent* CameraControllerComponent;
	
	UPROPERTY()
	UNKOrbitMapperComponent* OrbitMapperComponent;
	
	UPROPERTY()
	UNKRecordingCameraComponent* RecordingCameraComponent;  // Recording playback
	
	UPROPERTY()
	ANKOverheadCamera* OverheadCameraActor;  // Spawned overhead camera
	
	// ===== State =====
	
	EMappingScannerState CurrentState;
	
	// ===== Discovery Configuration (Persisted for Mapping) =====
	
	FDiscoveryConfiguration DiscoveryConfig;
	
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
	
	UFUNCTION()
	void OnMappingComplete();
	
	UFUNCTION()
	void OnMappingFailed();

	// ===== Internal Methods =====
	
	void TransitionToState(EMappingScannerState NewState);
	bool IsTargetLandscape() const;
};
