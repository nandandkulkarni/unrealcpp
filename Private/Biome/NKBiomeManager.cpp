// Fill out your copyright notice in the Description page of Project Settings.

#include "Biome/NKBiomeManager.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "PCGVolume.h"
#include "Components/BrushComponent.h" // Added by user
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
#include "Elements/PCGTypedGetter.h"
#include "Elements/PCGDataFromActor.h"  // For UPCGGetLandscapeSettings
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
	// Find landscape
	if (bAutoDetectLandscape && !TargetLandscape)
	{
		BIOME_LOG(TEXT("🔍 Auto-detecting landscape..."));
		TargetLandscape = FindLandscapeInLevel();
	}
	else if (!bAutoDetectLandscape && TargetLandscape)
	{
		BIOME_LOG(FString::Printf(TEXT("📌 Using manually assigned landscape: %s"), *TargetLandscape->GetName()));
	}
	
	if (!TargetLandscape)
	{
		BIOME_LOG_WARNING(TEXT("⚠️ No landscape found! Proceeding with Debug Cube test anyway."));
		// return; // Allow proceeding for Debug Cube
	}
	else
	{
		BIOME_LOG(FString::Printf(TEXT("✅ Found Landcape: %s"), *TargetLandscape->GetName()));
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
			if (bAllowHISMFallback)
			{
				BIOME_LOG_WARNING(TEXT("⚠️ PCG graph creation failed! Falling back to HISM mode..."));
				GrassSpawnMode = EGrassSpawnMode::HISM;
			}
			else
			{
				BIOME_LOG_ERROR(TEXT("❌ PCG graph creation failed! HISM fallback is disabled."));
				BIOME_LOG_ERROR(TEXT("   Enable 'bAllowHISMFallback' property to use HISM as fallback."));
				return;
			}
		}
		else
		{
			BIOME_LOG(TEXT("✅ PCG graph created successfully"));
			
			// Setup PCG component
			BIOME_LOG(TEXT("⚙️ Setting up PCG component..."));
			SetupPCGComponent();
		}
	}
	
	// HISM Mode (or fallback if enabled)
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
	// Allow null landscape for Debug Cube test
	if (!GrassPreset) //Removed !TargetLandscape check
	{
		BIOME_LOG_ERROR(TEXT("❌ No Grass Preset! cannot setup."));
		return;
	}
	
	// Create PCG component with LANDSCAPE as owner (critical for bounds)
	// PCGHelpers::GetGridBounds() checks if owner is ALandscapeProxy - if yes, uses GetLandscapeBounds()
	FBox TraceBounds;
	
	// Prioritize User's "MyCube"
	AActor* TargetActor = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetName().Contains(TEXT("MyCube")) || (It->GetActorLabel().Contains(TEXT("MyCube"))))
		{
			TargetActor = *It;
			break;
		}
	}
	
	if (TargetActor)
	{
		TraceBounds = TargetActor->GetComponentsBoundingBox(true);
		BIOME_LOG(FString::Printf(TEXT("🎯 Found Target Actor: %s"), *TargetActor->GetName()));
	}
	else if (TargetLandscape)
	{
		TraceBounds = TargetLandscape->GetComponentsBoundingBox(true);
	}
	else
	{
		// Fallback: Spawn Debug Floor

	
	}
	
	if (TraceBounds.IsValid)
	{
		// Reuse HISM Logic logic to calculate points
		FBox Bounds = TraceBounds;
		
		int32 Count = 100; // Fixed count for test
		
		// Create Point Data with valid Metadata
		GeneratedPointData = NewObject<UPCGPointData>(this);
		GeneratedPointData->Metadata = NewObject<UPCGMetadata>(GeneratedPointData);
		GeneratedPointData->Metadata->Initialize(nullptr);
		
		TArray<FPCGPoint>& PcgPoints = GeneratedPointData->GetMutablePoints();
		
		// Use Target Bounds for generation (MyCube or Landscape)
		BIOME_LOG(FString::Printf(TEXT("📐 Generating %d points on Target: %s"), Count, *Bounds.ToString()));
		
		PcgPoints.Reserve(Count);
		
		for (int32 i = 0; i < Count; i++)
		{
			// Generate random point within Target Bounds XY
			float RX = FMath::RandRange(Bounds.Min.X, Bounds.Max.X);
			float RY = FMath::RandRange(Bounds.Min.Y, Bounds.Max.Y);
			
			FHitResult Hit;
			// Trace from Top of bounds down to Bottom of bounds
			if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(RX, RY, Bounds.Max.Z + 100), FVector(RX, RY, Bounds.Min.Z - 100), ECC_WorldStatic))
			{
				FPCGPoint P;
				P.Transform.SetLocation(Hit.Location);
				
				// Random Rotation/Scale
				float Scale = FMath::RandRange(MinScale, MaxScale);
				float Yaw = FMath::RandRange(0.0f, 360.0f);
				
				P.Transform.SetRotation(FQuat(FRotator(0, Yaw, 0)));
				P.Transform.SetScale3D(FVector(Scale));
				P.Density = 1.0f;
				P.Seed = i;
				
				// CRITICAL: Set usage bounds/extents for culling
				P.BoundsMin = FVector(-100.0f);
				P.BoundsMax = FVector(100.0f);
				
				PcgPoints.Add(P);
			}
		}
		BIOME_LOG(FString::Printf(TEXT("✅ Generated %d points from traces"), PcgPoints.Num()));
	}
	
	// Create PCG Component (Local owner)
	BIOME_LOG(TEXT("🔧 Creating PCGComponent on BiomeManager..."));
	PCGComponent = NewObject<UPCGComponent>(this, UPCGComponent::StaticClass(), TEXT("RuntimeGrassPCG"));
	if (PCGComponent)
	{
		PCGComponent->RegisterComponent(); // Single registration
		
		PCGComponent->SetGraph(GrassPreset);
		PCGComponent->SetIsPartitioned(false); // Global mode for now
		PCGComponent->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnLoad;
		
		PCGComponent->OnPCGGraphGeneratedExternal.AddDynamic(this, &ANKBiomeManager::OnPCGGraphGenerated);
		PCGComponent->Activate(true);
		
		BIOME_LOG(TEXT("🚀 Triggering Generation..."));
		PCGComponent->Generate();
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
	// ... (Check NewGraph) ...
	
	BIOME_LOG(TEXT("✅ UPCGGraph created"));
	
	// 2. Create Input Node (Read Points from Property)
	UPCGDataFromActorSettings* InputSettings = NewObject<UPCGDataFromActorSettings>(NewGraph);
	InputSettings->ActorSelector.ActorFilter = EPCGActorFilter::Self;
	InputSettings->ActorSelector.ActorSelection = EPCGActorSelection::ByClass; // Select by Class
	InputSettings->ActorSelector.ActorSelectionClass = ANKBiomeManager::StaticClass(); // Explicitly select this class
	// InputSettings->ActorSelector.bSelectMultiple = false; // Default
	
	InputSettings->Mode = EPCGGetDataFromActorMode::GetDataFromProperty;
	InputSettings->PropertyName = FName("GeneratedPointData");
	
	// Explicitly map "GeneratedPointData" to output pin
	// Note: Property extraction typically outputs to "Out"
	
	UPCGNode* InputNode = NewGraph->AddNode(InputSettings);
	BIOME_LOG(TEXT("✅ GetActorData Node added (Property: GeneratedPointData)"));
	
	// 3. Create Static Mesh Spawner Node
	UPCGStaticMeshSpawnerSettings* SpawnerSettings = NewObject<UPCGStaticMeshSpawnerSettings>(NewGraph);
	SpawnerSettings->SetMeshSelectorType(UPCGMeshSelectorWeighted::StaticClass());
	
	if (UPCGMeshSelectorWeighted* MeshSelector = Cast<UPCGMeshSelectorWeighted>(SpawnerSettings->MeshSelectorParameters))
	{
		FPCGMeshSelectorWeightedEntry Entry;
		Entry.Descriptor.StaticMesh = GrassMesh;
		Entry.Weight = 1;
		MeshSelector->MeshEntries.Add(Entry);
	}
	
	UPCGNode* SpawnerNode = NewGraph->AddNode(SpawnerSettings);
	BIOME_LOG(TEXT("✅ Static Mesh Spawner Node added"));
	
	// 4. Connect Input -> Spawner
	NewGraph->AddEdge(InputNode, TEXT("Out"), SpawnerNode, TEXT("In"));
	BIOME_LOG(TEXT("🔗 Connected Input(Out) -> Spawner(In)"));
	
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
	
	BIOME_LOG(FString::Printf(TEXT("✅ Grass mesh validated: %s"), *GrassMesh->GetName()));
	
	// Legacy check removed to allow "MyCube" targeting without a landscape
	if (TargetLandscape)
	{
		BIOME_LOG(FString::Printf(TEXT("✅ Found Landscape: %s"), *TargetLandscape->GetName()));
	}
	else
	{
		BIOME_LOG(TEXT("⚠️ No Landscape found. Accessing fallback targeting (MyCube)..."));
	}
	
	// Create HISM component with detailed logging
	BIOME_LOG(TEXT("🔧 Creating HISM component..."));
	HISMComponent = NewObject<UHierarchicalInstancedStaticMeshComponent>(this, UHierarchicalInstancedStaticMeshComponent::StaticClass(), TEXT("GrassHISM"));
	if (!HISMComponent)
	{
		BIOME_LOG_ERROR(TEXT("❌ CRITICAL: Failed to create HISM component with NewObject!"));
		return;
	}
	
	// Configure HISM
	HISMComponent->SetStaticMesh(GrassMesh);
	HISMComponent->RegisterComponent();
	HISMComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
	
	// --- GENERATE DATA (Trace) using MyCube Logic ---
	FBox TraceBounds;
	
	// Prioritize "MyCube"
	AActor* TargetActor = nullptr;
	for (TActorIterator<AActor> It(GetWorld()); It; ++It)
	{
		if (It->GetName().Contains(TEXT("MyCube")) || (It->GetActorLabel().Contains(TEXT("MyCube"))))
		{
			TargetActor = *It;
			break;
		}
	}
	
	if (TargetActor)
	{
		TraceBounds = TargetActor->GetComponentsBoundingBox(true);
		BIOME_LOG(FString::Printf(TEXT("🎯 HISM Targeting Actor: %s"), *TargetActor->GetName()));
	}
	else if (TargetLandscape)
	{
		TraceBounds = TargetLandscape->GetComponentsBoundingBox(true);
		BIOME_LOG(TEXT("🎯 HISM Targeting Landscape"));
	}
	else
	{
		BIOME_LOG(TEXT("❌ No Target Found for HISM."));
		return;
	}
	
	if (!TraceBounds.IsValid) return;

	int32 Count = 100; // Fixed 100 points
	BIOME_LOG(FString::Printf(TEXT("🌱 Generating 100 HISM Instances on Target...")));
	
	for (int32 i = 0; i < Count; i++)
	{
		float RX = FMath::RandRange(TraceBounds.Min.X, TraceBounds.Max.X);
		float RY = FMath::RandRange(TraceBounds.Min.Y, TraceBounds.Max.Y);
		
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, FVector(RX, RY, TraceBounds.Max.Z + 100), FVector(RX, RY, TraceBounds.Min.Z - 100), ECC_WorldStatic))
		{
			FTransform InstanceTransform;
			InstanceTransform.SetLocation(Hit.Location);
			
			// Random Rotation/Scale
			float Scale = FMath::RandRange(MinScale, MaxScale);
			float Yaw = FMath::RandRange(0.0f, 360.0f);
			
			InstanceTransform.SetRotation(FQuat(FRotator(0, Yaw, 0)));
			InstanceTransform.SetScale3D(FVector(Scale));
			
			HISMComponent->AddInstance(InstanceTransform);
		}
	}
	
	BIOME_LOG(FString::Printf(TEXT("✅ HISM Spawned %d Instances"), HISMComponent->GetInstanceCount()));
	
}

