// Fill out your copyright notice in the Description page of Project Settings.

#include "DynamicPedestrianObstacle.h"

#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

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

	StartLocation = GetActorLocation();
	EndLocation = StartLocation + EndOffset;
	EndpointWaitRemaining = 0.0f;
	bMovingToEnd = true;
	bStopped = false;

	if (EndOffset.IsNearlyZero())
	{
		bStopped = true;
		UE_LOG(LogTemp, Warning, TEXT("DynamicPedestrianObstacle 的 EndOffset 为零，行人不会移动。"));
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("动态行人已启动：起点=%s，终点=%s，速度=%.1fcm/s"),
		*StartLocation.ToString(),
		*EndLocation.ToString(),
		MoveSpeedCmPerSecond);
}

void ADynamicPedestrianObstacle::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bStopped || MoveSpeedCmPerSecond <= 0.0f || DeltaSeconds <= 0.0f)
	{
		return;
	}

	if (EndpointWaitRemaining > 0.0f)
	{
		EndpointWaitRemaining = FMath::Max(0.0f, EndpointWaitRemaining - DeltaSeconds);
		return;
	}

	const FVector CurrentLocation = GetActorLocation();
	const FVector TargetLocation = bMovingToEnd ? EndLocation : StartLocation;
	const FVector TravelDirection = (TargetLocation - CurrentLocation).GetSafeNormal();

	if (bRotateAlongPath && !TravelDirection.IsNearlyZero())
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

void ADynamicPedestrianObstacle::HandleEndpointReached()
{
	if (!bLoop && bMovingToEnd)
	{
		bStopped = true;
		UE_LOG(LogTemp, Display, TEXT("动态行人已到达终点并停止。"));
		return;
	}

	bMovingToEnd = !bMovingToEnd;
	EndpointWaitRemaining = FMath::Max(0.0f, EndpointWaitSeconds);
}
