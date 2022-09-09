// Fill out your copyright notice in the Description page of Project Settings.


#include "CarController.h"

#include "Lane.h"
#include "WheeledVehicle.h"
#include "WheeledVehicleMovementComponent.h"
//#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Kismet/KismetMathLibrary.h"

#include <limits>

void ACarController::OnPossess(APawn* InPawn)
{
    AAIController::OnPossess(InPawn);

    AWheeledVehicle* Vehicle = Cast<AWheeledVehicle>(InPawn);
    if (Vehicle)
    {
        PosessedVehicle = Vehicle;
    }
}

void ACarController::OnUnPossess()
{
    PosessedVehicle = nullptr;
    
    AAIController::OnUnPossess();
}

void ACarController::BeginPlay()
{
    Super::BeginPlay();
    
    // Find Lane and Waypoint
    if (!CurrentLane.IsValid())
    {
        FindClosestWaypointIndex(CurrentLane, CurrentWaypointIndex);
        if (CurrentLane.IsValid())
            UE_LOG(LogTemp, Warning, TEXT("Set Current Lane: %s -> %i"), *CurrentLane->GetName(), CurrentWaypointIndex);
    }
}

void ACarController::Tick(float DeltaTime)
{
    AAIController::Tick(DeltaTime);
    
    if (PosessedVehicle)
    {
        if (HasValidWaypoint())
        {
            Drive();
            
            // Check Waypoint distance
            const FVector2D CurrentWaypointLocation(CurrentLane->GetWaypointByIndex(CurrentWaypointIndex).Location);
            const FVector2D CurrentCarLocation(PosessedVehicle->GetActorLocation());
            const float DistanceToWaypointSquared = (CurrentWaypointLocation - CurrentCarLocation).SizeSquared();
            if (DistanceToWaypointSquared <= (50.0f * 50.0f))
            {
                const FWaypoint& CurrentWaypoint = CurrentLane->GetWaypointByIndex(CurrentWaypointIndex);
                const int32 NumOutConnections = CurrentWaypoint.OutConnections.Num();
                if (NumOutConnections > 0)
                {
                    int32 TakeConnection = 0;
                    if (NumOutConnections > 1)
                        TakeConnection = 1;

                    const auto& OutConnection = CurrentWaypoint.OutConnections[TakeConnection];
                    if (OutConnection.Lane.IsValid() && OutConnection.Lane->HasWaypointId(OutConnection.Id))
                    {
                        CurrentLane = OutConnection.Lane;
                        CurrentWaypointIndex = OutConnection.Lane->GetWaypointIndex(OutConnection.Id);
                    }
                }
                else
                {
                    if (CurrentWaypointIndex < CurrentLane->GetWaypoints().Num() - 1)
                    {
                        CurrentWaypointIndex += 1;
                    }
                    else
                    {
                        CurrentWaypointIndex = -1;
                    }
                }
            }
        }
        else
        {
            PosessedVehicle->GetVehicleMovement()->SetThrottleInput(0.0f);
            PosessedVehicle->GetVehicleMovement()->SetBrakeInput(1.0f);
            //PosessedVehicle->GetVehicleMovement()->SetFixedBrakingDistance(0.5f);
        }
    }
}

bool ACarController::CheckCollisions() const
{
    FHitResult HitResult;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(PosessedVehicle);
    FVector Start = PosessedVehicle->GetActorLocation();
    Start.Z += 50.0f;
    FVector End = Start + (PosessedVehicle->GetActorForwardVector() * 700.0f);
    DrawDebugLine(GetWorld(), Start, End, FColor::Magenta);
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECollisionChannel::ECC_Visibility, QueryParams))
    {
        UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *HitResult.Actor->GetName());    
        return true;
    }

    return false;
}

ALane* ACarController::FindClosestLane(float Radius)
{
    for (TActorIterator<ALane> It(GetWorld()); It; ++It)
    {
        ALane* LaneActor = *It;
        if (LaneActor)
        {
            return LaneActor;
        }
    }

    return nullptr;
}

