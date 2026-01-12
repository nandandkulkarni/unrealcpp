// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Scanner/Interfaces/INKTerrainMapperInterface.h"
#include "Scanner/ScanDataStructures.h"
#include "NKTerrainMapperComponent.generated.h"

// Forward declarations
class INKLaserTracerInterface;
class INKCameraControllerInterface;

/**
 * Terrain mapper component
 * Handles orbit mapping mode - camera orbits around target shooting lasers
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPCPP_API UNKTerrainMapperComponent : public UActorComponent, public INKTerrainMapperInterface
{
	GENERATED_BODY()

public:
	UNKTerrainMapperComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ===== INKTerrainMapperInterface Implementation =====
	
	virtual void StartMapping(float StartAngle, float OrbitRadius, float ScanHeight) override;
	virtual void StopMapping() override;
	virtual void PauseMapping() override;
	virtual void ResumeMapping() override;
	
	virtual bool IsMapping() const override { return bIsMapping; }
	virtual bool IsPaused() const override { return bIsPaused; }
	virtual int32 GetRecordedPointCount() const override { return PointCount; }
	virtual float GetCurrentOrbitAngle() const override { return CurrentAngle; }
	virtual float GetMappingProgress() const override;
	virtual float GetElapsedTime() const override { return ElapsedTime; }
	
	virtual const TArray<FScanDataPoint>& GetScanData() const override { return ScanData; }
	virtual void ClearScanData() override;
	
	virtual bool SaveToJSON(const FString& FilePath) override;
	virtual bool LoadFromJSON(const FString& FilePath) override;
	
	virtual FOnMappingComplete& GetOnMappingCompleteEvent() override { return OnMappingComplete; }
	virtual FOnMappingProgress& GetOnMappingProgressEvent() override { return OnMappingProgress; }

	// ===== Configuration =====
	
	/** Step size in meters (distance camera moves between shots) */
	float StepSizeMeters = 10.0f;
	
	/** Orbit direction */
	EOrbitDirection OrbitDirection = EOrbitDirection::CounterClockwise;
	
	/** Shot interval in seconds */
	float ShotIntervalSeconds = 0.1f;  // 100ms
	
	/** Orbit center point */
	FVector OrbitCenter = FVector::ZeroVector;

private:
	// ===== State =====
	
	bool bIsMapping = false;
	bool bIsPaused = false;
	
	// ===== Orbit Parameters =====
	
	float OrbitRadius = 0.0f;
	float ScanHeight = 0.0f;
	float StartAngle = 0.0f;
	float CurrentAngle = 0.0f;
	
	// ===== Progress Tracking =====
	
	float TimeAccumulator = 0.0f;
	float ElapsedTime = 0.0f;
	int32 PointCount = 0;
	int32 ShotCount = 0;
	
	// ===== Data Storage =====
	
	TArray<FScanDataPoint> ScanData;
	
	// ===== Component References =====
	
	INKLaserTracerInterface* LaserTracer = nullptr;
	INKCameraControllerInterface* CameraController = nullptr;
	
	// ===== Events =====
	
	FOnMappingComplete OnMappingComplete;
	FOnMappingProgress OnMappingProgress;
	
	// ===== Internal Methods =====
	
	void PerformMappingShot();
	void CompleteMapping();
	float CalculateAngularStep() const;
};
