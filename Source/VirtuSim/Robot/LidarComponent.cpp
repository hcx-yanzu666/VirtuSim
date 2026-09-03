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
			distance = Result.Distance;
		}
		else
		{
			distance = ScanState.RangeMaxCentimeters;
		}
		ScanState.RangesCentimeters.Add(distance);

		const FVector DrawEnd = bHit ? Result.ImpactPoint : End;
		const FColor DrawColor = bHit ? FColor::Green : FColor::Red;

		DrawDebugLine(
			World,
			Start,
			DrawEnd,
			DrawColor,
			false,
			0.1f,
			0,
			1.0f
		);
	}

	if (URosCommunicationSubsystem* RosSubsystem = World->GetSubsystem<URosCommunicationSubsystem>())
	{
		RosSubsystem->PublishScan(ScanState);
	}
}
