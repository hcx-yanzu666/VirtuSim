// Fill out your copyright notice in the Description page of Project Settings.


#include "LidarComponent.h"
#include "DrawDebugHelpers.h"
#include "../Communication/RosCommunicationSubsystem.h"

// Sets default values for this component's properties
ULidarComponent::ULidarComponent()
{
	// LiDAR 需要按固定频率采样，组件 Tick 负责累计时间并触发扫描。
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void ULidarComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void ULidarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("LiDAR 扫描失败，所属 Robot Actor 为空"));
		return;
	}

	if (scanFrequencyHz <= 0.0f)
	{
		return;
	}

	scanElapsedSeconds += DeltaTime;

	const float scanIntervalSeconds = 1.0f / scanFrequencyHz;
	if (scanElapsedSeconds < scanIntervalSeconds)
	{
		return;
	}

	performScan();
	scanElapsedSeconds = 0.0f;
}

void ULidarComponent::performScan()
{
	UE_LOG(LogTemp, Display, TEXT("LiDAR 执行扫描，频率=%.1fHz"), scanFrequencyHz);

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();

	if (OwnerActor == nullptr || World == nullptr) return;

	const FTransform RobotWorldTransform = GetOwner()->GetActorTransform();
	const FTransform BaseToLaser(
    FRotator::ZeroRotator,
    FVector(20.0f, 0.0f, 30.0f),
    FVector::OneVector
    );
	//机器人在世界中的变换 + 雷达相对机器人的固定安装位姿 = 雷达在世界中的变换
	const FTransform LaserWorldTransform = BaseToLaser * RobotWorldTransform;
	const FVector Start = LaserWorldTransform.GetLocation();
	
	ScanState.TimestampSeconds = World->GetTimeSeconds();
	ScanState.RangesCentimeters.Empty();

	FCollisionQueryParams Params;
    Params.AddIgnoredActor(OwnerActor);
    // UE 配置使用“度”，LaserScan 使用“弧度”。
    ScanState.AngleMinRadians =
    FMath::DegreesToRadians(angleMinDegrees);
    ScanState.AngleMaxRadians =
    FMath::DegreesToRadians(angleMaxDegrees);
    ScanState.AngleIncrementRadians =
    FMath::DegreesToRadians(angleIncrementDegrees);
    // 配置使用“米”，UE 射线检测使用“厘米”。
    ScanState.RangeMinCentimeters = rangeMinMeters * 100.0f;
    ScanState.RangeMaxCentimeters = rangeMaxMeters * 100.0f;
	const int32 RayCount = FMath::FloorToInt(
    (ScanState.AngleMaxRadians - ScanState.AngleMinRadians) / ScanState.AngleIncrementRadians) + 1;
	for (int32 i = 0; i < RayCount; ++i)
	{
		float Angle = ScanState.AngleMinRadians + i * ScanState.AngleIncrementRadians;
		FVector LocalDirection(
			FMath::Cos(Angle),
			FMath::Sin(Angle),
			0.0f
		);
		//把 LiDAR 自己坐标系中的方向 -> 世界坐标系中的方向
		//局部坐标系相对于世界的 Transform . TransformVectorNoScale (局部坐标系的朝向)  然后 得到LIDAR 方向 -> 转换成世界坐标系中的方向
		FVector WorldDirection = LaserWorldTransform.TransformVectorNoScale(LocalDirection).GetSafeNormal();
		const FVector End = Start + WorldDirection * ScanState.RangeMaxCentimeters;
		FHitResult Result;
		bool bHit = World->LineTraceSingleByChannel(
			Result,
			Start,
			End,
			ECC_Visibility,
			Params
		);
		//hit 容器 存储 Hit.Location
         //Hit.Distance
		//Hit.GetActor()
		//Hit.Normal
		float distance;
		if (bHit)
		{
			distance = FMath::Clamp(
            Result.Distance,
            ScanState.RangeMinCentimeters,
            ScanState.RangeMaxCentimeters);
		}
		else
		{
			distance = ScanState.RangeMaxCentimeters;
		}
		ScanState.RangesCentimeters.Add(distance);

		if (bDrawScanPoints)
		{
			constexpr float ScanPointSpacingCentimeters = 20.0f;
			constexpr float ScanPointSizeCentimeters = 5.0f;
			const float PointLifetimeSeconds = (1.0f / scanFrequencyHz) * 1.1f;
			const int32 PointCount = FMath::Max(
				FMath::CeilToInt(distance / ScanPointSpacingCentimeters),
				1);

			// 每条射线只画点，不画线；点从雷达起点排列到本次测距终点。
			for (int32 PointIndex = 1; PointIndex <= PointCount; ++PointIndex)
			{
				const float PointDistance = FMath::Min(
					PointIndex * ScanPointSpacingCentimeters,
					distance);
				const FVector PointLocation = Start + WorldDirection * PointDistance;

				// 根据点到 LiDAR 的相对距离着色：近红、中蓝、远青。
				const float RangeSpan = FMath::Max(
					ScanState.RangeMaxCentimeters - ScanState.RangeMinCentimeters,
					UE_SMALL_NUMBER);
				const float DistanceRatio = FMath::Clamp(
					(PointDistance - ScanState.RangeMinCentimeters) /
					RangeSpan,
					0.0f,
					1.0f);
				const FLinearColor NearColor = FLinearColor::Red;
				const FLinearColor MidColor(0.05f, 0.25f, 1.0f, 1.0f);
				const FLinearColor FarColor(0.0f, 1.0f, 1.0f, 1.0f);
				const FLinearColor PointLinearColor = DistanceRatio < 0.5f
					? FMath::Lerp(NearColor, MidColor, DistanceRatio * 2.0f)
					: FMath::Lerp(MidColor, FarColor, (DistanceRatio - 0.5f) * 2.0f);

				DrawDebugPoint(
					World,
					PointLocation,
					ScanPointSizeCentimeters,
					PointLinearColor.ToFColor(true),
					false,
					PointLifetimeSeconds);
			}
		}
	}

	if (URosCommunicationSubsystem* RosSubsystem = World->GetSubsystem<URosCommunicationSubsystem>())
	{
		RosSubsystem->PublishScan(ScanState);
	}
}