void ANKBiomeManager::OnPCGGraphGenerated(UPCGComponent* InPCGComponent)
{
	BIOME_LOG(TEXT("✅ ========================================"));
	BIOME_LOG(TEXT("✅ PCG GENERATION EVENT RECEIVED!"));
	BIOME_LOG(TEXT("   The PCG graph has finished generating content."));
	
	if (InPCGComponent)
	{
		BIOME_LOG(FString::Printf(TEXT("   Component: %s"), *InPCGComponent->GetName()));
		
		// Inspect spawned HISM components on the owner (Landscape)
		AActor* ComponentOwner = InPCGComponent->GetOwner();
		if (ComponentOwner)
		{
			int32 TotalInstances = 0;
			int32 HISMCount = 0;
			
			TArray<UHierarchicalInstancedStaticMeshComponent*> HISMComponents;
			ComponentOwner->GetComponents<UHierarchicalInstancedStaticMeshComponent>(HISMComponents);
			
			for (auto HISM : HISMComponents)
			{
				if (HISM->GetInstanceCount() > 0)
				{
					// Log details about this batch
					BIOME_LOG(FString::Printf(TEXT("   🌿 Found HISM Batch: %s | Mesh: %s | Instances: %d"), 
						*HISM->GetName(), 
						HISM->GetStaticMesh() ? *HISM->GetStaticMesh()->GetName() : TEXT("None"),
						HISM->GetInstanceCount()));
						
					TotalInstances += HISM->GetInstanceCount();
					HISMCount++;
				}
			}
			
			BIOME_LOG(FString::Printf(TEXT("📊 PCG Generation Report:")));
			BIOME_LOG(FString::Printf(TEXT("   Total Instances Spawned: %d"), TotalInstances));
			BIOME_LOG(FString::Printf(TEXT("   HISM Batches: %d"), HISMCount));
			
			if (TotalInstances == 0)
			{
				BIOME_LOG_ERROR(TEXT("❌ PCG generated 0 instances! The graph logic ran but produced no points."));
				BIOME_LOG_WARNING(TEXT("   Possible causes:"));
				BIOME_LOG_WARNING(TEXT("   1. Landscape data empty (Heightmap resolution?)"));
				BIOME_LOG_WARNING(TEXT("   2. Density too low (PointsPerSquaredMeter)"));
				BIOME_LOG_WARNING(TEXT("   3. Surface Sampler 'Unbounded' setting"));
			}
			else
			{
				BIOME_LOG(TEXT("✅ Grass instances should be visible now."));
			}
		}
	}
	
	// Check for generated actors (Partition Actors)
	// Only relevant if partitioning is enabled
	if (InPCGComponent->IsPartitioned())
	{
		CheckPCGActorsInWorld();
	}
	
	BIOME_LOG(TEXT("✅ ========================================"));
}
