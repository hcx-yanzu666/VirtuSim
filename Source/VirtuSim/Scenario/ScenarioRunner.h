#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioDefinition.h"
#include "ScenarioRunner.generated.h"  // 必须是最后一个 include

UCLASS()
class VIRTUSIM_API AScenarioRunner : public AActor
{
    GENERATED_BODY()

public:
    AScenarioRunner();

protected:
    virtual void BeginPlay() override;

private:
    // 编辑器中设置场景文件名。
    UPROPERTY(EditAnywhere, Category = "Scenario")
    FString ScenarioFileName = TEXT("warehouse_baseline.json");

    // 保存加载后的配置。
    FScenarioDefinition Scenario;
};