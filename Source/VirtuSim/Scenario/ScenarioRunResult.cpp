#include "ScenarioRunResult.h"

#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace
{
TSharedRef<FJsonObject> MakeVectorObject(const FVector& Value, float YawDegrees)
{
    TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetNumberField(TEXT("x"), Value.X);
    Object->SetNumberField(TEXT("y"), Value.Y);
    Object->SetNumberField(TEXT("z"), Value.Z);
    Object->SetNumberField(TEXT("yaw"), YawDegrees);
    return Object;
}

TSharedRef<FJsonObject> MakeLidarObject(const FLidarParameters& Parameters)
{
    TSharedRef<FJsonObject> Object = MakeShared<FJsonObject>();
    Object->SetNumberField(TEXT("scan_frequency_hz"), Parameters.ScanFrequencyHz);
    Object->SetNumberField(TEXT("angle_increment_degrees"), Parameters.AngleIncrementDegrees);
    Object->SetNumberField(TEXT("horizontal_fov_degrees"), Parameters.HorizontalFovDegrees);
    Object->SetNumberField(TEXT("range_min_meters"), Parameters.RangeMinMeters);
    Object->SetNumberField(TEXT("range_max_meters"), Parameters.RangeMaxMeters);
    Object->SetBoolField(TEXT("noise_enabled"), Parameters.bNoiseEnabled);
    Object->SetNumberField(TEXT("noise_std_dev_meters"), Parameters.NoiseStdDevMeters);
    Object->SetNumberField(TEXT("dropout_probability"), Parameters.DropoutProbability);
    Object->SetNumberField(TEXT("fixed_delay_milliseconds"), Parameters.FixedDelayMilliseconds);
    Object->SetNumberField(TEXT("random_seed"), Parameters.RandomSeed);
    return Object;
}
}

bool FScenarioRunResultWriter::WriteJson(
    const FString& FilePath,
    const FScenarioRunResult& Result,
    FString& OutError)
{
    OutError.Reset();
    TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
    Root->SetStringField(TEXT("scenario_id"), Result.ScenarioId);
    Root->SetStringField(TEXT("run_id"), Result.RunId);
    Root->SetStringField(TEXT("state"), Result.State);
    Root->SetBoolField(TEXT("task_ended_confirmed"), Result.bTaskEndedConfirmed);
    Root->SetStringField(TEXT("detail"), Result.Detail);
    Root->SetStringField(TEXT("elapsed_time_basis"), TEXT("simulation_seconds_after_acceptance"));
    Root->SetNumberField(TEXT("elapsed_seconds"), Result.ElapsedSeconds);
    if (Result.bTravelDistanceMeasured)
    {
        Root->SetNumberField(TEXT("travel_distance_meters"), Result.TravelDistanceMeters);
    }
    Root->SetObjectField(
        TEXT("goal"),
        MakeVectorObject(Result.GoalLocationCentimeters, Result.GoalYawDegrees));
    Root->SetObjectField(TEXT("lidar"), MakeLidarObject(Result.LidarParameters));

    FString JsonText;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonText);
    if (!FJsonSerializer::Serialize(Root, Writer))
    {
        OutError = TEXT("序列化场景运行结果失败");
        return false;
    }
    Writer->Close();

    if (!FFileHelper::SaveStringToFile(JsonText, *FilePath))
    {
        OutError = FString::Printf(TEXT("保存场景运行结果失败：%s"), *FilePath);
        return false;
    }
    return true;
}
