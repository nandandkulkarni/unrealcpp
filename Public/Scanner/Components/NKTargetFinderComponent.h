// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Scanner/Interfaces/INKTargetFinderInterface.h"
#include "NKTargetFinderComponent.generated.h"

// Forward declarations
class INKLaserTracerInterface;
class INKCameraControllerInterface;

/**
 * Target discovery component
 * Rotates camera in place and finds first hit on target
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPCPP_API UNKTargetFinderComponent : public UActorComponent, public INKTargetFinderInterface
{
	GENERATED_BODY()

public:
	UNKTargetFinderComponent();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
	// ===== INKTargetFinderInterface Implementation =====
	
	virtual void StartDiscovery(AActor* Target, float ScanHeight) override;
	virtual void StopDiscovery() override;
	
	virtual bool IsDiscovering() const override { return bIsDiscovering; }
	virtual int32 GetShotCount() const override { return ShotCount; }
	virtual float GetCurrentAngle() const override { return CurrentAngle; }
	virtual float GetProgressPercent() const override { return (CurrentAngle / 360.0f) * 100.0f; }
	
	virtual bool HasFoundTarget() const override { return bHasFoundTarget; }
	virtual FHitResult GetFirstHit() const override { return FirstHit; }
	virtual float GetFirstHitAngle() const override { return FirstHitAngle; }
	
	virtual FOnTargetFound& GetOnTargetFoundEvent() override { return OnTargetFound; }
	virtual FOnDiscoveryFailed& GetOnDiscoveryFailedEvent() override { return OnDiscoveryFailed; }
	virtual FOnDiscoveryProgress& GetOnDiscoveryProgressEvent() override { return OnDiscoveryProgress; }
	
	// ===== Configuration =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	float AngularStepDegrees = 1.0f;  // Angle increment per shot (1° for fine detail)
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Discovery")
	float ShotInterval = 0.1f;  // Time between shots in seconds (100ms)

private:
	// ===== Internal Methods =====
	
	void PerformDiscoveryShot();
	void PositionCameraAtStart();
	void RotateCameraToAngle(float Angle);
	
	// ===== State =====
	
	bool bIsDiscovering;
	int32 ShotCount;
	float CurrentAngle;
	bool bHasFoundTarget;
	FHitResult FirstHit;
	float FirstHitAngle;
	
	// Shot timing
	float TimeAccumulator;
	
	// Discovery parameters
	UPROPERTY()
	AActor* TargetActor;
	FVector OrbitCenter;
	float ScanHeight;
	
	// ===== Events =====
	
	UPROPERTY(BlueprintAssignable, Category = "Discovery")
	FOnTargetFound OnTargetFound;
	
	UPROPERTY(BlueprintAssignable, Category = "Discovery")
	FOnDiscoveryFailed OnDiscoveryFailed;
	
	UPROPERTY(BlueprintAssignable, Category = "Discovery")
	FOnDiscoveryProgress OnDiscoveryProgress;
	
	// ===== Component References =====
	
	INKLaserTracerInterface* LaserTracer;
	INKCameraControllerInterface* CameraController;
};
