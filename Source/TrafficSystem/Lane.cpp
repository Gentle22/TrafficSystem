// Fill out your copyright notice in the Description page of Project Settings.


#include "Lane.h"

#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

// Sets default values
ALane::ALane()
{
	SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(SceneComponent);

	#if WITH_EDITOR
		// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
		PrimaryActorTick.bCanEverTick = true;
		DrawDebugEnabled = false;
	#endif
}

bool ALane::HasWaypointAt(const int32 Index) const
{
	return Index < Waypoints.Num();
}

const TArray<FWaypoint>& ALane::GetWaypoints() const
{
	return Waypoints;
}

int32 ALane::CalculateNextWaypointId() const
{
	int32 NextId = 0;
	for (const auto& IdAndIndex : MapWaypointIdToIndex)
	{
		if (IdAndIndex.Key > NextId)
			NextId = IdAndIndex.Key;
	}

	return (NextId + 1);
}

const FWaypoint& ALane::GetWaypointByIndex(const int32 Index) const
{
	check(Waypoints.IsValidIndex(Index));

	return Waypoints[Index];
}

const FWaypoint& ALane::GetWaypointById(const int32 WaypointId) const
{
	check(MapWaypointIdToIndex.Contains(WaypointId));

	return GetWaypointByIndex(MapWaypointIdToIndex[WaypointId]);
}

int32 ALane::GetWaypointIndex(const int32 Id) const
{
	check(MapWaypointIdToIndex.Contains(Id));
	
	return MapWaypointIdToIndex[Id];
}

int32 ALane::GetWaypointId(const int32 Index) const
{
	check(Index < Waypoints.Num());
	
	return Waypoints[Index].Id;
}

FVector ALane::GetWaypointDirection(int32 WaypointIndex) const
{
	if (GetWaypoints().Num() == 1 || !HasWaypointAt(WaypointIndex))
		return FVector::ForwardVector;

	// Last Waypoint get the same direction as previous
	if (WaypointIndex == GetWaypoints().Num() - 1)
		return GetWaypointDirection(WaypointIndex - 1);

	return (GetWaypointByIndex(WaypointIndex + 1).Location - GetWaypointByIndex(WaypointIndex).Location).GetSafeNormal();
}

void ALane::SetStop(const int32 Index, const bool bStopFlag)
{
	check(Waypoints.IsValidIndex(Index));

	Waypoints[Index].Stop = bStopFlag;
}

void ALane::SetWaypointLocation(int32 Index, const FVector& Location)
{
	check(Waypoints.IsValidIndex(Index));

	Waypoints[Index].Location = Location;
}


bool ALane::HasWaypointId(int32 WaypointId) const
{
	return MapWaypointIdToIndex.Contains(WaypointId) && HasWaypointAt(MapWaypointIdToIndex[WaypointId]);
}

FWaypoint ALane::CreateWaypoint(FVector Location, float Speed)
{
	FWaypoint NewWaypoint = FWaypoint(CalculateNextWaypointId(), std::move(Location), Speed);
	return NewWaypoint;
}

bool ALane::ShouldTickIfViewportsOnly() const
{
	return true;
}

#if WITH_EDITOR
void ALane::DebugDrawLane()
{
	for (int32 i = 0; i < Waypoints.Num(); i++)
	{
		DrawDebugPoint(GetWorld(), Waypoints[i].Location, 25.f, FColor::Green);

		// Draw Waypoints
		if (i > 0)
		{
			FVector StartLocation = Waypoints[i - 1].Location;
			FVector EndLocation = Waypoints[i].Location;

			FVector UnitDirectionVector = UKismetMathLibrary::GetDirectionUnitVector(StartLocation, EndLocation);
			StartLocation += 200.0f * UnitDirectionVector;
			EndLocation -= 200.0f * UnitDirectionVector;
			DrawDebugDirectionalArrow(GetWorld(), StartLocation, EndLocation, 2000.f, FColor::Green);
		}

		// Draw Outgoing Connections
		const FWaypoint& Waypoint = GetWaypointByIndex(i);
		for (const auto& Connection : Waypoint.OutConnections)
		{
			if (IsConnectionValid(Connection))
			{
				FVector StartConnection = Waypoint.Location;
				FVector EndConnection = Connection.Lane->GetWaypointById(Connection.Id).Location;

				FVector UnitDirectionVector =
					UKismetMathLibrary::GetDirectionUnitVector(StartConnection, EndConnection);
				StartConnection += 200.0f * UnitDirectionVector;
				EndConnection -= 200.0f * UnitDirectionVector;
				DrawDebugDirectionalArrow(GetWorld(), StartConnection, EndConnection, 2000.f, FColor::Orange);
			}
		}
	}
}
#endif

bool ALane::IsConnectionValid(const FConnection& Connection)
{
	return Connection.Lane.IsValid() && Connection.Lane->HasWaypointId(Connection.Id);
}

void ALane::PostActorCreated()
{
	FVector WaypointLocation(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 10.0f);
	AddWaypoint(std::move(WaypointLocation));
}

// Called every frame
void ALane::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	#if WITH_EDITOR
		if (DrawDebugEnabled)
		{
			DebugDrawLane();
		}
	#endif
}

void ALane::AddWaypoint(const FVector& Location, const float Speed)
{
	const FWaypoint Waypoint = CreateWaypoint(Location, Speed);
	Waypoints.Add(Waypoint);
	RebuildWaypointMap();
}

// Insert the given waypoint at the given index. Updates the index of all incoming connections
// for each waypoint after the insertion index.
void ALane::InsertWaypoint(const FVector& Location, int32 InsertIndex, const float Speed)
{
	const FWaypoint Waypoint = CreateWaypoint(Location, Speed);
	Waypoints.Insert(Waypoint, InsertIndex);
	RebuildWaypointMap();
}

