// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INKTerrainMapperInterface.generated.h"

/**
 * Delegate for mapping complete event
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMappingComplete, int32, PointCount);

/**
 * Delegate for mapping progress event
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMappingProgress, float, ProgressPercent, int32, CurrentPointCount);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNKTerrainMapperInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for terrain mapping component
 * Responsible for orbiting around target and recording scan data
 */
class TPCPP_API INKTerrainMapperInterface
{
	GENERATED_BODY()

public:
	// ===== Mapping Control =====
	
	/**
	 * Start terrain mapping
	 * @param StartAngle - Angle to start mapping from (degrees)
	 * @param OrbitRadius - Distance from target center
	 * @param ScanHeight - Z-height at which to scan
	 */
	virtual void StartMapping(float StartAngle, float OrbitRadius, float ScanHeight) = 0;
	
	/**
	 * Stop terrain mapping
	 */
	virtual void StopMapping() = 0;
	
	/**
	 * Pause terrain mapping
	 */
	virtual void PauseMapping() = 0;
	
	/**
	 * Resume terrain mapping
	 */
	virtual void ResumeMapping() = 0;
	
	// ===== Mapping State Queries =====
	
	/**
	 * Check if currently mapping
	 */
	virtual bool IsMapping() const = 0;
	
	/**
	 * Check if mapping is paused
	 */
	virtual bool IsPaused() const = 0;
	
	/**
	 * Get number of data points recorded
	 */
	virtual int32 GetRecordedPointCount() const = 0;
	
	/**
	 * Get current orbit angle (degrees)
	 */
	virtual float GetCurrentOrbitAngle() const = 0;
	
	/**
	 * Get mapping progress as percentage (0-100)
	 */
	virtual float GetMappingProgress() const = 0;
	
	/**
	 * Get elapsed time since mapping started (seconds)
	 */
	virtual float GetElapsedTime() const = 0;
	
	// ===== Data Access =====
	
	/**
	 * Get all recorded scan data
	 */
	virtual const TArray<struct FScanDataPoint>& GetScanData() const = 0;
	
	/**
	 * Clear all recorded scan data
	 */
	virtual void ClearScanData() = 0;
	
	// ===== Data Persistence =====
	
	/**
	 * Save scan data to JSON file
	 * @param FilePath - Full path to output JSON file
	 * @return true if save succeeded
	 */
	virtual bool SaveToJSON(const FString& FilePath) = 0;
	
	/**
	 * Load scan data from JSON file
	 * @param FilePath - Full path to input JSON file
	 * @return true if load succeeded
	 */
	virtual bool LoadFromJSON(const FString& FilePath) = 0;
	
	// ===== Events =====
	
	/**
	 * Event fired when mapping completes (full 360° orbit)
	 */
	virtual FOnMappingComplete& GetOnMappingCompleteEvent() = 0;
	
	/**
	 * Event fired on mapping progress updates
	 */
	virtual FOnMappingProgress& GetOnMappingProgressEvent() = 0;
};
