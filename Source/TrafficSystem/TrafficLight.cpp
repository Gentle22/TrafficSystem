// Fill out your copyright notice in the Description page of Project Settings.


#include "TrafficLight.h"

// Sets default values
ATrafficLight::ATrafficLight()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	SetStop(true);
}

void ATrafficLight::SetStop(const bool bStopFlag)
{
	bStop = bStopFlag;
	for (const auto& ConnectedWaypoint : ConnectedWaypoints)
	{
		if (ConnectedWaypoint.Lane.IsValid())
		{
			if (ConnectedWaypoint.Lane->HasWaypointId(ConnectedWaypoint.Id))
			{
				const int32 WaypointIndex = ConnectedWaypoint.Lane->GetWaypointIndex(ConnectedWaypoint.Id);
				ConnectedWaypoint.Lane->SetStop(WaypointIndex, bStopFlag);
			}
		}
	}
}

void ATrafficLight::AddWaypointConnection(ALane* FromLane, int32 WaypointId)
{
	const FConnection Connection(FromLane, WaypointId);
	if (ConnectedWaypoints.Contains(Connection))
		return;

	ConnectedWaypoints.Add(Connection);
}

void ATrafficLight::RemoveWaypointConnection(ALane* FromLane, int32 WaypointId)
{
	if (!IsValid(FromLane))
		return;
	
	const FConnection Connection(FromLane, WaypointId);
	if (!ConnectedWaypoints.Contains(Connection))
		return;

	ConnectedWaypoints.Remove(Connection);
}


