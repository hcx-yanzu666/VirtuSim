#pragma once

#include "CoreMinimal.h"
#include "ScenarioDefinition.h"

/** 一轮场景运行的最小结果快照；不保存实时对象指针。 */
struct FScenarioRunResult
{
    FString ScenarioId;
    FString RunId;
    FString State;
    /** false 表示报告已落盘，但外部任务是否结束仍不确定。 */
    bool bTaskEndedConfirmed = false;
    FString Detail;
    double ElapsedSeconds = 0.0;
    /** 仅在导航开始后有实际采样；初始化失败时不报告零距离。 */
    bool bTravelDistanceMeasured = false;
    double TravelDistanceMeters = 0.0;
    FVector GoalLocationCentimeters = FVector::ZeroVector;
    float GoalYawDegrees = 0.0f;
    FLidarParameters LidarParameters;
};

/** 将一轮结果写成易读 JSON。 */
class FScenarioRunResultWriter
{
public:
    static bool WriteJson(
        const FString& FilePath,
        const FScenarioRunResult& Result,
        FString& OutError);
};
