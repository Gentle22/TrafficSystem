// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "TrafficLightsController.generated.h"

class ATrafficLight;

USTRUCT(BlueprintType)
struct FTrafficLightGroup
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	TArray<ATrafficLight*> TrafficLights;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	float GoDuration;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	float WaitAfterStopDuration;

	FTrafficLightGroup()
		: GoDuration(10.0f), WaitAfterStopDuration(3.0f)
	{}

	FTrafficLightGroup(float InGoDuration, float InWaitAfterStopDuration)
		: GoDuration(InGoDuration), WaitAfterStopDuration(InWaitAfterStopDuration)
	{}
};

UCLASS()
class TRAFFICSYSTEM_API ATrafficLightsController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATrafficLightsController();

	TArray<FTrafficLightGroup> GetGroups() const;
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	static void SetStopStateForGroup(const FTrafficLightGroup& Group, bool bStopFlag);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USceneComponent* SceneComponent;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite)
	TArray<FTrafficLightGroup> Groups;

	void GoDurationExpired();
	void WaitDurationExpired();
	
	FTimerHandle TimerHandle;
	int32 CurrentGroupIndex;
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
