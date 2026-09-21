#include "ScenarioLoader.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
// 按固定格式读取位姿，保留 UE 厘米 / 度。
bool ReadPose(const FJsonObject& Object, FScenarioPose& Pose)
{
    return Object.TryGetNumberField(TEXT("x"), Pose.LocationCentimeters.X)
        && Object.TryGetNumberField(TEXT("y"), Pose.LocationCentimeters.Y)
        && Object.TryGetNumberField(TEXT("z"), Pose.LocationCentimeters.Z)
        && Object.TryGetNumberField(TEXT("yaw"), Pose.YawDegrees);
}

bool ReadLidar(const FJsonObject& Object, FLidarParameters& Parameters)
{
    // 这里只做字段映射，后续应用参数时复用组件自己的校验。
    return Object.TryGetNumberField(TEXT("scan_frequency_hz"), Parameters.ScanFrequencyHz)
        && Object.TryGetNumberField(TEXT("angle_increment_degrees"), Parameters.AngleIncrementDegrees)
        && Object.TryGetNumberField(TEXT("horizontal_fov_degrees"), Parameters.HorizontalFovDegrees)
        && Object.TryGetNumberField(TEXT("range_min_meters"), Parameters.RangeMinMeters)
        && Object.TryGetNumberField(TEXT("range_max_meters"), Parameters.RangeMaxMeters)
        && Object.TryGetBoolField(TEXT("noise_enabled"), Parameters.bNoiseEnabled)
        && Object.TryGetNumberField(TEXT("noise_std_dev_meters"), Parameters.NoiseStdDevMeters)
        && Object.TryGetNumberField(TEXT("dropout_probability"), Parameters.DropoutProbability)
        && Object.TryGetNumberField(TEXT("fixed_delay_milliseconds"), Parameters.FixedDelayMilliseconds)
        && Object.TryGetNumberField(TEXT("random_seed"), Parameters.RandomSeed);
}
}

bool FScenarioLoader::LoadFromFile(const FString& FilePath, FScenarioDefinition& OutScenario, FString& OutError)
{
    OutError.Reset();
    FString JsonText;
    if (!FFileHelper::LoadFileToString(JsonText, *FilePath))
    {
        OutError = FString::Printf(TEXT("读取场景文件失败：%s"), *FilePath);
        return false;
    }

    const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
    TSharedPtr<FJsonObject> Root;
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        OutError = TEXT("场景 JSON 解析失败");
        return false;
    }

    // 先填临时对象，成功后再输出，避免半份配置覆盖旧数据。
    FScenarioDefinition Scenario;
    if (!Root->TryGetNumberField(TEXT("schema_version"), Scenario.SchemaVersion)
        || !Root->TryGetStringField(TEXT("scenario_id"), Scenario.ScenarioId)
        || !Root->TryGetStringField(TEXT("coordinate_system"), Scenario.CoordinateSystem)
        || !Root->TryGetNumberField(TEXT("timeout_seconds"), Scenario.TimeoutSeconds))
    {
        OutError = TEXT("场景基本字段读取失败，请参考 warehouse_baseline.json");
        return false;
    }
    Root->TryGetStringField(TEXT("description"), Scenario.Description);

    const TArray<TSharedPtr<FJsonValue>>* Robots = nullptr;
    if (!Root->TryGetArrayField(TEXT("robots"), Robots) || Robots->Num() != 1)
    {
        OutError = TEXT("当前 Demo 的 robots 数组需要包含一个机器人");
        return false;
    }

    const TSharedPtr<FJsonObject>* RobotObject = nullptr;
    if (!(*Robots)[0].IsValid() || !(*Robots)[0]->TryGetObject(RobotObject)
        || !RobotObject->IsValid())
    {
        OutError = TEXT("机器人配置不是 JSON 对象");
        return false;
    }

    FScenarioRobotDefinition Robot;
    const FJsonObject& Object = **RobotObject;
    const TSharedPtr<FJsonObject>* Start = nullptr;
    const TSharedPtr<FJsonObject>* Goal = nullptr;
    const TSharedPtr<FJsonObject>* Lidar = nullptr;
    if (!Object.TryGetStringField(TEXT("id"), Robot.Id)
        || !Object.TryGetStringField(TEXT("actor_tag"), Robot.ActorTag)
        || !Object.TryGetObjectField(TEXT("start"), Start) || !Start->IsValid()
        || !Object.TryGetObjectField(TEXT("goal"), Goal) || !Goal->IsValid()
        || !Object.TryGetObjectField(TEXT("lidar"), Lidar) || !Lidar->IsValid()
        || !ReadPose(**Start, Robot.Start)
        || !ReadPose(**Goal, Robot.Goal)
        || !ReadLidar(**Lidar, Robot.LidarParameters))
    {
        OutError = TEXT("机器人字段读取失败，请参考 warehouse_baseline.json");
        return false;
    }

    Scenario.Robots.Add(MoveTemp(Robot));
    OutScenario = MoveTemp(Scenario);
    return true;
}
