// Fill out your copyright notice in the Description page of Project Settings.

#include "Biome/NKBiomeManager.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Landscape.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Scanner/Utilities/NKScannerLogger.h"

// Helper macros for logging (uses instance logger)
#define BIOME_LOG(Message) if (Logger) { Logger->Log(Message, TEXT("BiomeManager")); }
#define BIOME_LOG_ERROR(Message) if (Logger) { Logger->LogError(Message, TEXT("BiomeManager")); }
#define BIOME_LOG_WARNING(Message) if (Logger) { Logger->LogWarning(Message, TEXT("BiomeManager")); }

ANKBiomeManager::ANKBiomeManager()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// Load the PCG grass preset automatically
	static ConstructorHelpers::FObjectFinder<UPCGGraphInterface> GrassPresetFinder(
		TEXT("/Game/PCG/PCG_GrassPresetBased.PCG_GrassPresetBased")
	);
	
	if (GrassPresetFinder.Succeeded())
	{
		GrassPreset = GrassPresetFinder.Object;
		UE_LOG(LogTemp, Log, TEXT("BiomeManager: ✅ Loaded PCG_GrassPresetBased"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BiomeManager: ❌ Failed to load PCG_GrassPresetBased"));
	}
	
	// Load the Megascans wheat grass mesh
	static ConstructorHelpers::FObjectFinder<UStaticMesh> GrassMeshFinder(
		TEXT("/Game/Fab/Megascans/Plants/Wheat_Grass_tdcrdbur/Medium/tdcrdbur_tier_2/StaticMeshes/SM_tdcrdbur_VarA")
	);
	
	if (GrassMeshFinder.Succeeded())
	{
		GrassMesh = GrassMeshFinder.Object;
		UE_LOG(LogTemp, Log, TEXT("BiomeManager: ✅ Loaded grass mesh: %s"), *GrassMesh->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("BiomeManager: ❌ Failed to load grass mesh"));
	}
}

void ANKBiomeManager::BeginPlay()
{
	Super::BeginPlay();
	
	// Create dedicated logger instance for BiomeManager
	Logger = NewObject<UNKScannerLogger>(this, UNKScannerLogger::StaticClass());
	if (Logger)
	{
		Logger->bLogToFile = true;
		Logger->bIncludeTimestamp = true;
		Logger->bIncludeCategory = true;
		Logger->LogFilePath = TEXT("NKBiomeManagerLog.log");  // Separate file
		BIOME_LOG(TEXT("🌱 BiomeManager initialized"));
	}
	
	BIOME_LOG(TEXT("BeginPlay started"));
	
	// Enable runtime grass maps (critical for PCG grass to work at runtime)
	EnableRuntimeGrassMaps();
	
	// Find or use assigned landscape
	if (bAutoDetectLandscape)
	{
		BIOME_LOG(TEXT("🔍 Auto-detecting landscape..."));
		TargetLandscape = FindLandscapeInLevel();
	}
	else
	{
		BIOME_LOG(FString::Printf(TEXT("📌 Using manually assigned landscape: %s"), TargetLandscape ? *TargetLandscape->GetName() : TEXT("NULL")));
	}
	
	if (!TargetLandscape)
	{
		BIOME_LOG_ERROR(TEXT("❌ No landscape found! Cannot generate grass. Aborting."));
		return;
	}
	else
	{
		BIOME_LOG(FString::Printf(TEXT("✅ Landscape found: %s"), *TargetLandscape->GetName()));
	}
	
	if (!GrassPreset)
	{
		BIOME_LOG_ERROR(TEXT("❌ No GrassPreset loaded! Check constructor loading."));
		return;
	}
	else
	{
		BIOME_LOG(FString::Printf(TEXT("✅ GrassPreset ready: %s"), *GrassPreset->GetName()));
	}
	
	// Setup PCG component
	BIOME_LOG(TEXT("⚙️ Setting up PCG component..."));
	SetupPCGComponent();
	
	// Generate grass if enabled
	if (bGenerateOnBeginPlay && PCGComponent)
	{
		BIOME_LOG(TEXT("🚀 Triggering grass generation..."));
		RegenerateGrass();
	}
	else if (!bGenerateOnBeginPlay)
	{
		BIOME_LOG_WARNING(TEXT("⏸️ Auto-generation disabled (bGenerateOnBeginPlay=false)"));
	}
	
	BIOME_LOG(TEXT("✅ BeginPlay completed"));
}

void ANKBiomeManager::RegenerateGrass()
{
	if (!PCGComponent)
	{
		BIOME_LOG_ERROR(TEXT("❌ PCGComponent not created! Cannot regenerate."));
		return;
	}
	
	BIOME_LOG(TEXT("🔄 Calling PCGComponent->Generate()..."));
	PCGComponent->Generate();
	BIOME_LOG(TEXT("✅ Generate() call completed"));
}

ALandscape* ANKBiomeManager::FindLandscapeInLevel()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	// Iterate through all actors to find a landscape
	for (TActorIterator<ALandscape> It(World); It; ++It)
	{
		ALandscape* Landscape = *It;
		if (Landscape)
		{
			FBox Bounds = Landscape->GetComponentsBoundingBox(true);
			BIOME_LOG(FString::Printf(TEXT("🗺️ Found landscape '%s' | Bounds: (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f)"),
				*Landscape->GetName(),
				Bounds.Min.X/100, Bounds.Min.Y/100, Bounds.Min.Z/100,
				Bounds.Max.X/100, Bounds.Max.Y/100, Bounds.Max.Z/100));
			return Landscape;
		}
	}
	
	BIOME_LOG_WARNING(TEXT("⚠️ No landscape actors found in level!"));
	return nullptr;
}

