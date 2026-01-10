// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INKDebugVisualizerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNKDebugVisualizerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for debug visualization component
 * Responsible for drawing non-laser debug visuals (orbit paths, bounding boxes, scan points)
 */
class TPCPP_API INKDebugVisualizerInterface
{
	GENERATED_BODY()

public:
	// ===== Path Visualization =====
	
	/**
	 * Draw orbit path circle
	 * @param Center - Center of orbit
	 * @param Radius - Orbit radius
	 * @param Height - Z-height of orbit
	 */
	virtual void DrawOrbitPath(const FVector& Center, float Radius, float Height) = 0;
	
	/**
	 * Draw target bounding box
	 * @param Bounds - Bounding box to draw
	 */
	virtual void DrawBoundingBox(const FBox& Bounds) = 0;
	
	// ===== Scan Point Visualization =====
	
	/**
	 * Draw scan point sphere
	 * @param Location - World location
	 * @param bHit - Whether this was a hit (affects color)
	 */
	virtual void DrawScanPoint(const FVector& Location, bool bHit) = 0;
	
	/**
	 * Draw scan line from camera to point
	 * @param Start - Camera position
	 * @param End - Scan point
	 */
	virtual void DrawScanLine(const FVector& Start, const FVector& End) = 0;
	
	// ===== Clear Functions =====
	
	/**
	 * Clear all debug visualizations
	 */
	virtual void ClearAllDebugVisuals() = 0;
	
	/**
	 * Clear only scan point visualizations
	 */
	virtual void ClearScanPoints() = 0;
	
	// ===== Settings =====
	
	/**
	 * Set lifetime for debug visuals
	 * @param Lifetime - Lifetime in seconds (-1 for infinite)
	 */
	virtual void SetVisualsLifetime(float Lifetime) = 0;
	
	/**
	 * Enable/disable scan point spheres
	 */
	virtual void SetShowScanPoints(bool bShow) = 0;
	
	/**
	 * Enable/disable scan lines
	 */
	virtual void SetShowScanLines(bool bShow) = 0;
	
	/**
	 * Enable/disable orbit path
	 */
	virtual void SetShowOrbitPath(bool bShow) = 0;
	
	/**
	 * Enable/disable bounding box
	 */
	virtual void SetShowBoundingBox(bool bShow) = 0;
	
	/**
	 * Set scan point color
	 */
	virtual void SetScanPointColor(FColor Color) = 0;
	
	/**
	 * Set scan line color
	 */
	virtual void SetScanLineColor(FColor Color) = 0;
};
