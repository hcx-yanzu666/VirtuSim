#include "ScenarioRunner.h"

#include "ScenarioLoader.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"

AScenarioRunner::AScenarioRunner()
{
    // 当前只在 BeginPlay 加载一次，不需要每帧更新。
    PrimaryActorTick.bCanEverTick = false;
}

void AScenarioRunner::BeginPlay()
{
    Super::BeginPlay();

    // 拼接项目 Config 目录 / Scenarios / ScenarioFileName。
    const FString FilePath = FPaths::Combine(
        FPaths::ProjectConfigDir(),
        TEXT("Scenarios"),
        ScenarioFileName);

    // 加载器成功后直接写入成员 Scenario。
    FString Error;
    const bool bLoaded = FScenarioLoader::LoadFromFile(FilePath, Scenario, Error);

    if (!bLoaded)
    {
        UE_LOG(LogTemp, Error, TEXT("加载失败，原因：%s"), *Error);
        return;
    }

    // 加载器成功时保证数组里只有一个机器人。
    UE_LOG(LogTemp, Display, TEXT("场景加载结果：ScenarioId=%s，Robots.Num=%d"),
        *Scenario.ScenarioId,
        Scenario.Robots.Num());

    if (!Scenario.Robots.IsValidIndex(0))
    {
        UE_LOG(LogTemp, Error, TEXT("场景加载成功，但 robots[0] 不存在，数量=%d"),
            Scenario.Robots.Num());
        return;
    }

    const FScenarioRobotDefinition& Robot = Scenario.Robots[0];

    UE_LOG(LogTemp, Display, TEXT("场景：%s，机器人：%s，标签：%s"),
        *Scenario.ScenarioId, *Robot.Id, *Robot.ActorTag);

    UE_LOG(LogTemp, Display, TEXT("起点：%s，朝向：%.1f 度"),
        *Robot.Start.LocationCentimeters.ToString(),
        Robot.Start.YawDegrees);

    UE_LOG(LogTemp, Display, TEXT("目标：%s，朝向：%.1f 度"),
        *Robot.Goal.LocationCentimeters.ToString(),
        Robot.Goal.YawDegrees);

    UE_LOG(LogTemp, Display, TEXT("扫描频率：%.1f Hz"),
        Robot.LidarParameters.ScanFrequencyHz);

    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClassWithTag(
        GetWorld(),
        AActor::StaticClass(),
        FName(*Robot.ActorTag),
        FoundActors);

    if (FoundActors.Num() != 1)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("机器人标签匹配数量错误：Tag=%s，Count=%d"),
            *Robot.ActorTag,
            FoundActors.Num());
        return;
    }

    AActor* RobotActor = FoundActors[0];

    RobotActor->SetActorLocationAndRotation(
        Robot.Start.LocationCentimeters,
        FRotator(0.0f, Robot.Start.YawDegrees, 0.0f));
    UE_LOG(
        LogTemp,
        Display,
        TEXT("已应用机器人起点：Actor=%s，Location=%s，Yaw=%.1f"),
        *RobotActor->GetName(),
        *Robot.Start.LocationCentimeters.ToString(),
        Robot.Start.YawDegrees);
}
