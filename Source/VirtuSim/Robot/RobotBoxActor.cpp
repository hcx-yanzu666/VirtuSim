// Fill out your copyright notice in the Description page of Project Settings.
#include "RobotBoxActor.h"

#include "UObject/ConstructorHelpers.h"
#include "Components/StaticMeshComponent.h"
#include "RobotMotionComponent.h"

ARobotBoxActor::ARobotBoxActor()
{
	PrimaryActorTick.bCanEverTick = true;
	cubeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CubeMesh"));
	RootComponent = cubeMesh;

	// 直接绑定引擎自带的基础立方体，拖进场景后就能看到一个真实的方盒子。
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		cubeMesh->SetStaticMesh(CubeMeshAsset.Object);
		cubeMesh->SetRelativeScale3D(FVector(1.0f, 1.0f, 1.0f));
	}

	robotMotionComponent = CreateDefaultSubobject<URobotMotionComponent>(TEXT("RobotMotionComponent"));
}

void ARobotBoxActor::BeginPlay()
{
	Super::BeginPlay();
}

void ARobotBoxActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

