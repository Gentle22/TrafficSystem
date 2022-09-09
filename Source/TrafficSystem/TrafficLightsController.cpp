// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficLightsController.h"

#include "TrafficLight.h"

// Sets default values
ATrafficLightsController::ATrafficLightsController()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	CurrentGroupIndex = -1;
}

TArray<FTrafficLightGroup> ATrafficLightsController::GetGroups() const
{
	return Groups;
}

// Called when the game starts or when spawned
void ATrafficLightsController::BeginPlay()
{
	Super::BeginPlay();

	// Set all Groups to stop
	for (FTrafficLightGroup& Group : Groups)
	{
		for (ATrafficLight* TrafficLight : Group.TrafficLights)
		{
			if (TrafficLight->IsPendingKill())
				continue;

			TrafficLight->SetStop(true);
		}
	}
	
	if (Groups.Num() > 0)
	{
		WaitDurationExpired();
	}
}

void ATrafficLightsController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
}

void ATrafficLightsController::SetStopStateForGroup(const FTrafficLightGroup& Group, bool bStopFlag)
{
	for (ATrafficLight* TrafficLight : Group.TrafficLights)
	{
		if (TrafficLight->IsPendingKill())
			continue;

		TrafficLight->SetStop(bStopFlag);
	}
}

void ATrafficLightsController::GoDurationExpired()
{
	const FTrafficLightGroup& Group = Groups[CurrentGroupIndex];
	SetStopStateForGroup(Group, true);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ATrafficLightsController::WaitDurationExpired,
									Group.WaitAfterStopDuration, false);
}

void ATrafficLightsController::WaitDurationExpired()
{	
	CurrentGroupIndex += 1;
	if (CurrentGroupIndex >= Groups.Num())
		CurrentGroupIndex = 0;

	const FTrafficLightGroup& Group = Groups[CurrentGroupIndex];	
	SetStopStateForGroup(Group, false);
	GetWorldTimerManager().SetTimer(TimerHandle, this, &ATrafficLightsController::GoDurationExpired, Group.GoDuration,
									false);
}

// Called every frame
void ATrafficLightsController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

