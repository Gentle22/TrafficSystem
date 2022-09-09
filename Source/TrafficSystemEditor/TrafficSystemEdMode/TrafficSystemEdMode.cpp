#include "TrafficSystemEdMode.h"

#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "Chaos/ChaosDebugDraw.h"
#include "Engine/World.h"
#include "Engine/Selection.h"
#include "TrafficSystem/Lane.h"
#include "TrafficSystem/TrafficLight.h"
#include "TrafficSystem/TrafficLightsController.h"

// Implement the HitProxy for our Waypoints
IMPLEMENT_HIT_PROXY(HLaneWaypointHitProxy, HHitProxy)
IMPLEMENT_HIT_PROXY(HLaneWaypointEdgeHitProxy, HHitProxy)
IMPLEMENT_HIT_PROXY(HLaneWaypointOutConnectionHitProxy, HHitProxy)

// Set the ID for our Editor Mode
const FEditorModeID FTrafficSystemEdMode::EM_TrafficSystemEdMode(TEXT("EM_TrafficSystemEdMode"));

// ---------------------------------------------------------------------------------------------------------------------

FTrafficSystemEdMode::FTrafficSystemEdMode()
: FEdMode()
{
    if (GEditor)
    {
        // Register function to handle the deletion of a a Lane actor
        OnActorDeleteHandle = GEditor->OnLevelActorDeleted().AddLambda([](AActor* Actor)
        {
            UE_LOG(LogTemp, Warning, TEXT("Actor deleted. Remove all waypoints."));
            ALane* Lane = Cast<ALane>(Actor);
            if (Lane)
                Lane->RemoveAllWaypoints();
        });

        // Register function to handle moving a Lane actor around
        OnActorMoveHandle = GEditor->OnActorMoving().AddLambda([](AActor* Actor)
        {
            ALane* Lane = Cast<ALane>(Actor);
            if (Lane && Lane->GetWaypoints().Num() > 0)
            {
                const FVector& NewActorLocation = Lane->GetActorLocation();
                const FVector& FirstWaypointLocation = Lane->GetWaypointByIndex(0).Location;

                const FVector MoveDelta = NewActorLocation - FirstWaypointLocation;
                for (int32 Index = 0; Index < Lane->GetWaypoints().Num(); ++Index)
                {
                    Lane->SetWaypointLocation(Index, Lane->GetWaypointByIndex(Index).Location + MoveDelta);
                }
            }
        });
    }
}

// ---------------------------------------------------------------------------------------------------------------------

