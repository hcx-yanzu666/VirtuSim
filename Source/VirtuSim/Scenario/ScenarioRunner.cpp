#include "ScenarioRunner.h"

#include "ScenarioLoader.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "../Robot/LidarComponent.h"
#include "../Communication/RosCommunicationSubsystem.h"

AScenarioRunner::AScenarioRunner()
{
    PrimaryActorTick.bCanEverTick = true;
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

    ULidarComponent* LidarComponent =
        RobotActor->FindComponentByClass<ULidarComponent>();
    if (LidarComponent == nullptr)
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("机器人缺少 ULidarComponent：Actor=%s"),
            *RobotActor->GetName());
        return;
    }

    FString LidarError;
    if (!LidarComponent->ApplyRuntimeParameters(
            Robot.LidarParameters,
            LidarError))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("应用场景 LiDAR 参数失败：%s"),
            *LidarError);
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("已应用场景 LiDAR 参数：Frequency=%.1fHz Range=%.1f-%.1fm Seed=%d"),
        Robot.LidarParameters.ScanFrequencyHz,
        Robot.LidarParameters.RangeMinMeters,
        Robot.LidarParameters.RangeMaxMeters,
        Robot.LidarParameters.RandomSeed);

    URosCommunicationSubsystem* RosSubsystem =
        GetWorld()->GetSubsystem<URosCommunicationSubsystem>();
    if (RosSubsystem == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("找不到 ROS 通信子系统，无法发布场景导航目标"));
        return;
    }

    if (!RosSubsystem->PublishNavigationGoal(
            Robot.Goal.LocationCentimeters,
            Robot.Goal.YawDegrees))
    {
        UE_LOG(
            LogTemp,
            Error,
            TEXT("场景导航目标发布失败：Location=%s，Yaw=%.1f"),
            *Robot.Goal.LocationCentimeters.ToString(),
            Robot.Goal.YawDegrees);
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("已发布场景导航目标：Location=%s，Yaw=%.1f"),
        *Robot.Goal.LocationCentimeters.ToString(),
        Robot.Goal.YawDegrees);

    RunState = EScenarioRunState::Navigating;
    RunElapsedSeconds = 0.0f;
    UE_LOG(LogTemp, Display, TEXT("场景运行状态：Navigating"));
}

void AScenarioRunner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (RunState != EScenarioRunState::Navigating)
    {
        return;
    }

    RunElapsedSeconds += DeltaSeconds;

    if (RunElapsedSeconds >= Scenario.TimeoutSeconds)
    {
        URosCommunicationSubsystem* RosSubsystem =
            GetWorld() ? GetWorld()->GetSubsystem<URosCommunicationSubsystem>() : nullptr;

        if (RosSubsystem != nullptr)
        {
            RosSubsystem->CancelNavigation();
        }

        FinishRun(EScenarioRunState::TimeOut);
        return;
    }

    UpdateNavigationState(DeltaSeconds);
}

void AScenarioRunner::StartNavigationRun()
{
}

void AScenarioRunner::UpdateNavigationState(float DeltaSeconds)
{
    // 当前阶段只读取 Bridge 回传的终态；DeltaSeconds 暂留给后续超时计时使用。
    (void)DeltaSeconds;

    if (GetWorld() == nullptr)
    {
        return;
    }

    const URosCommunicationSubsystem* RosSubsystem =
        GetWorld()->GetSubsystem<URosCommunicationSubsystem>();
    if (RosSubsystem == nullptr)
    {
        return;
    }

    const FString NavigationStatus = RosSubsystem->GetNavigationStatus();
    if (NavigationStatus == TEXT("Succeeded"))
    {
        FinishRun(EScenarioRunState::Succeeded);
    }
    else if (NavigationStatus == TEXT("Failed"))
    {
        FinishRun(EScenarioRunState::Failed);
    }
    else if (NavigationStatus == TEXT("Canceled"))
    {
        FinishRun(EScenarioRunState::Canceled);
    }
}

void AScenarioRunner::FinishRun(EScenarioRunState FinalState)
{
    if (RunState != EScenarioRunState::Navigating)
    {
        return;
    }

    RunState = FinalState;

    const TCHAR* StateText = TEXT("Unknown");
    switch (RunState)
    {
    case EScenarioRunState::Succeeded:
        StateText = TEXT("Succeeded");
        break;
    case EScenarioRunState::Failed:
        StateText = TEXT("Failed");
        break;
    case EScenarioRunState::Canceled:
        StateText = TEXT("Canceled");
        break;
    case EScenarioRunState::TimeOut:
        StateText = TEXT("TimeOut");
        break;
    default:
        break;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("场景运行结束：Scenario=%s，State=%s，Elapsed=%.2fs"),
        *Scenario.ScenarioId,
        StateText,
        RunElapsedSeconds);
}
