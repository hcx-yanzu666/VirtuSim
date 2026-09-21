// Fill out your copyright notice in the Description page of Project Settings.
#include "RobotBoxActor.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "../Navigation/NavigationPathVisualizerComponent.h"
#include "RobotMotionComponent.h"
#include "LidarComponent.h"

ARobotBoxActor::ARobotBoxActor()
{
	// 运动、LiDAR 和路径显示由各组件的 Tick 驱动。
	PrimaryActorTick.bCanEverTick = false;
	cubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	RootComponent = cubeMesh;
	// Actor 的平移 Sweep 只检测根组件；用底盘的简单碰撞体阻挡场景障碍。
	cubeMesh->SetMobility(EComponentMobility::Movable);
	cubeMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	cubeMesh->SetSimulatePhysics(false);

	// 直接绑定引擎自带的基础立方体，拖进场景后就能看到一个真实的方盒子。
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		cubeMesh->SetStaticMesh(CubeMeshAsset.Object);
		// 引擎基础 Cube 原始尺寸为 100cm，缩放后作为 50cm 的临时机器人底盘。
		// 该尺寸的方形半对角线约为 0.36m，应与后续 Nav2 robot_radius 保持一致。
		cubeMesh->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
	}

	robotMotionComponent = CreateDefaultSubobject<URobotMotionComponent>(TEXT("RobotMotionComponent"));
	lidarComponent = CreateDefaultSubobject<ULidarComponent>(TEXT("LidarComponent"));
	navigationPathVisualizerComponent =
		CreateDefaultSubobject<UNavigationPathVisualizerComponent>(TEXT("NavigationPathVisualizerComponent"));
}

void ARobotBoxActor::BeginPlay()
{
	Super::BeginPlay();
}
