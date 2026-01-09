// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NKAAAActor.generated.h"

UCLASS()
class TPCPP_API ANKAAAActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANKAAAActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Properties")
	float TestValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Test Properties")
	FString ActorName;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