FTrafficSystemEdMode::~FTrafficSystemEdMode()
{
    if (GEditor)
    {
        GEditor->OnLevelActorDeleted().Remove(OnActorDeleteHandle);
        GEditor->OnLevelActorDeleted().Remove(OnActorMoveHandle);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::Enter()
{
    FEdMode::Enter();
    
    FTrafficSystemEditorCommands::Register();
    TrafficSystemEdModeActions = MakeShareable(new FUICommandList);
    MapCommands();
    
    ResetCurrentSelection();

    // Disable debug drawing which should only happen in the other editor modes
    for (TActorIterator<ALane> It(GetWorld()); It; ++It)
    {
        ALane* LaneActor = *It;
        if (LaneActor && LaneActor->DrawDebugEnabled)
        {
            LaneActorsWithDrawDebugEnabled.Add(LaneActor);
            LaneActor->DrawDebugEnabled = false;
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::Exit()
{
    // Restore debug drawing when leaving the Traffic System editor mode
    for (TWeakObjectPtr<ALane> LaneActor : LaneActorsWithDrawDebugEnabled)
    {
        if (LaneActor.IsValid())
        {
            LaneActor->DrawDebugEnabled = true;
        }
    }
    LaneActorsWithDrawDebugEnabled.Empty();

    TrafficSystemEdModeActions.Reset();
    FTrafficSystemEditorCommands::Unregister();

    FEdMode::Exit();
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
    UWorld* World = GetWorld();
    for (TActorIterator<ALane> It(World); It; ++It)
    {
        RenderLane(*It, PDI);
    }

    for (TActorIterator<ATrafficLight> It(World); It; ++It)
    {
        RenderTrafficLight(*It, PDI);
    }

    for (TActorIterator<ATrafficLightsController> It(World); It; ++It)
    {
        RenderTrafficLightsController(*It, PDI);
    }

    FEdMode::Render(View, Viewport, PDI);
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RenderLane(ALane* Lane, FPrimitiveDrawInterface* PDI) const
{
    if (!Lane || !PDI)
        return;

    for (int WaypointIndex = 0; WaypointIndex < Lane->GetWaypoints().Num(); ++WaypointIndex)
    {
        // Render Waypoint as arrow
        PDI->SetHitProxy(new HLaneWaypointHitProxy(Lane, WaypointIndex));
        RenderWaypoint(Lane->GetWaypointByIndex(WaypointIndex), Lane->GetWaypointDirection(WaypointIndex),
                       IsSelectedWaypoint(Lane, WaypointIndex), PDI);
        PDI->SetHitProxy(nullptr);

        // Render edges between waypoints
        if (WaypointIndex > 0)
        {
            const auto& FromWaypoint = Lane->GetWaypointByIndex(WaypointIndex - 1);
            const auto& ToWaypoint = Lane->GetWaypointByIndex(WaypointIndex);
            PDI->SetHitProxy(new HLaneWaypointEdgeHitProxy(Lane, WaypointIndex - 1, WaypointIndex));
            PDI->DrawLine(FromWaypoint.Location, ToWaypoint.Location, FLinearColor::Green, SDPG_Foreground);
            PDI->SetHitProxy(nullptr);
        }

        // Render connections between waypoints
        const auto& OutConnections = Lane->GetWaypointByIndex(WaypointIndex).OutConnections;
        for (int OutConnectionIndex = 0; OutConnectionIndex < OutConnections.Num(); ++OutConnectionIndex)
        {
            const auto& Connection = OutConnections[OutConnectionIndex];

            if (!Connection.Lane.IsValid())
                continue;

            const auto& FromWaypoint = Lane->GetWaypointByIndex(WaypointIndex);
            
            const FWaypoint& ToWaypoint = Connection.Lane->GetWaypointById(Connection.Id);
            RenderWaypointOutConnection(Lane, FromWaypoint, Connection.Lane.Get(), ToWaypoint, PDI);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RenderWaypoint(const FWaypoint& Waypoint, const FVector& Direction,
                                          const bool Selected, FPrimitiveDrawInterface* PDI)
{
    if (!PDI)
        return;

    const FColor DrawColor = Selected ? FColor::White : (Waypoint.Stop ? FColor::Red : FColor::Green);

    const float ArrowLength = 150.0f;
    const float ArrowWidth = 150.0f;
    const FVector ArrowTip = Waypoint.Location + Direction * ArrowLength;
    
    const float PointSize = 5.0f;
    PDI->DrawPoint(Waypoint.Location, DrawColor, PointSize, SDPG_Foreground);
    RenderArrow(ArrowTip, Direction, ArrowLength, ArrowWidth, DrawColor, PDI); 
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RenderWaypointOutConnection(ALane* FromLane, const FWaypoint& FromWaypoint, ALane* ToLane,
                                                       const FWaypoint& ToWaypoint, FPrimitiveDrawInterface* PDI)
{
    if (!FromLane || !ToLane || !PDI)
        return;

    const FVector& StartLocation = FromWaypoint.Location;
    const FVector& EndLocation = ToWaypoint.Location;

    PDI->SetHitProxy(new HLaneWaypointOutConnectionHitProxy(FromLane, FromLane->GetWaypointIndex(FromWaypoint.Id),
                                                            ToLane, ToLane->GetWaypointIndex(ToWaypoint.Id)));
    DrawDashedLine(PDI, StartLocation, EndLocation, FLinearColor::Yellow, 100.0f, SDPG_Foreground);
    RenderArrow(EndLocation, (EndLocation - StartLocation).GetSafeNormal(), 150.0f, 150.0f, FLinearColor::Yellow, PDI);
    PDI->SetHitProxy(nullptr);
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RenderArrow(const FVector& ArrowTip, const FVector& Direction, const float ArrowLength,
                                       const float ArrowWidth, const FLinearColor& Color, FPrimitiveDrawInterface* PDI)
{
    // Calculate Vertices for Arrow
    const FVector UnitDirection = Direction.GetSafeNormal();
    FVector YAxis;
    FVector ZAxis;
    UnitDirection.FindBestAxisVectors(YAxis, ZAxis);

    const FVector ArrowTail = ArrowTip - UnitDirection * ArrowLength;
    const FVector ArrowTail0 = ArrowTail + (ZAxis * ArrowWidth * 0.5f);
    const FVector ArrowTail1 = ArrowTail - (ZAxis * ArrowWidth * 0.5f);
    
    PDI->DrawLine(ArrowTail0, ArrowTip, Color, SDPG_Foreground, 5.0f);
    PDI->DrawLine(ArrowTail1, ArrowTip, Color, SDPG_Foreground, 5.0f);
    PDI->DrawLine(ArrowTail0, ArrowTail1, Color, SDPG_Foreground, 5.0f);

}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RenderTrafficLight(ATrafficLight* TrafficLight, FPrimitiveDrawInterface* PDI) const
{
    if (!TrafficLight || !PDI)
        return;

    for (int32 Index = 0; Index < TrafficLight->ConnectedWaypoints.Num(); ++Index)
    {
        const auto& Connection = TrafficLight->ConnectedWaypoints[Index];
        if (!Connection.Lane.IsValid() || !Connection.Lane->HasWaypointId(Connection.Id))
            continue;

        const FVector& Start = TrafficLight->GetActorLocation();
        const FVector& End = Connection.Lane->GetWaypointById(Connection.Id).Location;
        const FLinearColor DrawColor = TrafficLight->bStop ? FLinearColor::Red : FLinearColor::Green;
        DrawDashedLine(PDI, Start, End, DrawColor, 25.0f, SDPG_Foreground);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RenderTrafficLightsController(ATrafficLightsController* TrafficLightsController,
                                                         FPrimitiveDrawInterface* PDI) const
{
    if (!TrafficLightsController || !PDI)
        return;

    const TArray<FTrafficLightGroup>& TrafficLightsGroups = TrafficLightsController->GetGroups();
    for (int32 Index = 0; Index < TrafficLightsGroups.Num(); ++Index)
    {
        const FTrafficLightGroup& TrafficLightGroup = TrafficLightsGroups[Index];
        for (int32 TrafficLightIndex = 0; TrafficLightIndex < TrafficLightGroup.TrafficLights.Num(); ++TrafficLightIndex)
        {
            const ATrafficLight* TrafficLight = TrafficLightGroup.TrafficLights[TrafficLightIndex];
            if (!IsValid(TrafficLight))
                continue;

            const FVector& Start = TrafficLightsController->GetActorLocation();
            const FVector& End = TrafficLight->GetActorLocation();
            const FLinearColor DrawColor = FLinearColor::Blue;
            DrawDashedLine(PDI, Start, End, DrawColor, 25.0f, SDPG_Foreground);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy,
                                       const FViewportClick& Click)
{
    bool IsHandled = false; 
    
    // Did we hit something
    if (HitProxy)
    {
        if (HitProxy->IsA(HActor::StaticGetType()))
        {
            HActor* HitActor = static_cast<HActor*>(HitProxy);
            ATrafficLight* TrafficLight = Cast<ATrafficLight>(HitActor->Actor);
            if (TrafficLight && Click.IsAltDown() && HasValidSelection())
            {
                // Add connection to traffic light
                IsHandled = true;
                UE_LOG(LogTemp, Warning, TEXT("Click on Actor with Traffic Light Component"));
                const int32 WaypointId = CurrentSelectedLane->GetWaypointId(CurrentSelectedWaypointIndex);
                //TrafficLight->ConnectedWaypoints.Add(FConnection(CurrentSelectedLane, WaypointId));
                TrafficLight->AddWaypointConnection(CurrentSelectedLane.Get(), WaypointId);
            }
        }
        
        // Handle Click on Waypoint
        if (HitProxy->IsA(HLaneWaypointHitProxy::StaticGetType()))
        {
            IsHandled = true;
            
            HLaneWaypointHitProxy* LaneWayPointHitProxy = static_cast<HLaneWaypointHitProxy*>(HitProxy);
            ALane* ClickedLane = Cast<ALane>(LaneWayPointHitProxy->Lane);
            const int32 ClickedWaypointIndex = LaneWayPointHitProxy->Index;

            if (Click.IsAltDown())
            {
                // Create connection between 2 waypoints
                if (CurrentSelectedLane.IsValid() && CurrentSelectedLane != ClickedLane)
                {
                    const FScopedTransaction Transaction(FText::FromString("Add Connection"));
                    CurrentSelectedLane->Modify();
                    ClickedLane->Modify();
                    CurrentSelectedLane->ConnectWaypointTo(CurrentSelectedWaypointIndex, ClickedLane, ClickedWaypointIndex);
                } 
            }
            else
            {
                // Select clicked waypoint
                SelectWaypoint(ClickedLane, ClickedWaypointIndex); 
            }

            if (Click.GetKey() == EKeys::RightMouseButton)
            {
                // Waypoint right click context menu
                const auto MenuWidget = GenerateWaypointContextMenu();
                if (MenuWidget.IsValid())
                {
                    FSlateApplication::Get().PushMenu(Owner->GetToolkitHost()->GetParentWidget(),
                        FWidgetPath(),
                        MenuWidget.ToSharedRef(),
                        FSlateApplication::Get().GetCursorPos(),
                        FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
            
                }
            }
        }
        else if (HitProxy->IsA(HLaneWaypointEdgeHitProxy::StaticGetType()))
        {
            IsHandled = true;
            
            if (Click.GetKey() == EKeys::RightMouseButton)
            {
                // Edge right click context menu
                HLaneWaypointEdgeHitProxy* WaypointEdgeHitProxy = static_cast<HLaneWaypointEdgeHitProxy*>(HitProxy);
                ALane* ClickedLane = Cast<ALane>(WaypointEdgeHitProxy->Lane);
                const int32 FromIndex = WaypointEdgeHitProxy->FromWaypointIndex;
                const int32 ToIndex = WaypointEdgeHitProxy->ToWaypointIndex;
                
                
                const auto MenuWidget = GenerateEdgeContextMenu(ClickedLane, FromIndex, ToIndex);
                if (MenuWidget && MenuWidget.IsValid())
                {
                    FSlateApplication::Get().PushMenu(Owner->GetToolkitHost()->GetParentWidget(),
                        FWidgetPath(),
                        MenuWidget.ToSharedRef(),
                        FSlateApplication::Get().GetCursorPos(),
                        FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
                }
            }
        }
        else if (HitProxy->IsA(HLaneWaypointOutConnectionHitProxy::StaticGetType()))
        {
            IsHandled = true;
            
            if (Click.GetKey() == EKeys::RightMouseButton)
            {
                // Connection right click context menu
                HLaneWaypointOutConnectionHitProxy* LaneWayPointHitProxy = static_cast<HLaneWaypointOutConnectionHitProxy*>(HitProxy);
                ALane* FromLane = Cast<ALane>(LaneWayPointHitProxy->FromLane);
                const int32 FromIndex = LaneWayPointHitProxy->FromWaypointIndex;
                ALane* ToLane = Cast<ALane>(LaneWayPointHitProxy->ToLane);
                const int32 ToIndex = LaneWayPointHitProxy->ToWaypointIndex;
                
                const auto MenuWidget = GenerateConnectionContextMenu(FromLane, FromIndex, ToLane, ToIndex);
                if (MenuWidget && MenuWidget.IsValid())
                {
                    FSlateApplication::Get().PushMenu(Owner->GetToolkitHost()->GetParentWidget(),
                        FWidgetPath(),
                        MenuWidget.ToSharedRef(),
                        FSlateApplication::Get().GetCursorPos(),
                        FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu));
                }
            }
        }
    }

    // Handle Ctrl + Left Click
    if (CanAddWaypoint() && Click.IsControlDown())
    {
        IsHandled = true;

        // Calculate world position and direction from click position
        FVector WorldPosition;
        FVector WorldDirection;
        GetWorldPositionAndDirection(Click.GetClickPos(), GetSceneView(InViewportClient), WorldPosition,
                                     WorldDirection);

        // Add a new waypoint to the selected lane
        FVector HitLocation;
        if (CalculateHitLocation(WorldPosition, WorldDirection, HitLocation))
        {
            AddWaypoint(HitLocation);
        }        
    }

    return IsHandled;
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::InputDelta(FEditorViewportClient* InViewportClient, FViewport* InViewport, FVector& InDrag,
                                      FRotator& InRot, FVector& InScale)
{
    if (InViewportClient->GetCurrentWidgetAxis() == EAxisList::None)
        return false;

    if (HasValidSelection())
    {
        if (!InDrag.IsZero())
        {
            CurrentSelectedLane->Modify();
            FVector NewLocation = CurrentSelectedLane->GetWaypointByIndex(CurrentSelectedWaypointIndex).Location;
            NewLocation += InDrag;
            CurrentSelectedLane->SetWaypointLocation(CurrentSelectedWaypointIndex, NewLocation);
            
            if (CurrentSelectedWaypointIndex == 0 && CurrentSelectedLane->GetActorLocation() != NewLocation)
                CurrentSelectedLane->SetActorLocation(NewLocation);
        }
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key,
                                    EInputEvent Event)
{
    bool isHandled = false;

    if (!isHandled && Event == IE_Pressed)
    {
        isHandled = TrafficSystemEdModeActions->ProcessCommandBindings(Key, FSlateApplication::Get().GetModifierKeys(),
                                                                       false);
    }
    
    return isHandled;
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::ActorSelectionChangeNotify()
{
    if (GetSelectedLaneActor() != CurrentSelectedLane)
        ResetCurrentSelection();
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::UsesTransformWidget(FWidget::EWidgetMode CheckMode) const
{
    return (CheckMode == FWidget::EWidgetMode::WM_Translate);
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::ShowModeWidgets() const
{
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::ShouldDrawWidget() const
{
    return true;
}

// ---------------------------------------------------------------------------------------------------------------------

FVector FTrafficSystemEdMode::GetWidgetLocation() const
{
    if (GetSelectedLaneActor() == CurrentSelectedLane && HasValidSelection())
    {
        return CurrentSelectedLane->GetWaypointByIndex(CurrentSelectedWaypointIndex).Location;
    }

    return FEdMode::GetWidgetLocation();
}

// ---------------------------------------------------------------------------------------------------------------------

ALane* FTrafficSystemEdMode::GetSelectedLaneActor() const
{
    TArray<ALane*> SelectedObjects;
    GEditor->GetSelectedActors()->GetSelectedObjects(SelectedObjects);

    // If only one Lane is selected
    if (SelectedObjects.Num() == 1)
    {
        return Cast<ALane>(SelectedObjects[0]);
    }

    return nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::SelectWaypoint(ALane* LaneActor, int32 WaypointIndex)
{
    if (CurrentSelectedLane != LaneActor)
    {
        GEditor->SelectNone(true, true);
        CurrentSelectedLane = LaneActor;
        if (CurrentSelectedLane.IsValid())
        {
            GEditor->SelectActor(CurrentSelectedLane.Get(), true, true);
        }
    }

    if (CurrentSelectedWaypointIndex != WaypointIndex)
        CurrentSelectedWaypointIndex = WaypointIndex;
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::IsSelectedWaypoint(const ALane* Lane, const int32 Index) const
{
    return CurrentSelectedLane.IsValid() &&
           Lane == CurrentSelectedLane && 
           CurrentSelectedWaypointIndex == Index;
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::CanAddWaypoint() const
{
    ALane* Actor = GetSelectedLaneActor();
    return Actor != nullptr;
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::AddWaypoint(const FVector& Location, float TargetSpeed)
{    
    ALane* Actor = GetSelectedLaneActor();
    if (Actor)
    {
        const FScopedTransaction Transaction(FText::FromString("Add Waypoint"));
        Actor->Modify();
        Actor->AddWaypoint(Location, TargetSpeed);
        SelectWaypoint(Actor, Actor->GetWaypoints().Num() - 1);
    }
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::CanRemoveSelectedWaypoint() const
{
    if (HasValidSelection())
    {
        return true;    
    }

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RemoveSelectedWaypoint()
{
    if (HasValidSelection())
    {
        const FScopedTransaction Transaction(FText::FromString("Add Waypoint"));
        CurrentSelectedLane->Modify();
        
        CurrentSelectedLane->RemoveWaypoint(CurrentSelectedWaypointIndex);
        if (CurrentSelectedLane->GetWaypoints().Num() > 0)
        {
            SelectWaypoint(CurrentSelectedLane.Get(), FMath::Max(0, CurrentSelectedWaypointIndex - 1));
        }
        
    }
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::InsertWaypoint(ALane* FromLane, int32 FromIndex, int32 ToIndex)
{
    const FVector& FromLocation = FromLane->GetWaypointByIndex(FromIndex).Location;
    const FVector& ToLocation = FromLane->GetWaypointByIndex(ToIndex).Location;

    const FVector Direction = ToLocation - FromLocation;
    const float Distance = Direction.Size();
    const FVector UnitDirection = Direction.GetSafeNormal();

    const FVector WaypointLocation = FromLocation + (UnitDirection * Distance * 0.5f);

    const FScopedTransaction Transaction(FText::FromString("Insert Waypoint"));

    FromLane->Modify();
    FromLane->InsertWaypoint(WaypointLocation, ToIndex);
    SelectWaypoint(FromLane, ToIndex);
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::HasValidSelection() const
{
    return (CurrentSelectedLane.IsValid() &&
            CurrentSelectedWaypointIndex >= 0 &&
            CurrentSelectedWaypointIndex < CurrentSelectedLane->GetWaypoints().Num());
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::RemoveConnection(ALane* FromLane, int32 FromIndex, ALane* ToLane, int32 ToIndex)
{
    const FScopedTransaction Transaction(FText::FromString("Remove Connection"));
    FromLane->Modify();
    ToLane->Modify();
    FromLane->DisconnectWaypointFrom(FromIndex, ToLane, ToIndex);   
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::ResetCurrentSelection()
{
    CurrentSelectedLane = nullptr;
    CurrentSelectedWaypointIndex = -1;
}

// ---------------------------------------------------------------------------------------------------------------------

bool FTrafficSystemEdMode::CalculateHitLocation(const FVector& WorldPosition, const FVector& WorldDirection,
                                                FVector& out_HitLocation, const float Distance) const
{
    FHitResult HitResult;
    const FVector WorldEnd = WorldPosition + (Distance * WorldDirection);
    if (GetWorld()->LineTraceSingleByChannel(HitResult, WorldPosition, WorldEnd, ECC_Visibility))
    {
        out_HitLocation = HitResult.ImpactPoint;
        out_HitLocation.Z += 10.f;
        return true;
    }

    return false;
}

// ---------------------------------------------------------------------------------------------------------------------

FSceneView* FTrafficSystemEdMode::GetSceneView(FEditorViewportClient* ViewportClient)
{
    FSceneViewFamilyContext ViewFamily(FSceneViewFamily::ConstructionValues(       
            ViewportClient->Viewport, 
            ViewportClient->GetScene(), 
            ViewportClient->EngineShowFlags));

    return ViewportClient->CalcSceneView(&ViewFamily);
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::GetWorldPositionAndDirection(const FIntPoint& ClickPos, const FSceneView* SceneView,
                                                        FVector& out_WorldPosition, FVector& out_WorldDirection)
{
    SceneView->DeprojectFVector2D(FVector2D(ClickPos), out_WorldPosition, out_WorldDirection);
}

// ---------------------------------------------------------------------------------------------------------------------

void FTrafficSystemEdMode::MapCommands()
{
    const auto& Commands = FTrafficSystemEditorCommands::Get();
    TrafficSystemEdModeActions->MapAction(Commands.DeleteWaypoint,
        FExecuteAction::CreateSP(this, &FTrafficSystemEdMode::RemoveSelectedWaypoint),
        FCanExecuteAction::CreateSP(this, &FTrafficSystemEdMode::CanRemoveSelectedWaypoint));
}

// ---------------------------------------------------------------------------------------------------------------------

TSharedPtr<SWidget> FTrafficSystemEdMode::GenerateWaypointContextMenu() const
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.PushCommandList(TrafficSystemEdModeActions.ToSharedRef());
    MenuBuilder.BeginSection("Waypoints");

    if (HasValidSelection())
    {
        TSharedRef<SWidget> LabelWidget =
            SNew(STextBlock)
            .Text(FText::FromString(FString::FromInt(CurrentSelectedWaypointIndex)))
            .ColorAndOpacity(FLinearColor::Green);
        MenuBuilder.AddWidget(LabelWidget, FText::FromString(TEXT("Waypoint Index: ")));
        MenuBuilder.AddMenuSeparator();
        MenuBuilder.AddMenuEntry(FTrafficSystemEditorCommands::Get().DeleteWaypoint);
    }
    
    MenuBuilder.EndSection();
    MenuBuilder.PopCommandList();

    TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();
    return MenuWidget;
}

// ---------------------------------------------------------------------------------------------------------------------

TSharedPtr<SWidget> FTrafficSystemEdMode::GenerateEdgeContextMenu(ALane* Lane, int32 FromIndex, int32 ToIndex)
{
    FMenuBuilder MenuBuilder(true, nullptr);

    MenuBuilder.BeginSection("Edge");

    TSharedRef<SWidget> LabelWidget =
        SNew(STextBlock)
            .Text(FText::FromString(FString::FromInt(ToIndex)))
            .ColorAndOpacity(FLinearColor::Green);
    MenuBuilder.AddWidget(LabelWidget, FText::FromString(TEXT("Waypoint Index: ")));
    MenuBuilder.AddMenuSeparator();

    TSharedRef<SWidget> ActionWidget =
        SNew(STextBlock)
            .Text(FText::FromString(TEXT("Insert Waypoint")))
            .ColorAndOpacity(FLinearColor::Green);
    FUIAction Action = FUIAction(FExecuteAction::CreateLambda([=]()
    {
        InsertWaypoint(Lane, FromIndex, ToIndex);
    }));
    MenuBuilder.AddMenuEntry(Action, ActionWidget);

    MenuBuilder.EndSection();

    TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();
    return MenuWidget;
}

// ---------------------------------------------------------------------------------------------------------------------

TSharedPtr<SWidget> FTrafficSystemEdMode::GenerateConnectionContextMenu(ALane* FromLane, int32 FromIndex,
                                                                        ALane* ToLane, int32 ToIndex)
{
    FMenuBuilder MenuBuilder(true, nullptr);
    MenuBuilder.BeginSection("Connections");

    TSharedRef<SWidget> LabelWidget =
        SNew(STextBlock)
            .Text(FText::FromString(FString::FromInt(0)))
            .ColorAndOpacity(FLinearColor::Green);
    MenuBuilder.AddWidget(LabelWidget, FText::FromString(TEXT("Waypoint Index: ")));
    MenuBuilder.AddMenuSeparator();
    
    TSharedRef<SWidget> ActionWidget =
        SNew(STextBlock)
            .Text(FText::FromString(TEXT("Remove Connection")))
            .ColorAndOpacity(FLinearColor::Green);
    FUIAction Action = FUIAction(FExecuteAction::CreateLambda([=]()
    {
        RemoveConnection(FromLane, FromIndex, ToLane, ToIndex);
    }));
    MenuBuilder.AddMenuEntry(Action, ActionWidget);

    MenuBuilder.EndSection();

    TSharedPtr<SWidget> MenuWidget = MenuBuilder.MakeWidget();
    return MenuWidget;
}