bool ACarController::FindClosestWaypointIndex(TWeakObjectPtr<ALane>& OutLane, int32& OutWaypointIndex)
{
    if (!PosessedVehicle)
        return false;

    const FVector& VehicleLocation = PosessedVehicle->GetActorLocation();

    float ShortestDistanceSquared = std::numeric_limits<float>::max();
    for (TActorIterator<ALane> It(GetWorld()); It; ++It)
    {
        ALane* LaneActor = *It;
        if (LaneActor)
        {
            for (int i = 0; i < LaneActor->GetWaypoints().Num(); ++i)
            {
                const float DistanceSquared = (LaneActor->GetWaypointByIndex(i).Location - VehicleLocation).SizeSquared();
                if (DistanceSquared < ShortestDistanceSquared)
                {
                    ShortestDistanceSquared = DistanceSquared;
                    OutWaypointIndex = i;
                    OutLane = LaneActor;

                    
                }
            }
        }
    }

    if (OutLane != nullptr)
        return true;

    return false;
}

bool ACarController::HasValidWaypoint() const
{
    if (CurrentLane.IsValid() && CurrentWaypointIndex >= 0 && CurrentWaypointIndex < CurrentLane->GetWaypoints().Num())
        return true;

    return false;
}

float ACarController::CalculateTurnAngle(const FVector& VehicleLocation, const FVector& VehicleForward,
                                         const FVector& TargetLocation)
{
    // Get a normalized 2D Vector since the Z axis is not taken into account
    FVector2D VehicleForward2D(VehicleForward);
    VehicleForward2D.Normalize();

    // Calculate 2D Vector to target location
    FVector2D TargetVector2D(TargetLocation - VehicleLocation);
    TargetVector2D.Normalize();

    // Calculate orientation 0 - 180 for right, 0 - (-180) for left
    const float Angle1 = FMath::Atan2(VehicleForward2D.X, VehicleForward2D.Y);
    const float Angle2 = FMath::Atan2(TargetVector2D.X, TargetVector2D.Y);
    float TurnAngle = FMath::RadiansToDegrees(Angle1 - Angle2);
    if (TurnAngle > 180.0f)
    {
        TurnAngle -= 360.0f;
    }
    else if (TurnAngle < -180.0f)
    {
        TurnAngle += 360.0f;
    }

    return TurnAngle;
}

void ACarController::Drive() const
{
    if (!PosessedVehicle || !CurrentLane.IsValid() || CurrentWaypointIndex >= CurrentLane->GetWaypoints().Num())
        return;

    const FVector& VehicleLocation = PosessedVehicle->GetActorLocation();
    const FWaypoint& TargetWp = CurrentLane->GetWaypointByIndex(CurrentWaypointIndex);

    const float TurnAngle = CalculateTurnAngle(VehicleLocation, PosessedVehicle->GetActorForwardVector(), TargetWp.Location);
    //UE_LOG(LogTemp, Warning, TEXT("Turn Angle: %f"), TurnAngle);

    float Steering = 0.0f; 
    if (FMath::Abs(TurnAngle) < 1.0f)
    {
        Steering = 0.0f;
    }
    else if (TurnAngle < 45.0f)
    {
        Steering = 0.5f * FMath::Sign(TurnAngle);
    }
    else
    {
        Steering = FMath::Sign(TurnAngle);
    }    
    
    PosessedVehicle->GetVehicleMovement()->SetSteeringInput(Steering);

    float Throttle = 0.7f;
    if (Steering < 0.5f)
    {
        Throttle = 0.4f;
    }
    else
    {
        Throttle = 0.3f;
    }

    if (TargetWp.Stop || CheckCollisions())
    {
        PosessedVehicle->GetVehicleMovement()->SetThrottleInput(0.0f);
        PosessedVehicle->GetVehicleMovement()->SetBrakeInput(1.0f);
        //PosessedVehicle->GetVehicleMovement()->SetFixedBrakingDistance(1.0f);
    }
    else
    {
        PosessedVehicle->GetVehicleMovement()->SetThrottleInput(Throttle);
    }
    

    // Debug Stuff
    FVector DebugVehicleLocation = VehicleLocation;
    DebugVehicleLocation.Z += 10.f;
    DrawDebugLine(GetWorld(), DebugVehicleLocation, DebugVehicleLocation + PosessedVehicle->GetActorForwardVector() * 500.0f, FColor::Red);
    DrawDebugLine(GetWorld(), DebugVehicleLocation, TargetWp.Location, FColor::Blue);
}

