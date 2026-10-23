// Fill out your copyright notice in the Description page of Project Settings.

#include "DynamicPedestrianObstacle.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "ScenarioDefinition.h"

namespace
{
// 固定行为参数；场景 JSON 只负责身份与路线，不改变行人行为。
constexpr float MoveSpeedCmPerSecond = 100.0f;
constexpr float YieldDistanceCm = 180.0f;
constexpr float FixedResumeDistanceCm = 260.0f;
constexpr float ResumeClearSeconds = 0.5f;
constexpr float RearAllowanceCm = 50.0f;
}

ADynamicPedestrianObstacle::ADynamicPedestrianObstacle()
{
	PrimaryActorTick.bCanEverTick = true;

	CollisionCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
	RootComponent = CollisionCapsule;
	CollisionCapsule->InitCapsuleSize(34.0f, 88.0f);
	CollisionCapsule->SetMobility(EComponentMobility::Movable);
	CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionCapsule->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
	// ULidarComponent 使用 ECC_Visibility 射线，因此必须显式阻挡这个通道。
	CollisionCapsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	CollisionCapsule->SetGenerateOverlapEvents(false);
	// Nav2 通过 /scan 更新代价地图，无需让这个代理修改 UE 自身的 NavMesh。
	CollisionCapsule->SetCanEverAffectNavigation(false);

	PedestrianMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PedestrianMesh"));
	PedestrianMesh->SetupAttachment(CollisionCapsule);
	PedestrianMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PedestrianMesh->SetRelativeLocation(MeshRelativeLocation);
	PedestrianMesh->SetRelativeRotation(MeshRelativeRotation);
	PedestrianMesh->SetMobility(EComponentMobility::Movable);
	PedestrianMesh->SetCanEverAffectNavigation(false);
	PedestrianMesh->bPauseAnims = true;
}

void ADynamicPedestrianObstacle::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (CollisionCapsule != nullptr)
	{
		CollisionCapsule->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		CollisionCapsule->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionCapsule->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	}

	ApplyMeshAsset();
}

void ADynamicPedestrianObstacle::BeginPlay()
{
	Super::BeginPlay();

	// Runner 可能先于行人 BeginPlay 应用路线；保留等待或已启动状态，
	// 不覆盖路线，也不在这里提前开跑。
	if (bScenarioControlled)
	{
		SetPedestrianAnimationPaused(bStopped || bYieldingToRobot);
		return;
	}

	// 没有场景 JSON 控制时，保留旧蓝图实例的自动往返行为。
	StartLocation = GetActorLocation();
	EndLocation = StartLocation + EndOffset;
	ResumeClearElapsed = 0.0f;
	bMovingToEnd = true;
	bStopped = false;
	bYieldingToRobot = false;
	FindRobotActor();

	if (EndOffset.IsNearlyZero())
	{
		bStopped = true;
		SetPedestrianAnimationPaused(true);
		UE_LOG(LogTemp, Warning, TEXT("DynamicPedestrianObstacle 的 EndOffset 为零，行人不会移动。"));
	}
	else
	{
		SetPedestrianAnimationPaused(false);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("动态行人已启动：起点=%s，终点=%s，速度=%.1fcm/s"),
		*StartLocation.ToString(),
		*EndLocation.ToString(),
		MoveSpeedCmPerSecond);
}

void ADynamicPedestrianObstacle::ApplyScenarioParameters(
	const FScenarioPedestrianDefinition& Definition)
{
	bScenarioControlled = true;
	StartLocation = Definition.Start.LocationCentimeters;
	EndLocation = Definition.End.LocationCentimeters;
	ResetToStart();

	UE_LOG(LogTemp, Display,
		TEXT("已应用行人场景路线：Id=%s，起点=%s，终点=%s，等待导航开始"),
		*Definition.Id, *StartLocation.ToString(), *EndLocation.ToString());
}

void ADynamicPedestrianObstacle::ResetToStart()
{
	ResumeClearElapsed = 0.0f;
	bMovingToEnd = true;
	bYieldingToRobot = false;
	bStopped = true;
	SetPedestrianAnimationPaused(true);
	SetActorLocation(StartLocation, false);

	const FVector TravelDirection = (EndLocation - StartLocation).GetSafeNormal2D();
	if (!TravelDirection.IsNearlyZero())
	{
		SetActorRotation(FRotator(0.0f, TravelDirection.Rotation().Yaw, 0.0f));
	}
}

void ADynamicPedestrianObstacle::StartMovement()
{
	if (EndLocation.Equals(StartLocation, 0.1f))
	{
		bStopped = true;
		SetPedestrianAnimationPaused(true);
		return;
	}

	FindRobotActor();
	bStopped = false;
	SetPedestrianAnimationPaused(false);
	UE_LOG(LogTemp, Display, TEXT("动态行人开始移动。"));
}

