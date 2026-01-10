// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Scanner/Interfaces/INKCameraControllerInterface.h"
#include "NKCameraControllerComponent.generated.h"

/**
 * Camera controller component
 * Handles camera positioning and rotation
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPCPP_API UNKCameraControllerComponent : public UActorComponent, public INKCameraControllerInterface
{
	GENERATED_BODY()

public:
	UNKCameraControllerComponent();

public:
	// ===== INKCameraControllerInterface Implementation =====
	
	virtual void SetPosition(const FVector& Position) override;
	virtual void MoveToOrbitPosition(float Angle, float Radius, const FVector& Center, float Height) override;
	
	virtual void SetRotation(const FRotator& Rotation) override;
	virtual void LookAtTarget(const FVector& Target) override;
	virtual void RotateToAngle(float Yaw) override;
	
	virtual FVector CalculateOrbitPosition(float Angle, float Radius, const FVector& Center, float Height) const override;
	virtual FRotator CalculateLookAtRotation(const FVector& CameraPos, const FVector& Target) const override;
	
	virtual FVector GetCameraPosition() const override;
	virtual FRotator GetCameraRotation() const override;
	virtual FVector GetCameraForward() const override;
	virtual UCineCameraComponent* GetCineCameraComponent() const override;

private:
	UPROPERTY()
	UCineCameraComponent* CineCameraComponent;
};
