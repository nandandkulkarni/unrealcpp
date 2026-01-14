// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKBiomeManager.generated.h"

// Forward declarations
class UPCGComponent;
class UPCGGraphInterface;
class UPCGGraph;
class ALandscape;
class UNKScannerLogger;
class UHierarchicalInstancedStaticMeshComponent;

/**
 * Grass spawning mode
 */
UENUM(BlueprintType)
enum class EGrassSpawnMode : uint8
{
	/** Use PCG framework with programmatic graph creation */
	PCG UMETA(DisplayName = "PCG (Procedural Content Generation)"),
	
	/** Direct spawning using Hierarchical Instanced Static Mesh */
	HISM UMETA(DisplayName = "HISM (Direct Spawning)")
};

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
	
	/**
	 * Grass spawning mode
	 * PCG: Uses Procedural Content Generation framework (default)
	 * HISM: Direct spawning using Hierarchical Instanced Static Mesh
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass")
	EGrassSpawnMode GrassSpawnMode = EGrassSpawnMode::PCG;
	
	/**
	 * Grass density (points per square meter)
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass", meta = (ClampMin = "0.01", ClampMax = "10.0"))
	float PointsPerSquareMeter = 0.5f;
	
	/**
	 * Minimum grass scale
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MinScale = 0.8f;
	
	/**
	 * Maximum grass scale
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PCG Grass", meta = (ClampMin = "0.1", ClampMax = "5.0"))
	float MaxScale = 1.2f;	
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
	
	/**
	 * Check if PCG actors were created in the world
	 */
	void CheckPCGActorsInWorld();
	
	/**
	 * Create PCG graph programmatically without preset
	 */
	UPCGGraph* CreatePCGGraphProgrammatically();
	
	/**
	 * Spawn grass using HISM (fallback mode)
	 */
	void SpawnGrassWithHISM();
	
	/**
	 * HISM component for direct spawning (created if using HISM mode)
	 */
	UPROPERTY()
	UHierarchicalInstancedStaticMeshComponent* HISMComponent;
};
