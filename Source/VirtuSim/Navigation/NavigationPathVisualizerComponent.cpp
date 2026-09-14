#include "NavigationPathVisualizerComponent.h"

#include "../Communication/RosCommunicationSubsystem.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"

UNavigationPathVisualizerComponent::UNavigationPathVisualizerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UNavigationPathVisualizerComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// TODO 1：检查 bVisualizationEnabled 和 GetWorld()，不满足条件就返回。
	if (!bVisualizationEnabled || GetWorld() == nullptr)
	{
		return;
	}
	const URosCommunicationSubsystem* RosSubsystem = GetWorld()->GetSubsystem<URosCommunicationSubsystem>();
	if (RosSubsystem == nullptr)
	{
		return;
	}
	FNavigationPathState PathState;
	const FVector HeightOffset(0.0f, 0.0f, HeightOffsetCentimeters);
	if (RosSubsystem->TryGetLatestNavigationPath(PathState))
	{
		for (int PointIndex = 1; PointIndex < PathState.Points.Num(); ++PointIndex)
		{
			DrawDebugLine(GetWorld(),PathState.Points[PointIndex - 1] + HeightOffset , PathState.Points[PointIndex] + HeightOffset,FColor::Green,false,0.0f,0,4.0f);
		}
	}
}
