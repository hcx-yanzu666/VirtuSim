#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioDefinition.h"
#include "ScenarioRunResult.h"
#include "ScenarioRunner.generated.h"  // 必须是最后一个 include


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
};
