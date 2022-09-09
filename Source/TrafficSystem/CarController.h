// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "CarController.generated.h"

/**
 * 
 */
UCLASS()
class TRAFFICSYSTEM_API ACarController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	bool CheckCollisions() const;

protected:
	class AWheeledVehicle* PosessedVehicle;

	UPROPERTY(VisibleAnywhere)
	TWeakObjectPtr<class ALane> CurrentLane;
	UPROPERTY(VisibleAnywhere)
	int32 CurrentWaypointIndex;
	
	ALane* FindClosestLane(float Radius);
	bool FindClosestWaypointIndex(TWeakObjectPtr<ALane>& OutLane, int32& OutWaypointIndex);
	bool HasValidWaypoint() const;

	static float CalculateTurnAngle(const FVector& VehicleLocation, const FVector& VehicleForward,
									const FVector& TargetLocation);

	void Drive() const;
};

