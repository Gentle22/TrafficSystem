// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Lane.h"
#include "GameFramework/Actor.h"
#include "TrafficLight.generated.h"

UCLASS()
class TRAFFICSYSTEM_API ATrafficLight : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrafficLight();

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	bool bStop;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	TArray<FConnection> ConnectedWaypoints;

	UFUNCTION(BlueprintCallable)
	void SetStop(bool bStopFlag);

	void AddWaypointConnection(ALane* FromLane, int32 WaypointId);
	void RemoveWaypointConnection(ALane* FromLane, int32 WaypointId);
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USceneComponent* SceneComponent;
};
