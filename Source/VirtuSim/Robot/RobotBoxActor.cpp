// Fill out your copyright notice in the Description page of Project Settings.
#include "RobotBoxActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "../Navigation/NavigationPathVisualizerComponent.h"
#include "RobotMotionComponent.h"
#include "LidarComponent.h"

ARobotBoxActor::ARobotBoxActor()
{
	// 运动、LiDAR 和路径显示由各组件的 Tick 驱动。
	PrimaryActorTick.bCanEverTick = false;
	robotBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RobotBodyMesh"));
	RootComponent = robotBodyMesh;
	// 当前导航阶段不由 UE 碰撞体阻挡运动；后续接入 URDF/精准 footprint 后再恢复碰撞验证。
	robotBodyMesh->SetMobility(EComponentMobility::Movable);
	robotBodyMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	robotBodyMesh->SetSimulatePhysics(false);

	robotMotionComponent = CreateDefaultSubobject<URobotMotionComponent>(TEXT("RobotMotionComponent"));
	lidarComponent = CreateDefaultSubobject<ULidarComponent>(TEXT("LidarComponent"));
	navigationPathVisualizerComponent =
		CreateDefaultSubobject<UNavigationPathVisualizerComponent>(TEXT("NavigationPathVisualizerComponent"));
}

void ARobotBoxActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (robotBodyMesh != nullptr)
	{
		robotBodyMesh->SetStaticMesh(RobotBodyAsset);
	}
}

void ARobotBoxActor::BeginPlay()
{
	Super::BeginPlay();
}
