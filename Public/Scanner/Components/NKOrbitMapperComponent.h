// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NKOrbitMapperComponent.generated.h"

// Forward declarations
class UNKLaserTracerComponent;

/**
 * Delegate fired when mapping completes successfully
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMappingCompleteSignature);

/**
 * Delegate fired when mapping is cancelled or fails
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMappingFailedSignature);

/**
 * Component that handles async tick-based orbital mapping
 * Similar to TargetFinderComponent but for the mapping phase
 * 
 * Moves camera around orbit incrementally, shooting one laser per tick
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPCPP_API UNKOrbitMapperComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNKOrbitMapperComponent();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// ===== Configuration =====
	
	/** Angular step in degrees (how much to rotate each tick) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapping|Settings")
	float AngularStepDegrees = 5.0f;
	
	/** Delay between shots in seconds (0 = every tick) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapping|Settings")
	float ShotDelay = 0.0f;
	
	/** Whether to draw debug visualization during mapping */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mapping|Debug")
	bool bDrawDebugVisuals = true;
	
	// ===== Public API =====
	
	/**
	 * Start orbital mapping around target
	 * 
	 * @param InTargetActor - The actor to orbit around
	 * @param InOrbitCenter - Center point of orbit (XY from target center, Z from scan height)
	 * @param InOrbitRadius - Radius of orbit circle
	 * @param InScanHeight - Z height to maintain during orbit
	 * @param InStartAngle - Starting angle in degrees (usually from discovery first hit)
	 * @param InLaserTracer - Laser tracer component to use for shooting
	 */
	UFUNCTION(BlueprintCallable, Category = "Mapping")
	void StartMapping(
		AActor* InTargetActor,
		FVector InOrbitCenter,
		float InOrbitRadius,
		float InScanHeight,
		float InStartAngle,
		UNKLaserTracerComponent* InLaserTracer
	);
	
	/**
	 * Stop mapping (can be resumed or cancelled)
	 */
	UFUNCTION(BlueprintCallable, Category = "Mapping")
	void StopMapping();
	
	/**
	 * Check if currently mapping
	 */
	UFUNCTION(BlueprintPure, Category = "Mapping")
	bool IsMapping() const { return bIsMapping; }
	
	/**
	 * Get current angle around orbit (0-360)
	 */
	UFUNCTION(BlueprintPure, Category = "Mapping")
	float GetCurrentAngle() const { return CurrentAngle; }
	
	/**
	 * Get progress percentage (0-100)
	 */
	UFUNCTION(BlueprintPure, Category = "Mapping")
	float GetProgressPercent() const;
	
	/**
	 * Get total number of shots taken
	 */
	UFUNCTION(BlueprintPure, Category = "Mapping")
	int32 GetShotCount() const { return ShotCount; }
	
	/**
	 * Get number of hits recorded
	 */
	UFUNCTION(BlueprintPure, Category = "Mapping")
	int32 GetHitCount() const { return HitCount; }
	
	// ===== Events =====
	
	UPROPERTY(BlueprintAssignable, Category = "Mapping|Events")
	FOnMappingCompleteSignature OnMappingComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Mapping|Events")
	FOnMappingFailedSignature OnMappingFailed;

protected:
	virtual void BeginPlay() override;

private:
	// ===== State =====
	
	bool bIsMapping = false;
	
	UPROPERTY()
	AActor* TargetActor = nullptr;
	
	UPROPERTY()
	UNKLaserTracerComponent* LaserTracer = nullptr;
	
	FVector OrbitCenter;
	float OrbitRadius = 0.0f;
	float ScanHeight = 0.0f;
	float StartAngle = 0.0f;
	float CurrentAngle = 0.0f;
	
	int32 ShotCount = 0;
	int32 HitCount = 0;
	
	float TimeSinceLastShot = 0.0f;
	
	// ===== Helper Methods =====
	
	/**
	 * Calculate position on orbit at given angle
	 */
	FVector CalculateOrbitPosition(float Angle) const;
	
	/**
	 * Calculate rotation to look at target center
	 */
	FRotator CalculateLookAtRotation(const FVector& FromPosition, const FVector& ToPosition) const;
	
	/**
	 * Perform one mapping step (move + shoot)
	 */
	void PerformMappingStep(float DeltaTime);
	
	/**
	 * Complete the mapping process
	 */
	void CompletMapping();
};
