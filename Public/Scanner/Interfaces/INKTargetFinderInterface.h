// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INKTargetFinderInterface.generated.h"

/**
 * Delegate for target found event
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTargetFound, FHitResult, HitResult);

/**
 * Delegate for discovery failed event
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiscoveryFailed);

/**
 * Delegate for discovery progress event
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDiscoveryProgress, int32, ShotCount, float, CurrentAngle);

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNKTargetFinderInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for target discovery component
 * Responsible for rotating camera in place and finding first hit on target
 */
class TPCPP_API INKTargetFinderInterface
{
	GENERATED_BODY()

public:
	// ===== Discovery Control =====
	
	/**
	 * Start the discovery process
	 * @param Target - The actor to discover
	 * @param OrbitRadius - Distance from target center to camera position
	 * @param ScanHeight - Z-height at which to scan
	 */
	virtual void StartDiscovery(AActor* Target, float OrbitRadius, float ScanHeight) = 0;
	
	/**
	 * Stop the discovery process
	 */
	virtual void StopDiscovery() = 0;
	
	// ===== Discovery State Queries =====
	
	/**
	 * Check if currently discovering
	 */
	virtual bool IsDiscovering() const = 0;
	
	/**
	 * Get the number of laser shots fired during discovery
	 */
	virtual int32 GetShotCount() const = 0;
	
	/**
	 * Get the current angle being tested (0-360 degrees)
	 */
	virtual float GetCurrentAngle() const = 0;
	
	/**
	 * Get discovery progress as percentage (0-100)
	 */
	virtual float GetProgressPercent() const = 0;
	
	// ===== Discovery Results =====
	
	/**
	 * Check if target has been found
	 */
	virtual bool HasFoundTarget() const = 0;
	
	/**
	 * Get the first hit result
	 */
	virtual FHitResult GetFirstHit() const = 0;
	
	/**
	 * Get the angle at which first hit was found
	 */
	virtual float GetFirstHitAngle() const = 0;
	
	// ===== Events =====
	// Note: These would be implemented as UPROPERTY() delegates in the actual component
	// Interface just defines the contract
	
	/**
	 * Event fired when target is found
	 */
	virtual FOnTargetFound& GetOnTargetFoundEvent() = 0;
	
	/**
	 * Event fired when discovery fails (360° rotation with no hits)
	 */
	virtual FOnDiscoveryFailed& GetOnDiscoveryFailedEvent() = 0;
	
	/**
	 * Event fired on discovery progress updates
	 */
	virtual FOnDiscoveryProgress& GetOnDiscoveryProgressEvent() = 0;
};
