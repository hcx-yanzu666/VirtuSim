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

	ResetRandomStreams();
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

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	if (Parameters.ScanFrequencyHz > 0.0f)
	{
		scanElapsedSeconds += DeltaTime;
		const float scanIntervalSeconds = 1.0f / Parameters.ScanFrequencyHz;
		if (scanElapsedSeconds >= scanIntervalSeconds)
		{
			performScan();
			scanElapsedSeconds = 0.0f;
		}
	}

	// 先扫描入队，再每帧检查到期数据：0ms 新帧可以当帧发布。
	// 即使本帧不扫描，也必须检查队列，避免额外等待一个扫描周期。
	PublishReadyScans(World->GetTimeSeconds());
}

void ULidarComponent::PublishReadyScans(double CurrentTimeSeconds)
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	URosCommunicationSubsystem* RosSubsystem = World->GetSubsystem<URosCommunicationSubsystem>();
	if (RosSubsystem == nullptr)
	{
		return;
	}

	while (const FPendingLidarScan* PendingScan = PendingScans.Peek())
	{
		// 同一配置内延迟固定，队首未到期时后续帧也未到期；应用新配置时会清空队列。
		if (PendingScan->PublishAtSeconds > CurrentTimeSeconds)
		{
			break;
		}

		if (!RosSubsystem->PublishScan(PendingScan->ScanState))
		{
			break;
		}

		// 发布后再移除；Pop 后不再访问队首指针。
		PendingScans.Pop();
		--PendingScanCount;
	}
}

bool ULidarComponent::ApplyRuntimeParameters(const FLidarParameters& InParameters, FString& OutError)
{
	OutError.Reset();

	// 校验规则住在参数结构体内部，这里只负责“要么整组生效、要么完全不变”。
	if (!InParameters.Validate(OutError))
	{
		return false;
	}

	Parameters = InParameters;
	ResetRandomStreams();
	PendingScans.Empty();
	PendingScanCount = 0;
	// 扫描频率可能变了，重新开始计时，避免沿用旧频率下累计的时间。
	scanElapsedSeconds = 0.0f;
	return true;
}

void ULidarComponent::performScan()
{
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
    // 扫描起止角不单独存储，由视场角派生，避免两份数据不一致。
    ScanState.AngleMinRadians =
    FMath::DegreesToRadians(Parameters.GetAngleMinDegrees());
    ScanState.AngleMaxRadians =
    FMath::DegreesToRadians(Parameters.GetAngleMaxDegrees());
    ScanState.AngleIncrementRadians =
    FMath::DegreesToRadians(Parameters.AngleIncrementDegrees);
    // 配置使用“米”，UE 射线检测使用“厘米”。
    ScanState.RangeMinCentimeters = Parameters.RangeMinMeters * 100.0f;
    ScanState.RangeMaxCentimeters = Parameters.RangeMaxMeters * 100.0f;
	const int32 RayCount = FMath::FloorToInt(
    (ScanState.AngleMaxRadians - ScanState.AngleMinRadians) / ScanState.AngleIncrementRadians) + 1;
	for (int32 i = 0; i < RayCount; ++i)
	{
		float Angle = ScanState.AngleMinRadians + i * ScanState.AngleIncrementRadians;
		FVector LocalDirection(
			FMath::Cos(Angle),
			-FMath::Sin(Angle),
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
		float distance = ScanState.RangeMaxCentimeters;
		const bool bDropped = bHit && Parameters.DropoutProbability > 0.0f && DropoutRandomStream.FRand() < Parameters.DropoutProbability;
		if (bHit && !bDropped)
		{
			float NoiseCentimeters = 0.0f;
			if (Parameters.bNoiseEnabled && Parameters.NoiseStdDevMeters > 0.0f)
			{
				const float NoiseMeters =
					GenerateStandardNormalSample() * Parameters.NoiseStdDevMeters;
				NoiseCentimeters = NoiseMeters * 100.0f;
			}
			distance = FMath::Clamp(
            Result.Distance + NoiseCentimeters,
            ScanState.RangeMinCentimeters,
            ScanState.RangeMaxCentimeters);
		}
		ScanState.RangesCentimeters.Add(distance);

		if (Parameters.bDrawScanPoints && bHit && !bDropped)
		{
			constexpr float ScanPointSpacingCentimeters = 20.0f;
			const float PointLifetimeSeconds = (1.0f / Parameters.ScanFrequencyHz) * 1.1f;
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
					Parameters.ScanPointSizeCentimeters,
					PointLinearColor.ToFColor(true),
					false,
					PointLifetimeSeconds);
			}
		}
	}
	FPendingLidarScan PendingScan;
	PendingScan.ScanState = ScanState;
	PendingScan.PublishAtSeconds =
		ScanState.TimestampSeconds
		+ Parameters.FixedDelayMilliseconds / 1000.0;
	// ROS 长期无法发布时只保留最近的有限帧，避免内存随运行时间持续增长。
	// 正常延迟所需帧数小于上限，因此不会改变正常的固定延迟行为。
	while (PendingScanCount >= MaxPendingScanCount)
	{
		PendingScans.Pop();
		--PendingScanCount;
	}
	if (PendingScans.Enqueue(MoveTemp(PendingScan)))
	{
		++PendingScanCount;
	}
}

void ULidarComponent::ResetRandomStreams()
{
	NoiseRandomStream.Initialize(Parameters.RandomSeed);

	    // 从同一个实验种子派生出另一条独立随机序列。
    DropoutRandomStream.Initialize(
        Parameters.RandomSeed ^ 0x5A17C9E3);
}

float ULidarComponent::GenerateStandardNormalSample()
{
	// Box-Muller：两个独立的 [0,1) 均匀随机数 -> 一个标准高斯随机数。
	// U1 用于控制半径，U2 用于控制角度。
	// FRand() 的下界是闭的，U1 取到 0 会让 Log(0) 变成 -inf，
	// 所以先抬到一个极小正数，代价是把概率为 0 的那一个点挪走，不影响分布。
	const float U1 = FMath::Max(NoiseRandomStream.FRand(), UE_SMALL_NUMBER);
	const float U2 = NoiseRandomStream.FRand();
	const float Radius = FMath::Sqrt(-2.0f * FMath::Loge(U1));
	const float Theta = 2.0f * UE_PI * U2;
	// 变换同时产出 Radius*Cos 与 Radius*Sin 两个独立样本，
	// 这里只取一个，另一个直接丢弃：省下缓存状态，代价是每次多取一个随机数。
	return Radius * FMath::Cos(Theta);
}
