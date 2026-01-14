// Fill out your copyright notice in the Description page of Project Settings.

#include "Biome/NKBiomeManager.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Landscape.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Scanner/Utilities/NKScannerLogger.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

// PCG Includes
#include "Elements/PCGSurfaceSampler.h"
#include "Elements/PCGStaticMeshSpawner.h"
#include "MeshSelectors/PCGMeshSelectorWeighted.h"
#include "Data/PCGPointData.h"

// Helper macros for logging (uses instance logger)
#define BIOME_LOG(Message) if (Logger) { Logger->Log(Message, TEXT("BiomeManager")); }
#define BIOME_LOG_ERROR(Message) if (Logger) { Logger->LogError(Message, TEXT("BiomeManager")); }
#define BIOME_LOG_WARNING(Message) if (Logger) { Logger->LogWarning(Message, TEXT("BiomeManager")); }

ANKBiomeManager::ANKBiomeManager()
{
	PrimaryActorTick.bCanEverTick = false;
	
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
	
	BIOME_LOG(FString::Printf(TEXT("🎮 Grass Spawn Mode: %s"), GrassSpawnMode == EGrassSpawnMode::PCG ? TEXT("PCG") : TEXT("HISM")));
	
	// Handle different spawn modes
	if (GrassSpawnMode == EGrassSpawnMode::PCG)
	{
		// PCG Mode: Create graph programmatically
		BIOME_LOG(TEXT("🔨 Creating PCG graph programmatically..."));
		GrassPreset = CreatePCGGraphProgrammatically();
		
		if (!GrassPreset)
		{
			BIOME_LOG_WARNING(TEXT("⚠️ PCG graph creation failed! Falling back to HISM mode..."));
			GrassSpawnMode = EGrassSpawnMode::HISM;
		}
		else
		{
			BIOME_LOG(TEXT("✅ PCG graph created successfully"));
			
			// Setup PCG component
			BIOME_LOG(TEXT("⚙️ Setting up PCG component..."));
			SetupPCGComponent();
		}
	}
	
	// HISM Mode (or fallback)
	if (GrassSpawnMode == EGrassSpawnMode::HISM)
	{
		BIOME_LOG(TEXT("🌿 Using HISM direct spawning mode..."));
		SpawnGrassWithHISM();
	}
	
	// Generate grass if PCG mode and enabled
	if (GrassSpawnMode == EGrassSpawnMode::PCG && bGenerateOnBeginPlay && PCGComponent)
	{
		BIOME_LOG(TEXT("🚀 Triggering grass generation..."));
		RegenerateGrass();
		
		// Schedule a check to see if PCG actors were created
		FTimerHandle CheckTimer;
		GetWorld()->GetTimerManager().SetTimer(CheckTimer, [this]()
		{
			CheckPCGActorsInWorld();
		}, 2.0f, false);  // Check after 2 seconds
	}
	else if (!bGenerateOnBeginPlay)
	{
		BIOME_LOG_WARNING(TEXT("⏸️ Auto-generation disabled (bGenerateOnBeginPlay=false)"));
	}
	
	BIOME_LOG(TEXT("========================================"));
	BIOME_LOG(TEXT("✅ BiomeManager BeginPlay Completed"));
	BIOME_LOG(FString::Printf(TEXT("   Mode: %s"), GrassSpawnMode == EGrassSpawnMode::PCG ? TEXT("PCG") : TEXT("HISM")));
	BIOME_LOG(FString::Printf(TEXT("   Landscape: %s"), TargetLandscape ? *TargetLandscape->GetName() : TEXT("None")));
	if (GrassSpawnMode == EGrassSpawnMode::HISM && HISMComponent)
	{
		BIOME_LOG(FString::Printf(TEXT("   HISM Instances: %d"), HISMComponent->GetInstanceCount()));
	}
	BIOME_LOG(TEXT("========================================"));
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
	
	// Bind to generation complete event
	// Note: In UE 5.7 this is OnPCGGraphGeneratedExternal
	PCGComponent->OnPCGGraphGeneratedExternal.AddDynamic(this, &ANKBiomeManager::OnPCGGraphGenerated);
	BIOME_LOG(TEXT("🔗 Bound to OnPCGGraphGeneratedExternal event"));
	
	// Activate component (CRITICAL - component must be active to generate)
	BIOME_LOG(TEXT("⚡ Activating PCGComponent..."));
	PCGComponent->Activate(true);
	BIOME_LOG(TEXT("✅ PCGComponent activated"));
	
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
	UE_LOG(LogTemp, Log, TEXT("     📐 Landscape Bounds: (%.1f, %.1f, %.1f) to (%.1f, %.1f, %.1f) meters"),
		LandscapeBounds.Min.X/100.0f, LandscapeBounds.Min.Y/100.0f, LandscapeBounds.Min.Z/100.0f,
		LandscapeBounds.Max.X/100.0f, LandscapeBounds.Max.Y/100.0f, LandscapeBounds.Max.Z/100.0f);
	UE_LOG(LogTemp, Log, TEXT("     🎨 Graph: %s"), *GrassPreset->GetName());
	UE_LOG(LogTemp, Log, TEXT("     ✅ Ready for generation"));
	
	// Debug: Inspect PCG Component Properties
	BIOME_LOG(TEXT("🔍 PCG Component Property Inspection:"));
	BIOME_LOG(FString::Printf(TEXT("     Owner: %s"), PCGComponent->GetOwner() ? *PCGComponent->GetOwner()->GetName() : TEXT("NULL")));
	BIOME_LOG(FString::Printf(TEXT("     Graph: %s"), PCGComponent->GetGraph() ? *PCGComponent->GetGraph()->GetName() : TEXT("NULL")));
	BIOME_LOG(FString::Printf(TEXT("     IsPartitioned: %s"), PCGComponent->IsPartitioned() ? TEXT("TRUE") : TEXT("FALSE")));
	BIOME_LOG(FString::Printf(TEXT("     GenerationTrigger: %d"), (int32)PCGComponent->GenerationTrigger));
	BIOME_LOG(FString::Printf(TEXT("     IsRegistered: %s"), PCGComponent->IsRegistered() ? TEXT("TRUE") : TEXT("FALSE")));
	BIOME_LOG(FString::Printf(TEXT("     IsActive: %s"), PCGComponent->IsActive() ? TEXT("TRUE") : TEXT("FALSE")));
	BIOME_LOG(FString::Printf(TEXT("     World: %s"), PCGComponent->GetWorld() ? *PCGComponent->GetWorld()->GetName() : TEXT("NULL")));
	
	// Check if graph has nodes
	if (PCGComponent->GetGraph())
	{
		UPCGGraph* Graph = Cast<UPCGGraph>(PCGComponent->GetGraph());
		if (Graph)
		{
			BIOME_LOG(FString::Printf(TEXT("     Graph Nodes: %d"), Graph->GetNodes().Num()));
		}
	}
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

void ANKBiomeManager::CheckPCGActorsInWorld()
{
	BIOME_LOG(TEXT("🔍 Checking for PCG actors in world..."));
	
	UWorld* World = GetWorld();
	if (!World)
	{
		BIOME_LOG_ERROR(TEXT("❌ World is NULL!"));
		return;
	}
	
	int32 PCGActorCount = 0;
	int32 PCGComponentCount = 0;
	
	// Count all PCG-related actors
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (Actor && Actor->GetName().Contains(TEXT("PCG")))
		{
			PCGActorCount++;
			BIOME_LOG(FString::Printf(TEXT("     Found PCG Actor: %s"), *Actor->GetName()));
		}
		
		// Check for PCG components
		UPCGComponent* PCGComp = Actor->FindComponentByClass<UPCGComponent>();
		if (PCGComp)
		{
			PCGComponentCount++;
			BIOME_LOG(FString::Printf(TEXT("     Found PCGComponent on: %s"), *Actor->GetName()));
		}
	}
	
	BIOME_LOG(FString::Printf(TEXT("📊 Total PCG Actors: %d"), PCGActorCount));
	BIOME_LOG(FString::Printf(TEXT("📊 Total PCG Components: %d"), PCGComponentCount));
	
	if (PCGActorCount == 0 && PCGComponentCount == 1)
	{
		BIOME_LOG_WARNING(TEXT("⚠️ No PCG partition actors created! Partitioned generation may not be working."));
		BIOME_LOG_WARNING(TEXT("💡 Try: 1) Check PCG graph has valid nodes, 2) Verify landscape material has grass output"));
	}
}