// Removes the given waypoint at the given index. Updates the index of all incoming connections
// for each waypoint after the removed index.
void ALane::RemoveWaypoint(const int32 RemovedIndex)
{
	if (!HasWaypointAt(RemovedIndex))
		return;

	const FWaypoint& RemovedWaypoint = GetWaypointByIndex(RemovedIndex);

	// Remove all outgoing connections from the removed waypoint
	for (const FConnection& OutConnection : GetOutConnectionsAt(RemovedIndex))
	{
		ALane* Lane = OutConnection.Lane.Get();
		if (!Lane)
			continue;

		Lane->Modify();
		Lane->RemoveInConnectionAt(OutConnection.Id, FConnection(this, RemovedWaypoint.Id));
	}
	
	// Remove all incoming connections from removed waypoint
	for (const FConnection& InConnection : GetInConnectionsAt(RemovedIndex))
	{
		ALane* Lane = InConnection.Lane.Get();
		if (!Lane)
			continue;

		Lane->Modify();
		Lane->RemoveOutConnectionAt(InConnection.Id, FConnection(this, RemovedWaypoint.Id));
	}

	Waypoints.RemoveAt(RemovedIndex);
	RebuildWaypointMap();
}


void ALane::RemoveAllWaypoints()
{
	for (int32 WaypointIndex = Waypoints.Num() - 1; WaypointIndex >= 0; --WaypointIndex)
	{
		RemoveWaypoint(WaypointIndex);
	}
	RebuildWaypointMap();
}

// Connects the Waypoint at the given FromIndex to a Waypoint on the ToLane at the given ToIndex
void ALane::ConnectWaypointTo(const int32 FromIndex, ALane* ToLane, const int32 ToIndex)
{
	if (!ToLane || !HasWaypointAt(FromIndex) || !ToLane->HasWaypointAt(ToIndex))
		return;

	const int32 FromWaypointId = GetWaypointId(FromIndex);
	const int32 ToWaypointId = ToLane->GetWaypointId(ToIndex);
	
	// Add outgoing connection to ourself
	AddOutConnectionAt(FromWaypointId, FConnection(ToLane, ToWaypointId));

	// Add incoming connection to other lane
	ToLane->AddInConnectionAt(ToWaypointId, FConnection(this, FromWaypointId));
}

bool ALane::AddOutConnectionAt(const int32 WaypointId, FConnection OutConnection)
{
	if (!MapWaypointIdToIndex.Contains(WaypointId))
		return false;

	FWaypoint& Waypoint = Waypoints[MapWaypointIdToIndex[WaypointId]];
	if (Waypoint.OutConnections.Contains(OutConnection))
		return false;

	Waypoint.OutConnections.Add(std::move(OutConnection));
	return true;
}

bool ALane::AddInConnectionAt(int32 WaypointId, FConnection InConnection)
{
	if (!MapWaypointIdToIndex.Contains(WaypointId))
		return false;

	FWaypoint& Waypoint = Waypoints[MapWaypointIdToIndex[WaypointId]];
	if (Waypoint.InConnections.Contains(InConnection))
		return false;
	
	Waypoint.InConnections.Add(std::move(InConnection));
	return true;
}

// Removes connection between waypoints
void ALane::DisconnectWaypointFrom(int32 FromIndex, ALane* ToLane, int32 ToIndex)
{
	if (!ToLane || !HasWaypointAt(FromIndex) || !ToLane->HasWaypointAt(ToIndex))
		return;

	// Remove incoming connection from ToLane's waypoint
	ToLane->RemoveInConnectionAt(ToLane->GetWaypointId(ToIndex), FConnection(this, GetWaypointId(FromIndex)));

	RemoveOutConnectionAt(GetWaypointId(FromIndex), FConnection(ToLane, ToLane->GetWaypointId(ToIndex)));	
}

void ALane::RemoveOutConnectionAt(int32 WaypointId, const FConnection& OutConnection)
{
	if (!MapWaypointIdToIndex.Contains(WaypointId))
		return;

	FWaypoint& Waypoint = Waypoints[MapWaypointIdToIndex[WaypointId]];
	if (!Waypoint.OutConnections.Contains(OutConnection))
		return;
	
	Waypoint.OutConnections.Remove(OutConnection);
}

void ALane::RemoveInConnectionAt(int32 WaypointId, const FConnection& InConnection)
{
	if (!MapWaypointIdToIndex.Contains(WaypointId))
		return;

	FWaypoint& Waypoint = Waypoints[MapWaypointIdToIndex[WaypointId]];
	if (!Waypoint.InConnections.Contains(InConnection))
		return;
	
	Waypoint.InConnections.Remove(InConnection);
}


// Returns all incoming connections of the waypoint at the give index or an empty array if index is out of bounds.
TArray<FConnection> ALane::GetInConnectionsAt(const int32 WaypointIndex) const
{
	if (Waypoints.IsValidIndex(WaypointIndex))
		return Waypoints[WaypointIndex].InConnections;
	
	return {};
}

// Returns all outgoing connections of the waypoint at the give index or an empty array if index is out of bounds.
TArray<FConnection> ALane::GetOutConnectionsAt(const int32 WaypointIndex) const
{
	if (Waypoints.IsValidIndex(WaypointIndex))
		return Waypoints[WaypointIndex].OutConnections;
	
	return {};
}

void ALane::RebuildWaypointMap()
{
	MapWaypointIdToIndex.Empty(Waypoints.Num());
	for (int32 Index = 0; Index < Waypoints.Num(); ++Index)
	{
		const auto& Waypoint = Waypoints[Index];
		MapWaypointIdToIndex.Add(Waypoint.Id, Index);
	}
}

