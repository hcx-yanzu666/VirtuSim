#include "ScenarioRunner.h"

#include "ScenarioLoader.h"
#include "Misc/Paths.h"
#include "Kismet/GameplayStatics.h"
#include "../Robot/LidarComponent.h"
#include "../Communication/RosCommunicationSubsystem.h"
#include "Misc/Guid.h"
#include "HAL/PlatformTime.h"

AScenarioRunner::AScenarioRunner()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AScenarioRunner::BeginPlay()
{
    Super::BeginPlay();
    RunState = EScenarioRunState::Initializing;
    RunId = FGuid::NewGuid().ToString(EGuidFormats::DigitsWithHyphens);

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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
        return;
    }

    StatusDelegateHandle = RosSubsystem->OnNavigationStatusReceived.AddUObject(
        this, &AScenarioRunner::HandleNavigationStatus);
    RunState = EScenarioRunState::WaitingForAcceptance;
    WaitStartedWallSeconds = FPlatformTime::Seconds();

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
        ResultDetail = TEXT("Initialization failed; see preceding UE log");
        FinishRun(EScenarioRunState::SetupFailed);
        return;
    }

    UE_LOG(
        LogTemp,
        Display,
        TEXT("已发布场景导航目标：Location=%s，Yaw=%.1f"),
        *Robot.Goal.LocationCentimeters.ToString(),
        Robot.Goal.YawDegrees);

    UE_LOG(LogTemp, Display, TEXT("场景运行状态：WaitingForAcceptance"));
}

void AScenarioRunner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bResultWritten) return;

    const double WaitSeconds = FPlatformTime::Seconds() - WaitStartedWallSeconds;
    if (RunState == EScenarioRunState::Navigating)
    {
        RunElapsedSeconds += DeltaSeconds;
        if (RunElapsedSeconds >= Scenario.TimeoutSeconds)
        {
            RequestRunCancel(EScenarioRunState::TimeOut);
        }
    }
    else if (RunState == EScenarioRunState::WaitingForAcceptance && WaitSeconds >= AcceptanceWaitSeconds)
    {
        // 请求可能已经送达，等待超时后仍需请求取消。
        RequestRunCancel(EScenarioRunState::AcceptanceTimedOut);
    }
    else if (RunState == EScenarioRunState::WaitingForCancel && WaitSeconds >= CancelWaitSeconds)
    {
        ResultDetail += TEXT("; cancellation/result confirmation timed out; task may still be active");
        FinishRun(PendingFinalState);
    }
}

void AScenarioRunner::RequestRunCancel(EScenarioRunState Reason)
{
    PendingFinalState = Reason;
    RunState = EScenarioRunState::WaitingForCancel;
    WaitStartedWallSeconds = FPlatformTime::Seconds();
    URosCommunicationSubsystem* Ros = GetWorld()->GetSubsystem<URosCommunicationSubsystem>();
    const bool bSent = Ros && Ros->CancelNavigation();
    ResultDetail = bSent ? TEXT("Cancellation requested; awaiting terminal result")
                        : TEXT("Cancellation request could not be published");
    UE_LOG(LogTemp, Warning, TEXT("场景运行状态：WaitingForCancel，%s"), *ResultDetail);
}