UPCGGraph* ANKBiomeManager::CreatePCGGraphProgrammatically()
{
	BIOME_LOG(TEXT("🔧 Creating PCG graph from scratch..."));
	
	if (!GrassMesh)
	{
		BIOME_LOG_ERROR(TEXT("❌ No grass mesh loaded! Cannot create PCG graph."));
		return nullptr;
	}
	
	// 1. Create a new PCG graph
	UPCGGraph* NewGraph = NewObject<UPCGGraph>(this, UPCGGraph::StaticClass(), TEXT("RuntimeGrassGraph"));
	if (!NewGraph)
	{
		BIOME_LOG_ERROR(TEXT("❌ Failed to create UPCGGraph object!"));
		return nullptr;
	}
	
	BIOME_LOG(TEXT("✅ UPCGGraph created"));
	
	// 2. Create Surface Sampler Node
	// Note: AddNode requires an instantiated Settings object
	UPCGSurfaceSamplerSettings* SamplerSettings = NewObject<UPCGSurfaceSamplerSettings>(NewGraph);
	SamplerSettings->PointsPerSquaredMeter = PointsPerSquareMeter;
	SamplerSettings->PointExtents = FVector(10.0f, 10.0f, 10.0f);
	SamplerSettings->Looseness = 1.0f;
	
	UPCGNode* SamplerNode = NewGraph->AddNode(SamplerSettings);
	if (!SamplerNode)
	{
		BIOME_LOG_ERROR(TEXT("❌ Failed to create Sampler Node!"));
		return nullptr;
	}
	BIOME_LOG(TEXT("✅ Surface Sampler Node added"));
	
	// 3. Create Static Mesh Spawner Node
	UPCGStaticMeshSpawnerSettings* SpawnerSettings = NewObject<UPCGStaticMeshSpawnerSettings>(NewGraph);
	
	// Set Mesh Selector Type (Weighted is the default/standard)
	SpawnerSettings->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass());
	
	// Configure Mesh Selector
	if (UPCGMeshSelectorWeighted* MeshSelector = Cast<UPCGMeshSelectorWeighted>(SpawnerSettings->MeshSelectorParameters))
	{
		FPCGMeshSelectorWeightedEntry Entry;
		Entry.Descriptor.StaticMesh = GrassMesh;
		Entry.Weight = 1;
		
		MeshSelector->MeshEntries.Add(Entry);
	}
	// Note: bApplyMeshConfig behaves differently in 5.7 or might be implied; skipping to rely on selector defaults
	
	UPCGNode* SpawnerNode = NewGraph->AddNode(SpawnerSettings);
	if (!SpawnerNode)
	{
		BIOME_LOG_ERROR(TEXT("❌ Failed to create Spawner Node!"));
		return nullptr;
	}
	BIOME_LOG(TEXT("✅ Static Mesh Spawner Node added"));

	// 4. Connect Input (Landscape) -> Sampler
	// Note: In UE 5.2+, Input node is implicit or retrieved via FindInputNode
	UPCGNode* InputNode = NewGraph->GetInputNode();
	if (InputNode)
	{
		// Connect Input "Landscape" pin to Sampler "Surface" pin
		NewGraph->AddEdge(InputNode, TEXT("Landscape"), SamplerNode, TEXT("Surface"));
		BIOME_LOG(TEXT("🔗 Connected Input -> Sampler"));
	}

	// 5. Connect Sampler -> Spawner
	// Connect Sampler "Out" pin to Spawner "In" pin
	NewGraph->AddEdge(SamplerNode, TEXT("Out"), SpawnerNode, TEXT("In"));
	BIOME_LOG(TEXT("🔗 Connected Sampler -> Spawner"));
	
	return NewGraph;
}

