// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INKCameraControllerInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UNKCameraControllerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for camera movement and positioning component
 * Responsible for camera positioning, rotation, and orbit calculations
 */
class TPCPP_API INKCameraControllerInterface
{
	GENERATED_BODY()

public:
	// ===== Position Control =====
	
	/**
	 * Set camera position
	 * @param Position - World position
	 */
	virtual void SetPosition(const FVector& Position) = 0;
	
	/**
	 * Move camera to orbit position at specific angle
	 * @param Angle - Angle around orbit (degrees, 0 = South)
	 * @param Radius - Distance from center
	 * @param Center - Center point of orbit
	 * @param Height - Z-height of orbit
	 */
	virtual void MoveToOrbitPosition(float Angle, float Radius, const FVector& Center, float Height) = 0;
	
	// ===== Rotation Control =====
	
	/**
	 * Set camera rotation
	 * @param Rotation - World rotation
	 */
	virtual void SetRotation(const FRotator& Rotation) = 0;
	
	/**
	 * Rotate camera to look at target
	 * @param Target - World position to look at
	 */
	virtual void LookAtTarget(const FVector& Target) = 0;
	
	/**
	 * Rotate camera to specific yaw angle
	 * @param Yaw - Yaw angle in degrees
	 */
	virtual void RotateToAngle(float Yaw) = 0;
	
	// ===== Orbit Calculations =====
	
	/**
	 * Calculate orbit position at specific angle
	 * @param Angle - Angle around orbit (degrees, 0 = South)
	 * @param Radius - Distance from center
	 * @param Center - Center point of orbit
	 * @param Height - Z-height of orbit
	 * @return World position on orbit
	 */
	virtual FVector CalculateOrbitPosition(float Angle, float Radius, const FVector& Center, float Height) const = 0;
	
	/**
	 * Calculate look-at rotation
	 * @param CameraPos - Camera position
	 * @param Target - Target to look at
	 * @return Rotation to look at target
	 */
	virtual FRotator CalculateLookAtRotation(const FVector& CameraPos, const FVector& Target) const = 0;
	
	// ===== Camera Queries =====
	
	/**
	 * Get current camera position
	 */
	virtual FVector GetCameraPosition() const = 0;
	
	/**
	 * Get current camera rotation
	 */
	virtual FRotator GetCameraRotation() const = 0;
	
	/**
	 * Get camera forward vector
	 */
	virtual FVector GetCameraForward() const = 0;
	
	/**
	 * Get the CineCameraComponent (if using ACineCameraActor)
	 */
	virtual class UCineCameraComponent* GetCineCameraComponent() const = 0;
};