void ADynamicPedestrianObstacle::FindRobotActor()
{
	RobotActor.Reset();

	if (RobotActorTag.IsNone() || GetWorld() == nullptr)
	{
		return;
	}

	TArray<AActor*> TaggedActors;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), RobotActorTag, TaggedActors);
	if (TaggedActors.Num() > 0 && IsValid(TaggedActors[0]))
	{
		RobotActor = TaggedActors[0];
		UE_LOG(LogTemp, Display, TEXT("动态行人已绑定机器人：标签=%s，Actor=%s"), *RobotActorTag.ToString(), *TaggedActors[0]->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("动态行人未找到机器人标签：%s，将按原路线移动。"), *RobotActorTag.ToString());
	}
}

bool ADynamicPedestrianObstacle::IsRobotBlockingPath(
	const FVector& CurrentLocation,
	const FVector& TravelDirection,
	float DistanceThresholdCm) const
{
	const AActor* TrackedRobot = RobotActor.Get();
	if (!IsValid(TrackedRobot) || DistanceThresholdCm <= 0.0f)
	{
		return false;
	}

	FVector ToRobot = TrackedRobot->GetActorLocation() - CurrentLocation;
	ToRobot.Z = 0.0f;
	const float DistanceCm = ToRobot.Size();
	if (DistanceCm > DistanceThresholdCm)
	{
		return false;
	}

	// 机器人在行人身后时不触发让行；侧向横穿和前方相遇都会通过这个判断。
	const float ForwardDistanceCm = FVector::DotProduct(ToRobot, TravelDirection);
	return ForwardDistanceCm >= -RearAllowanceCm;
}

void ADynamicPedestrianObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bStopped)
	{
		// 每帧重新确认停止状态，避免蓝图或动画蓝图把暂停标志改回去。
		SetPedestrianAnimationPaused(true);
		return;
	}

	if (DeltaSeconds <= 0.0f)
	{
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = bMovingToEnd ? EndLocation : StartLocation;
	const FVector TravelDirection = (TargetLocation - CurrentLocation).GetSafeNormal();

	if (!bYieldingToRobot
		&& IsRobotBlockingPath(CurrentLocation, TravelDirection, YieldDistanceCm))
	{
		bYieldingToRobot = true;
		ResumeClearElapsed = 0.0f;
		SetPedestrianAnimationPaused(true);
		UE_LOG(LogTemp, Display, TEXT("动态行人前方检测到机器人，停止等待：距离约 %.1fcm"),
			FVector::Dist2D(CurrentLocation, RobotActor->GetActorLocation()));
		return;
	}

	if (bYieldingToRobot)
	{
		if (IsRobotBlockingPath(CurrentLocation, TravelDirection, FixedResumeDistanceCm))
		{
			ResumeClearElapsed = 0.0f;
			return;
		}

		ResumeClearElapsed += DeltaSeconds;
		if (ResumeClearElapsed < ResumeClearSeconds)
		{
			return;
		}

		bYieldingToRobot = false;
		ResumeClearElapsed = 0.0f;
		SetPedestrianAnimationPaused(false);
		UE_LOG(LogTemp, Display, TEXT("机器人已离开安全范围，动态行人继续移动。"));
	}

	if (!TravelDirection.IsNearlyZero())
	{
		const FRotator CurrentRotation = GetActorRotation();
		const FRotator TargetRotation(0.0f, TravelDirection.Rotation().Yaw, 0.0f);
		SetActorRotation(FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaSeconds, 8.0f));
	}

	const FVector NewLocation = FMath::VInterpConstantTo(
		CurrentLocation,
		TargetLocation,
		DeltaSeconds,
		MoveSpeedCmPerSecond);
	SetActorLocation(NewLocation, false);

	if (NewLocation.Equals(TargetLocation, 0.1f))
	{
		SetActorLocation(TargetLocation, false);
		HandleEndpointReached();
	}
}

void ADynamicPedestrianObstacle::ApplyMeshAsset()
{
	if (PedestrianMesh == nullptr)
	{
		return;
	}

	PedestrianMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PedestrianMesh->SetRelativeLocation(MeshRelativeLocation);
	PedestrianMesh->SetRelativeRotation(MeshRelativeRotation);

	if (PedestrianMeshAsset != nullptr)
	{
		PedestrianMesh->SetSkeletalMesh(PedestrianMeshAsset);
	}
}

void ADynamicPedestrianObstacle::SetPedestrianAnimationPaused(bool bPaused)
{
	if (PedestrianMesh != nullptr)
	{
		// bPauseAnims 对动画蓝图和单节点动画都有效，暂停时会保留当前骨骼姿态。
		PedestrianMesh->bPauseAnims = bPaused;
	}
}

void ADynamicPedestrianObstacle::HandleEndpointReached()
{
	// 行人持续往返；当前阶段只有接近机器人时才允许进入停止状态。
	bMovingToEnd = !bMovingToEnd;
	SetPedestrianAnimationPaused(false);
}
