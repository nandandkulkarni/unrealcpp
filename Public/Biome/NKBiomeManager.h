// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKBiomeManager.generated.h"

// Forward declarations
class UPCGComponent;
class UPCGGraphInterface;
class ALandscape;
class UNKScannerLogger;

/**
 * Biome Manager Actor
 * Automates PCG grass spawning on landscapes using the Runtime Grass GPU preset
 */
UCLASS()
class TPCPP_API ANKBiomeManager : public AActor
{
	GENERATED_BODY()
	
public:	
	ANKBiomeManager();

protected:
	virtual void BeginPlay() override;

public:	
	// ===== Configuration =====
	
	/**
	 * PCG Graph to use for grass generation
	 * Assign the "Runtime Grass GPU" preset here
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass")
	UPCGGraphInterface* GrassPreset;
	
	/**
	 * Auto-detect landscape in level
	 * If false, manually assign TargetLandscape
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass")
	bool bAutoDetectLandscape = true;
	
	/**
	 * Target landscape for grass generation
	 * Only used if bAutoDetectLandscape is false
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass",
		meta = (EditCondition = "!bAutoDetectLandscape", EditConditionHides))
	ALandscape* TargetLandscape;
	
	/**
	 * Generate grass automatically on BeginPlay
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass")
	bool bGenerateOnBeginPlay = true;
	
	// ===== Public API =====
	
	/**
	 * Regenerate grass (useful for runtime updates)
	 */
	UFUNCTION(BlueprintCallable, Category = "PCG Grass")
	void RegenerateGrass();
	
	/**
	 * Get the PCG component (created at runtime)
	 */
	UFUNCTION(BlueprintPure, Category = "PCG Grass")
	UPCGComponent* GetPCGComponent() const { return PCGComponent; }

private:
	// ===== Components =====
	
	/**
	 * PCG Component (created at runtime)
	 */
	UPROPERTY()
	UPCGComponent* PCGComponent;
	
	/**
	 * Grass mesh to spawn (loaded automatically)
	 */
	UPROPERTY()
	UStaticMesh* GrassMesh;
	
	/**
	 * Logger instance for BiomeManager (writes to separate file)
	 */
	UPROPERTY()
	UNKScannerLogger* Logger;
	
	// ===== Helper Methods =====
	
	/**
	 * Find landscape actor in level
	 */
	ALandscape* FindLandscapeInLevel();
	
	/**
	 * Setup PCG component with grass preset and bounds
	 */
	void SetupPCGComponent();
	
	/**
	 * Execute console command for runtime grass maps
	 */
	void EnableRuntimeGrassMaps();
};