void ANKBiomeManager::SetupPCGComponent()
{
	if (!TargetLandscape || !GrassPreset)
	{
		return;
	}
	
	// Create PCG component
	PCGComponent = NewObject<UPCGComponent>(this, UPCGComponent::StaticClass(), TEXT("RuntimeGrassPCG"));
	if (!PCGComponent)
	{
		BIOME_LOG_ERROR(TEXT("❌ Failed to create PCGComponent with NewObject!"));
		return;
	}
	BIOME_LOG(TEXT("✅ PCGComponent created successfully"));
	
	// Register component
	BIOME_LOG(TEXT("📝 Registering PCGComponent..."));
	PCGComponent->RegisterComponent();
	BIOME_LOG(TEXT("✅ PCGComponent registered"));
	
	// Set the grass preset graph
	BIOME_LOG(FString::Printf(TEXT("🎨 Setting graph to: %s"), *GrassPreset->GetName()));
	PCGComponent->SetGraph(GrassPreset);
	
	// Configure for runtime generation
	BIOME_LOG(TEXT("⚙️ Configuring: GenerationTrigger=GenerateAtRuntime"));
	PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateAtRuntime;
	
	// Enable partitioning for large landscapes (critical for performance)
	BIOME_LOG(TEXT("🗺️ Enabling partitioned generation..."));
	PCGComponent->SetIsPartitioned(true);
	BIOME_LOG(TEXT("✅ Partitioning enabled"));
	
	// Get landscape bounds
	FBox LandscapeBounds = TargetLandscape->GetComponentsBoundingBox(true);
	
	BIOME_LOG(TEXT("✅ PCG Component fully configured:"));
	BIOME_LOG(FString::Printf(TEXT("     📐 Landscape Bounds: (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f) meters"),
		LandscapeBounds.Min.X/100.0f, LandscapeBounds.Min.Y/100.0f, LandscapeBounds.Min.Z/100.0f,
		LandscapeBounds.Max.X/100.0f, LandscapeBounds.Max.Y/100.0f, LandscapeBounds.Max.Z/100.0f));
	BIOME_LOG(FString::Printf(TEXT("     🎨 Graph: %s"), *GrassPreset->GetName()));
	BIOME_LOG(TEXT("     ✅ Ready for generation"));
}

void ANKBiomeManager::EnableRuntimeGrassMaps()
{
	// Execute console command to enable runtime grass map resources
	// This is CRITICAL for the grass to appear at runtime
	BIOME_LOG(TEXT("🎮 Executing console command: grassmap.alwaysbuildruntimegenerationresources 1"));
	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("grassmap.alwaysbuildruntimegenerationresources 1"));
		BIOME_LOG(TEXT("✅ Runtime grass map resources enabled"));
	}
	else
	{
		BIOME_LOG_ERROR(TEXT("❌ GEngine is NULL! Cannot execute console command!"));
	}
}
