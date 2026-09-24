#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioDefinition.h"
#include "ScenarioRunner.generated.h"  // 必须是最后一个 include


UENUM()
enum class EScenarioRunState : uint8
{
    Idle,
    Initializing,
    Navigating,
    Succeeded,
    Failed,
    Canceled,
    TimeOut
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
private:
    // 编辑器中设置场景文件名。
    UPROPERTY(EditAnywhere, Category = "Scenario")
    FString ScenarioFileName = TEXT("warehouse_baseline.json");

    // 保存加载后的配置。
    FScenarioDefinition Scenario;

    EScenarioRunState RunState = EScenarioRunState::Idle;
    float RunElapsedSeconds = 0.0f;
    FString RunId;

    void StartNavigationRun();
    void UpdateNavigationState(float DeltaSeconds);
    void FinishRun(EScenarioRunState FinalState);
};