void AScenarioRunner::HandleNavigationStatus(const FString& Status)
{
    if (bResultWritten) return;
    // 只消费新的通知，不读取上一轮缓存状态；当前仍要求独占单任务。
    if (RunState == EScenarioRunState::WaitingForAcceptance &&
        (Status == TEXT("GoalAccepted") || Status == TEXT("Navigating")))
    {
        RunState = EScenarioRunState::Navigating;
        RunElapsedSeconds = 0.0f;
        UE_LOG(LogTemp, Display, TEXT("场景运行状态：Navigating（已接受，开始计时）"));
    }

    if (Status == TEXT("Succeeded") || Status == TEXT("Failed") || Status == TEXT("Canceled"))
    {
        bTaskEndedConfirmed = true;
        ResultDetail = FString::Printf(TEXT("Bridge terminal status: %s"), *Status);
        if (RunState == EScenarioRunState::WaitingForCancel &&
            (PendingFinalState == EScenarioRunState::TimeOut ||
             PendingFinalState == EScenarioRunState::AcceptanceTimedOut))
        {
            // 保留超时原因，不被晚到的 Canceled/Succeeded 覆盖。
            FinishRun(PendingFinalState);
        }
        else
        {
            FinishRun(Status == TEXT("Succeeded") ? EScenarioRunState::Succeeded :
                Status == TEXT("Canceled") ? EScenarioRunState::Canceled : EScenarioRunState::Failed);
        }
    }
    else if (Status == TEXT("Canceling") && RunState != EScenarioRunState::WaitingForCancel)
    {
        // 用户主动取消也需要等待最终结果。
        PendingFinalState = EScenarioRunState::Canceled;
        RunState = EScenarioRunState::WaitingForCancel;
        WaitStartedWallSeconds = FPlatformTime::Seconds();
    }
    else if (Status == TEXT("CancelFailed"))
    {
        ResultDetail = TEXT("Bridge could not confirm cancellation/task state");
        if (RunState != EScenarioRunState::WaitingForCancel)
        {
            RequestRunCancel(EScenarioRunState::Failed);
        }
        // 不刷新等待起点，避免无限等待。
    }
}

void AScenarioRunner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (URosCommunicationSubsystem* Ros = GetWorld()->GetSubsystem<URosCommunicationSubsystem>())
    {
        Ros->OnNavigationStatusReceived.Remove(StatusDelegateHandle);
        if (!bResultWritten && (RunState == EScenarioRunState::Navigating ||
            RunState == EScenarioRunState::WaitingForAcceptance || RunState == EScenarioRunState::WaitingForCancel))
        {
            Ros->CancelNavigation();
            ResultDetail = TEXT("UE EndPlay; cancellation unconfirmed");
            FinishRun(EScenarioRunState::Interrupted);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AScenarioRunner::FinishRun(EScenarioRunState FinalState)
{
    if (bResultWritten)
    {
        return;
    }

    bResultWritten = true;
    RunState = FinalState;

    const FString StateText = GetRunStateText();

    UE_LOG(
        LogTemp,
        Display,
        TEXT("场景运行结束：Scenario=%s，State=%s，Elapsed=%.2fs"),
        *Scenario.ScenarioId,
        *StateText,
        RunElapsedSeconds);

    WriteRunResult();
}

FString AScenarioRunner::GetRunStateText() const
{
    switch (RunState)
    {
    case EScenarioRunState::Succeeded: return TEXT("Succeeded");
    case EScenarioRunState::Failed: return TEXT("Failed");
    case EScenarioRunState::Canceled: return TEXT("Canceled");
    case EScenarioRunState::TimeOut: return TEXT("TimeOut");
    case EScenarioRunState::SetupFailed: return TEXT("SetupFailed");
    case EScenarioRunState::AcceptanceTimedOut: return TEXT("AcceptanceTimedOut");
    case EScenarioRunState::Interrupted: return TEXT("Interrupted");
    default: return TEXT("Unknown");
    }
}

void AScenarioRunner::WriteRunResult()
{
    // 配置读取失败也保留初始化失败报告；空字段不代表真实测量值。
    const FScenarioRobotDefinition Robot = Scenario.Robots.IsValidIndex(0)
        ? Scenario.Robots[0] : FScenarioRobotDefinition();
    FScenarioRunResult Result;
    Result.ScenarioId = Scenario.ScenarioId;
    Result.RunId = RunId;
    Result.State = GetRunStateText();
    Result.bTaskEndedConfirmed = bTaskEndedConfirmed;
    Result.Detail = ResultDetail;
    Result.ElapsedSeconds = RunElapsedSeconds;
    Result.GoalLocationCentimeters = Robot.Goal.LocationCentimeters;
    Result.GoalYawDegrees = Robot.Goal.YawDegrees;
    Result.LidarParameters = Robot.LidarParameters;

    const FString Directory = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("ScenarioResults"));
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString FilePath = FPaths::Combine(
        Directory,
        FString::Printf(TEXT("%s_%s.json"), *Scenario.ScenarioId, *RunId));

    FString Error;
    if (!FScenarioRunResultWriter::WriteJson(FilePath, Result, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("场景结果写入失败：%s"), *Error);
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("场景结果已写入：%s"), *FilePath);
}
