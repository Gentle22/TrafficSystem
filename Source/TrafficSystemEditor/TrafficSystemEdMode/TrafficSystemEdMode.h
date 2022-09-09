#pragma once

#include "EditorModeManager.h"
#include "TrafficSystem/Lane.h"
#include "TrafficSystem/TrafficLightsController.h"
#include "TrafficSystemEditor/TrafficSystemEditorModule.h"
#include "UnrealEd/Public/EdMode.h"

class ALane;
class ATrafficLight;

// ---------------------------------------------------------------------------------------------------------------------

struct HLaneWaypointHitProxy : HHitProxy
{
    DECLARE_HIT_PROXY()

    HLaneWaypointHitProxy(UObject* InLane, int32 InWaypointIndex)
    : HHitProxy(HPP_UI)
    , Lane(InLane)
    , Index(InWaypointIndex)
    {}
    
    UObject* Lane;
    int32 Index;
};

struct HLaneWaypointEdgeHitProxy : HHitProxy
{
    DECLARE_HIT_PROXY()

    HLaneWaypointEdgeHitProxy(UObject* InLane, int32 InFromWaypointIndex, int32 InToWaypointIndex)
    : HHitProxy(HPP_UI)
    , Lane(InLane)
    , FromWaypointIndex(InFromWaypointIndex)
    , ToWaypointIndex(InToWaypointIndex)
    {}
    
    UObject* Lane;
    int32 FromWaypointIndex;
    int32 ToWaypointIndex;
};

struct HLaneWaypointOutConnectionHitProxy : HHitProxy
{
    DECLARE_HIT_PROXY()

    HLaneWaypointOutConnectionHitProxy(UObject* InFromLane, int32 InFromWaypointIndex,
                                       UObject* InToFromLane, int32 InToWaypointIndex)
    : HHitProxy(HPP_UI)
    , FromLane(InFromLane)
    , FromWaypointIndex(InFromWaypointIndex)
    , ToLane(InToFromLane)
    , ToWaypointIndex(InToWaypointIndex)
    {}
    
    UObject* FromLane;
    int32 FromWaypointIndex;
    UObject* ToLane;
    int32 ToWaypointIndex;
};

// ---------------------------------------------------------------------------------------------------------------------

class FTrafficSystemEditorCommands : public TCommands<FTrafficSystemEditorCommands>
{
public:
    FTrafficSystemEditorCommands()
    : TCommands<FTrafficSystemEditorCommands>("TrafficSystemEditor",
        FText::FromString(TEXT("Traffic System Editor")),
        NAME_None,
        FEditorStyle::GetStyleSetName())
    {}

#define LOCTEXT_NAMESPACE ""
    virtual void RegisterCommands() override
    {
        UI_COMMAND(DeleteWaypoint, "Delete Waypoint", "Delete the currently selected Waypoint.",
            EUserInterfaceActionType::Button, FInputChord(EKeys::Delete));
        UI_COMMAND(RemoveConnection, "Remove Connection", "Remove right clicked connection.",
            EUserInterfaceActionType::Button, FInputChord());
    }
#undef LOCTEST_NAMESPACE

    TSharedPtr<FUICommandInfo> DeleteWaypoint;
    TSharedPtr<FUICommandInfo> RemoveConnection;
};

// ---------------------------------------------------------------------------------------------------------------------

class FTrafficSystemEdMode : public FEdMode
{
public:
    const static FEditorModeID EM_TrafficSystemEdMode;

    FTrafficSystemEdMode();
    ~FTrafficSystemEdMode();
    
    // FEdMode overrides
    virtual void Enter() override;
    virtual void Exit() override;
    
    virtual void Render(const FSceneView* View,FViewport* Viewport,FPrimitiveDrawInterface* PDI) override;

    virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                             const FViewportClick& Click) override;
    virtual bool InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag,
                            FRotator& InRot, FVector& InScale) override;
    virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event) override;

    virtual void ActorSelectionChangeNotify() override;

    virtual bool UsesTransformWidget(FWidget::EWidgetMode CheckMode) const override;
    virtual bool ShowModeWidgets() const override;
    virtual bool ShouldDrawWidget() const override;
    virtual FVector GetWidgetLocation() const override;

    // Traffic System Editor Mode
    ALane* GetSelectedLaneActor() const;

    void SelectWaypoint(ALane* LaneActor, int32 WaypointIndex);
    bool CanAddWaypoint() const;
    void AddWaypoint(const FVector& Location, float TargetSpeed = 50.0f);
    bool CanRemoveSelectedWaypoint() const;
    void RemoveSelectedWaypoint();
    bool HasValidSelection() const;
    
    static void RemoveConnection(ALane* FromLane, int32 FromIndex, ALane* ToLane, int32 ToIndex);
    void InsertWaypoint(ALane* FromLane, int32 FromIndex, int32 ToIndex);
    
protected:
    void ResetCurrentSelection();
    
    bool CalculateHitLocation(const FVector& WorldPosition, const FVector& WorldDirection,
                              FVector& out_HitLocation, const float Distance = 10000.f) const;
    
    // Retrieve Scene from Viewport
    static FSceneView* GetSceneView(FEditorViewportClient* ViewportClient);
    
    // Calculate world position and direction from click position
    static void GetWorldPositionAndDirection(const FIntPoint& ClickPos, const FSceneView* SceneView,
                                             FVector& out_WorldPosition, FVector& out_WorldDirection);

    bool IsSelectedWaypoint(const ALane* Lane, const int32 Index) const;

    void RenderLane(ALane* Lane, FPrimitiveDrawInterface* PDI) const;
    static void RenderWaypoint(const FWaypoint& Waypoint, const FVector& Direction,
                               const bool Selected, FPrimitiveDrawInterface* PDI);
    static void RenderWaypointOutConnection(ALane* FromLane, const FWaypoint& FromWaypoint, ALane* ToLane,
                                            const FWaypoint& ToWaypoint, FPrimitiveDrawInterface* PDI);
    static void RenderArrow(const FVector& ArrowTip, const FVector& Direction, float ArrowLength, float ArrowWidth,
                            const FLinearColor& Color, FPrimitiveDrawInterface* PDI);

    void RenderTrafficLight(ATrafficLight* TrafficLight, FPrimitiveDrawInterface* PDI) const;
    void RenderTrafficLightsController(ATrafficLightsController* TrafficLightsController,
                                       FPrimitiveDrawInterface* PDI) const;

    // Commands and context menues
    void MapCommands();
    TSharedPtr<SWidget> GenerateWaypointContextMenu() const;
    TSharedPtr<SWidget> GenerateEdgeContextMenu(ALane* FromLane, int32 FromIndex, int32 ToIndex);
    TSharedPtr<SWidget> GenerateConnectionContextMenu(ALane* FromLane, int32 FromIndex, ALane* ToLane, int32 ToIndex);
    
    TWeakObjectPtr<ALane> CurrentSelectedLane;
    int32 CurrentSelectedWaypointIndex = -1;

    TSharedPtr<FUICommandList> TrafficSystemEdModeActions;
    
    

    FDelegateHandle OnActorDeleteHandle;
    FDelegateHandle OnActorMoveHandle;
    
    TArray<TWeakObjectPtr<ALane>> LaneActorsWithDrawDebugEnabled;
};




