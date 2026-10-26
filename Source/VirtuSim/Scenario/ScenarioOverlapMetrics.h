#pragma once

#include "CoreMinimal.h"

/** 仅评价平面几何，不查询或修改 UE 物理碰撞状态。边界相切也计为重叠。 */
inline bool IsScenarioCircleOverlappingBox(
    const FBox2D& Box, const FVector2D& Center, double Radius)
{
    if (!Box.bIsValid)
    {
        return false;
    }
    const FVector2D Closest(
        FMath::Clamp(Center.X, Box.Min.X, Box.Max.X),
        FMath::Clamp(Center.Y, Box.Min.Y, Box.Max.Y));
    return (Center - Closest).SizeSquared() <= FMath::Square(Radius);
}

/** 任一行人重叠的时间并集；使用上一采样状态近似两个采样之间的区间。 */
struct FScenarioOverlapMetrics
{
    int32 EpisodeCount = 0;
    double DurationSeconds = 0.0;

    void Sample(bool bAnyOverlap, double SimulationSeconds)
    {
        if (bSampled && bPreviousAnyOverlap)
        {
            DurationSeconds += FMath::Max(0.0, SimulationSeconds - LastSampleSeconds);
        }
        if (bAnyOverlap && !bPreviousAnyOverlap)
        {
            ++EpisodeCount;
        }
        bSampled = true;
        bPreviousAnyOverlap = bAnyOverlap;
        LastSampleSeconds = SimulationSeconds;
    }

private:
    bool bSampled = false;
    bool bPreviousAnyOverlap = false;
    double LastSampleSeconds = 0.0;
};