void ANKBiomeManager::SpawnGrassWithHISM()
{
	BIOME_LOG(TEXT("🌿 ========================================"));
	BIOME_LOG(TEXT("🌿 Starting HISM Grass Spawning"));
	BIOME_LOG(TEXT("🌿 ========================================"));
	
	// Validation checks with detailed logging
	if (!GrassMesh)
	{
		BIOME_LOG_ERROR(TEXT("❌ CRITICAL: No grass mesh loaded! Cannot spawn with HISM."));
		BIOME_LOG_ERROR(TEXT("   Expected mesh: SM_tdcrdbur_VarA"));
		return;
	}
	BIOME_LOG(FString::Printf(TEXT("✅ Grass mesh validated: %s"), *GrassMesh->GetName()));
	
	if (!TargetLandscape)
	{
		BIOME_LOG_ERROR(TEXT("❌ CRITICAL: No target landscape! Cannot spawn grass."));
		return;
	}
	BIOME_LOG(FString::Printf(TEXT("✅ Target landscape validated: %s"), *TargetLandscape->GetName()));
	
	// Create HISM component with detailed logging
	BIOME_LOG(TEXT("🔧 Creating HISM component..."));
	HISMComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("GrassHISM"));
	if (!HISMComponent)
	{
		BIOME_LOG_ERROR(TEXT("❌ CRITICAL: Failed to create HISM component with NewObject!"));
		return;
	}
	BIOME_LOG(TEXT("✅ HISM component object created"));
	
	// Configure HISM component
	BIOME_LOG(TEXT("⚙️ Configuring HISM component..."));
	HISMComponent->SetStaticMesh(GrassMesh);
	BIOME_LOG(FString::Printf(TEXT("   ✓ Static mesh set: %s"), *GrassMesh->GetName()));
	
	HISMComponent->SetMobility(EComponentMobility::Stationary);
	BIOME_LOG(TEXT("   ✓ Mobility: Stationary"));
	
	HISMComponent->SetCastShadow(false);
	BIOME_LOG(TEXT("   ✓ Cast shadow: Disabled (performance)"));
	
	HISMComponent->RegisterComponent();
	BIOME_LOG(TEXT("   ✓ Component registered"));
	
	HISMComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	BIOME_LOG(TEXT("   ✓ Component attached to root"));
	
	BIOME_LOG(TEXT("✅ HISM component fully configured"));
	
	// Get landscape bounds with detailed logging
	BIOME_LOG(TEXT("📐 Calculating landscape bounds..."));
	FBox LandscapeBounds = TargetLandscape->GetComponentsBoundingBox(true);
	FVector LandscapeMin = LandscapeBounds.Min;
	FVector LandscapeMax = LandscapeBounds.Max;
	
	BIOME_LOG(FString::Printf(TEXT("   Min: (%.1f, %.1f, %.1f)"), LandscapeMin.X, LandscapeMin.Y, LandscapeMin.Z));
	BIOME_LOG(FString::Printf(TEXT("   Max: (%.1f, %.1f, %.1f)"), LandscapeMax.X, LandscapeMax.Y, LandscapeMax.Z));
	
	// Calculate area and number of instances
	float LandscapeWidth = (LandscapeMax.X - LandscapeMin.X) / 100.0f;  // Convert to meters
	float LandscapeHeight = (LandscapeMax.Y - LandscapeMin.Y) / 100.0f;
	float AreaSquareMeters = LandscapeWidth * LandscapeHeight;
	int32 InstanceCount = FMath::FloorToInt(AreaSquareMeters * PointsPerSquareMeter);
	
	BIOME_LOG(FString::Printf(TEXT("📊 Landscape Statistics:")));
	BIOME_LOG(FString::Printf(TEXT("   Width: %.1f meters"), LandscapeWidth));
	BIOME_LOG(FString::Printf(TEXT("   Height: %.1f meters"), LandscapeHeight));
	BIOME_LOG(FString::Printf(TEXT("   Total Area: %.1f sq meters"), AreaSquareMeters));
	BIOME_LOG(FString::Printf(TEXT("   Density: %.2f points/sq meter"), PointsPerSquareMeter));
	BIOME_LOG(FString::Printf(TEXT("   Target Instances: %d"), InstanceCount));
	
	// Spawn instances with progress tracking
	BIOME_LOG(TEXT("🌱 Starting instance generation..."));
	TArray<FTransform> InstanceTransforms;
	InstanceTransforms.Reserve(InstanceCount);
	
	UWorld* World = GetWorld();
	int32 SuccessfulTraces = 0;
	int32 FailedTraces = 0;
	
	// Log every 10% progress
	int32 ProgressInterval = FMath::Max(1, InstanceCount / 10);
	
	for (int32 i = 0; i < InstanceCount; i++)
	{
		// Progress logging
		if (i % ProgressInterval == 0 && i > 0)
		{
			float Progress = (float)i / (float)InstanceCount * 100.0f;
			BIOME_LOG(FString::Printf(TEXT("   Progress: %.0f%% (%d/%d instances)"), Progress, i, InstanceCount));
		}
		
		// Random position on landscape
		float RandomX = FMath::RandRange(LandscapeMin.X, LandscapeMax.X);
		float RandomY = FMath::RandRange(LandscapeMin.Y, LandscapeMax.Y);
		
		// Line trace to get terrain height and normal
		FVector TraceStart(RandomX, RandomY, LandscapeMax.Z + 1000.0f);
		FVector TraceEnd(RandomX, RandomY, LandscapeMin.Z - 1000.0f);
		
		FHitResult HitResult;
		if (World->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic))
		{
			SuccessfulTraces++;
			
			// Random scale
			float RandomScale = FMath::RandRange(MinScale, MaxScale);
			
			// Random rotation around Z axis
			float RandomYaw = FMath::RandRange(0.0f, 360.0f);
			
			FTransform InstanceTransform;
			InstanceTransform.SetLocation(HitResult.Location);
			InstanceTransform.SetRotation(FQuat(FRotator(0.0f, RandomYaw, 0.0f)));
			InstanceTransform.SetScale3D(FVector(RandomScale));
			
			InstanceTransforms.Add(InstanceTransform);
		}
		else
		{
			FailedTraces++;
		}
	}
	
	BIOME_LOG(TEXT("📊 Line Trace Results:"));
	BIOME_LOG(FString::Printf(TEXT("   ✅ Successful: %d (%.1f%%)"), SuccessfulTraces, (float)SuccessfulTraces / (float)InstanceCount * 100.0f));
	BIOME_LOG(FString::Printf(TEXT("   ❌ Failed: %d (%.1f%%)"), FailedTraces, (float)FailedTraces / (float)InstanceCount * 100.0f));
	
	// Add all instances at once
	BIOME_LOG(TEXT("🔨 Adding instances to HISM component..."));
	HISMComponent->AddInstances(InstanceTransforms, false);
	
	BIOME_LOG(TEXT("✅ ========================================"));
	BIOME_LOG(FString::Printf(TEXT("✅ HISM SPAWNING COMPLETE!")));
	BIOME_LOG(FString::Printf(TEXT("   Total Instances Added: %d"), InstanceTransforms.Num()));
	BIOME_LOG(FString::Printf(TEXT("   Scale Range: %.2f - %.2f"), MinScale, MaxScale));
	BIOME_LOG(FString::Printf(TEXT("   Coverage: %.1f%% of target"), (float)InstanceTransforms.Num() / (float)InstanceCount * 100.0f));
	BIOME_LOG(TEXT("✅ ========================================"));
}

void ANKBiomeManager::OnPCGGraphGenerated(UPCGComponent* InPCGComponent)
{
	BIOME_LOG(TEXT("✅ ========================================"));
	BIOME_LOG(TEXT("✅ PCG GENERATION EVENT RECEIVED!"));
	BIOME_LOG(TEXT("   The PCG graph has finished generating content."));
	
	if (InPCGComponent)
	{
		BIOME_LOG(FString::Printf(TEXT("   Component: %s"), *InPCGComponent->GetName()));
	}
	
	// Check for generated actors
	CheckPCGActorsInWorld();
	
	BIOME_LOG(TEXT("✅ ========================================"));
}
