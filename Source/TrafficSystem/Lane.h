#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Lane.generated.h"

USTRUCT(BlueprintType)
struct FConnection
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TWeakObjectPtr<class ALane> Lane;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 Id;

	FConnection() = default;
	
	FConnection(const TWeakObjectPtr<class ALane>& InLane, const int32 InId)
	: Lane(InLane)
	, Id(InId)
	{}

	bool operator==(const FConnection& rhs) const
	{
		return Lane == rhs.Lane && Id == rhs.Id;
	}
};

USTRUCT(BlueprintType)
struct FWaypoint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	int32 Id;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving")
	bool Stop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Driving")
	float TargetSpeed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info")
	FVector Location;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info")
	TArray<FConnection> OutConnections;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Info")
	TArray<FConnection> InConnections;

	FWaypoint() = default;
	
	explicit FWaypoint(const int32 InId, FVector InLocation = FVector(), const float InTargetSpeed = 50.0f)
	: Id(InId)
	, Stop(false)
	, TargetSpeed(InTargetSpeed)
	, Location(std::move(InLocation))
	{}
};

UCLASS()
class TRAFFICSYSTEM_API ALane : public AActor
{
	using TWaypointMap = TMap<int32, int32>;

	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ALane();

	#if WITH_EDITOR
		UPROPERTY(EditAnywhere)
		bool DrawDebugEnabled;		
	#endif
	
	// AActor overrides
	virtual void PostActorCreated() override;
	virtual void Tick(float DeltaTime) override;

	// ALane
	virtual void AddWaypoint(const FVector& Location, float Speed = 50.0f);
	virtual void InsertWaypoint(const FVector& Location , int32 Index, float Speed = 50.0f);
	virtual void RemoveWaypoint(int32 RemoveIndex);
	virtual void RemoveAllWaypoints();

	virtual void ConnectWaypointTo(int32 FromIndex, ALane* ToLane, int32 ToIndex);
	virtual void DisconnectWaypointFrom(int32 FromIndex, ALane* ToLane, int32 ToIndex);

	bool HasWaypointId(int32 Id) const;
	bool HasWaypointAt(int32 Index) const;
	
	const TArray<FWaypoint>& GetWaypoints() const;
	const FWaypoint& GetWaypointByIndex(int32 Index) const;
	const FWaypoint& GetWaypointById(int32 Id) const;
	int32 GetWaypointIndex(int32 Id) const;
	int32 GetWaypointId(int32 Index) const;

	void SetStop(int32 Index, bool bStopFlag);
	void SetWaypointLocation(int32 Index, const FVector& Location);
	
	FVector GetWaypointDirection(int32 Index) const;
	
protected:
	virtual bool ShouldTickIfViewportsOnly() const override;
	
	#if WITH_EDITOR
		void DebugDrawLane();
	#endif

	static bool IsConnectionValid(const FConnection& Connection);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class USceneComponent* SceneComponent;

	UPROPERTY(EditInstanceOnly, EditFixedSize, BlueprintReadWrite, Category = "Waypoints")
	TArray<FWaypoint> Waypoints;

	UPROPERTY(VisibleAnywhere, Category = "Waypoints")
	TMap<int32, int32> MapWaypointIdToIndex;

private:
	bool AddOutConnectionAt(int32 WaypointIndex, FConnection OutConnection);
	bool AddInConnectionAt(int32 WaypointIndex, FConnection InConnection);
	void RemoveOutConnectionAt(int32 WaypointIndex, const FConnection& OutConnection);
	void RemoveInConnectionAt(int32 WaypointIndex, const FConnection& InConnection);
	TArray<FConnection> GetInConnectionsAt(int32 WaypointIndex) const;
	TArray<FConnection> GetOutConnectionsAt(int32 WaypointIndex) const;
	
	FWaypoint CreateWaypoint(FVector Location = FVector(), float Speed = 50.0f);
	
	int32 CalculateNextWaypointId() const;
	void RebuildWaypointMap();
};

