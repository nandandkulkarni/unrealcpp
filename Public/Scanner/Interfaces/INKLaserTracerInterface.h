// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INKLaserTracerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNKLaserTracerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for laser tracing and visualization component
 * Responsible for performing laser traces and drawing laser visualizations
 */
class TPCPP_API INKLaserTracerInterface
{
	GENERATED_BODY()

public:
	// ===== Laser Tracing =====
	
	/**
	 * Perform a laser trace from camera forward
	 * @param OutHit - Hit result if trace hits something
	 * @return true if trace hit something
	 */
	virtual bool PerformTrace(FHitResult& OutHit) = 0;
	
	/**
	 * Perform a laser trace at a specific angle
	 * @param Angle - Angle to trace at (degrees)
	 * @param OutHit - Hit result if trace hits something
	 * @return true if trace hit something
	 */
	virtual bool PerformTraceAtAngle(float Angle, FHitResult& OutHit) = 0;
	
	// ===== Laser Properties =====
	
	/**
	 * Set maximum laser range
	 * @param Range - Max range in centimeters
	 */
	virtual void SetMaxRange(float Range) = 0;
	
	/**
	 * Get maximum laser range
	 * @return Max range in centimeters
	 */
	virtual float GetMaxRange() const = 0;
	
	/**
	 * Set trace collision channel
	 * @param Channel - Collision channel to use for traces
	 */
	virtual void SetTraceChannel(ECollisionChannel Channel) = 0;
	
	/**
	 * Get trace collision channel
	 */
	virtual ECollisionChannel GetTraceChannel() const = 0;
	
	// ===== Hit Information =====
	
	/**
	 * Check if last shot hit something
	 */
	virtual bool GetLastShotHit() const = 0;
	
	/**
	 * Get actor hit by last shot
	 */
	virtual AActor* GetLastHitActor() const = 0;
	
	/**
	 * Get location hit by last shot
	 */
	virtual FVector GetLastHitLocation() const = 0;
	
	/**
	 * Get distance of last hit
	 * @return Distance in centimeters
	 */
	virtual float GetLastHitDistance() const = 0;
	
	// ===== Laser Visualization =====
	
	/**
	 * Draw a laser beam
	 * @param Start - Start position
	 * @param End - End position
	 * @param bHit - Whether laser hit something
	 */
	virtual void DrawLaserBeam(const FVector& Start, const FVector& End, bool bHit) = 0;
	
	/**
	 * Draw a discovery shot (persistent colored line)
	 * @param Start - Start position
	 * @param End - End position
	 * @param bHit - Whether shot hit something (green if true, red if false)
	 */
	virtual void DrawDiscoveryShot(const FVector& Start, const FVector& End, bool bHit) = 0;
	
	/**
	 * Clear all laser visualizations
	 */
	virtual void ClearLaserVisuals() = 0;
	
	// ===== Visual Settings =====
	
	/**
	 * Set laser beam color
	 */
	virtual void SetLaserColor(FColor Color) = 0;
	
	/**
	 * Set laser beam thickness
	 */
	virtual void SetLaserThickness(float Thickness) = 0;
	
	/**
	 * Enable/disable laser visualization
	 */
	virtual void SetShowLaser(bool bShow) = 0;
};
