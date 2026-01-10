// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Scanner/Interfaces/INKLaserTracerInterface.h"
#include "NKLaserTracerComponent.generated.h"

/**
 * Laser tracing component
 * Performs laser traces and visualizes results
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TPCPP_API UNKLaserTracerComponent : public UActorComponent, public INKLaserTracerInterface
{
	GENERATED_BODY()

public:
	UNKLaserTracerComponent();

public:
	// ===== INKLaserTracerInterface Implementation =====
	
	virtual bool PerformTrace(FHitResult& OutHit) override;
	virtual bool PerformTraceAtAngle(float Angle, FHitResult& OutHit) override;
	
	virtual void SetMaxRange(float Range) override { MaxRange = Range; }
	virtual float GetMaxRange() const override { return MaxRange; }
	virtual void SetTraceChannel(ECollisionChannel Channel) override { TraceChannel = Channel; }
	virtual ECollisionChannel GetTraceChannel() const override { return TraceChannel; }
	
	virtual bool GetLastShotHit() const override { return bLastShotHit; }
	virtual AActor* GetLastHitActor() const override { return LastHitActor; }
	virtual FVector GetLastHitLocation() const override { return LastHitLocation; }
	virtual float GetLastHitDistance() const override { return LastHitDistance; }
	
	virtual void DrawLaserBeam(const FVector& Start, const FVector& End, bool bHit) override;
	virtual void DrawDiscoveryShot(const FVector& Start, const FVector& End, bool bHit) override;
	virtual void ClearLaserVisuals() override;
	
	virtual void SetLaserColor(FColor Color) override { LaserColor = Color; }
	virtual void SetLaserThickness(float Thickness) override { LaserThickness = Thickness; }
	virtual void SetShowLaser(bool bShow) override { bShowLaser = bShow; }
	
	// ===== Configuration =====
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Trace")
	float MaxRange = 100000.0f;  // 1000m default
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Trace")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_WorldStatic;  // Changed to WorldStatic for landscape support
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Trace")
	bool bUseComplexCollision = true;  // Use complex collision for landscapes
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Trace")
	bool bUseFallbackChannel = false;  // Try alternate channel if primary fails
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Trace", 
		meta = (EditCondition = "bUseFallbackChannel"))
	TEnumAsByte<ECollisionChannel> FallbackTraceChannel = ECC_Visibility;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Visualization")
	bool bShowLaser = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Visualization")
	FColor LaserColor = FColor::Red;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Visualization")
	float LaserThickness = 2.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Laser Visualization")
	float VisualsLifetime = -1.0f;  // Infinite by default

private:
	// Last shot state
	bool bLastShotHit;
	UPROPERTY()
	AActor* LastHitActor;
	FVector LastHitLocation;
	float LastHitDistance;
};
