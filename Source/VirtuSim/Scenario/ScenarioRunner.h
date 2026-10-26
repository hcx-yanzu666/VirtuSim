#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioDefinition.h"
#include "ScenarioRunResult.h"
#include "ScenarioOverlapMetrics.h"
#include "ScenarioRunner.generated.h"  // 必须是最后一个 include

class ADynamicPedestrianObstacle;

UENUM()
enum class EScenarioRunState : uint8
{
    Idle,
    Initializing,
    WaitingForAcceptance,
    WaitingForCancel,
    Navigating,
    Succeeded,
    Failed,
    Canceled,
    TimeOut,
    SetupFailed,
    AcceptanceTimedOut,
    Interrupted
};
UCLASS()
class VIRTUSIM_API AScenarioRunner : public AActor
{
    GENERATED_BODY()

public:
    AScenarioRunner();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    // 编辑器中设置场景文件名。
    UPROPERTY(EditAnywhere, Category = "Scenario")
    FString ScenarioFileName = TEXT("warehouse_baseline.json");

    // 保存加载后的配置。
    FScenarioDefinition Scenario;

    // 仅启动本轮 JSON 配置的行人，不影响关卡中其他自动往返行人。
    TArray<TWeakObjectPtr<ADynamicPedestrianObstacle>> ConfiguredPedestrians;
    TWeakObjectPtr<AActor> ScenarioRobotActor;
    FVector LastRobotLocation = FVector::ZeroVector;
    double TravelDistanceCentimeters = 0.0;
    bool bTravelDistanceStarted = false;
    bool bOverlapEvaluationAvailable = false;
    bool bOverlapSamplingStarted = false;
    // 几何缺失时本轮评价不完整，不得报告为“未重叠”。
    bool bOverlapEvaluationIncomplete = false;
    FScenarioOverlapMetrics OverlapMetrics;

    EScenarioRunState RunState = EScenarioRunState::Idle;
    float RunElapsedSeconds = 0.0f;
    FString RunId;

    void HandleNavigationStatus(const FString& Status);
    void RequestRunCancel(EScenarioRunState Reason);
    EScenarioRunState PendingFinalState = EScenarioRunState::Canceled;
    double WaitStartedWallSeconds = 0.0;
    bool bResultWritten = false;
    bool bTaskEndedConfirmed = false;
    FString ResultDetail;
    FDelegateHandle StatusDelegateHandle;

    UPROPERTY(EditAnywhere, Category = "Scenario", meta = (ClampMin = "1.0"))
    float AcceptanceWaitSeconds = 15.0f;
    UPROPERTY(EditAnywhere, Category = "Scenario", meta = (ClampMin = "1.0"))
    float CancelWaitSeconds = 10.0f;
    void FinishRun(EScenarioRunState FinalState);
    FString GetRunStateText() const;
    void WriteRunResult();
    bool PrepareConfiguredPedestrians();
    void AccumulateTravelDistance();
    void SampleOverlapEvaluation();
};
