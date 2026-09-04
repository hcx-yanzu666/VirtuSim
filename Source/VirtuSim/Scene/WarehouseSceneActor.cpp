#include "WarehouseSceneActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
constexpr float kCubeMeshSizeCentimeters = 100.0f;
constexpr float kWarehouseLengthCentimeters = 2400.0f;
constexpr float kWarehouseWidthCentimeters = 1600.0f;
constexpr float kFloorThicknessCentimeters = 10.0f;
constexpr float kWallThicknessCentimeters = 20.0f;
constexpr float kWallHeightCentimeters = 180.0f;
constexpr float kShelfHeightCentimeters = 120.0f;
}

AWarehouseSceneActor::AWarehouseSceneActor()
{
	PrimaryActorTick.bCanEverTick = false;

	sceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = sceneRoot;

	floorInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorInstances"));
	floorInstances->SetupAttachment(sceneRoot);

	wallInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallInstances"));
	wallInstances->SetupAttachment(sceneRoot);

	shelfInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ShelfInstances"));
	shelfInstances->SetupAttachment(sceneRoot);

	obstacleInstances = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ObstacleInstances"));
	obstacleInstances->SetupAttachment(sceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMeshAsset.Succeeded())
	{
		floorInstances->SetStaticMesh(CubeMeshAsset.Object);
		wallInstances->SetStaticMesh(CubeMeshAsset.Object);
		shelfInstances->SetStaticMesh(CubeMeshAsset.Object);
		obstacleInstances->SetStaticMesh(CubeMeshAsset.Object);
	}

	for (UInstancedStaticMeshComponent* Component : { floorInstances, wallInstances, shelfInstances, obstacleInstances })
	{
		Component->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Component->SetCollisionObjectType(ECC_WorldStatic);
		Component->SetCollisionResponseToAllChannels(ECR_Block);
	}
}

void AWarehouseSceneActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	floorInstances->ClearInstances();
	wallInstances->ClearInstances();
	shelfInstances->ClearInstances();
	obstacleInstances->ClearInstances();

	buildBoundary();
	buildShelves();
	buildTestObstacles();
}

void AWarehouseSceneActor::addBoxInstance(UInstancedStaticMeshComponent* TargetComponent, const FVector& Center, const FVector& Size) const
{
	if (TargetComponent == nullptr)
	{
		return;
	}

	const FVector Scale = Size / kCubeMeshSizeCentimeters;
	TargetComponent->AddInstance(FTransform(FRotator::ZeroRotator, Center, Scale));
}

void AWarehouseSceneActor::buildBoundary()
{
	addBoxInstance(
		floorInstances,
		FVector(0.0f, 0.0f, -kFloorThicknessCentimeters * 0.5f),
		FVector(kWarehouseLengthCentimeters, kWarehouseWidthCentimeters, kFloorThicknessCentimeters));

	const float halfLength = kWarehouseLengthCentimeters * 0.5f;
	const float halfWidth = kWarehouseWidthCentimeters * 0.5f;
	const float wallZ = kWallHeightCentimeters * 0.5f;

	addBoxInstance(wallInstances, FVector(0.0f, halfWidth, wallZ), FVector(kWarehouseLengthCentimeters, kWallThicknessCentimeters, kWallHeightCentimeters));
	addBoxInstance(wallInstances, FVector(0.0f, -halfWidth, wallZ), FVector(kWarehouseLengthCentimeters, kWallThicknessCentimeters, kWallHeightCentimeters));
	addBoxInstance(wallInstances, FVector(halfLength, 0.0f, wallZ), FVector(kWallThicknessCentimeters, kWarehouseWidthCentimeters, kWallHeightCentimeters));
	addBoxInstance(wallInstances, FVector(-halfLength, 0.0f, wallZ), FVector(kWallThicknessCentimeters, kWarehouseWidthCentimeters, kWallHeightCentimeters));
}

void AWarehouseSceneActor::buildShelves()
{
	const FVector shelfSize(90.0f, 420.0f, kShelfHeightCentimeters);
	const float shelfZ = kShelfHeightCentimeters * 0.5f;

	addBoxInstance(shelfInstances, FVector(-520.0f, -320.0f, shelfZ), shelfSize);
	addBoxInstance(shelfInstances, FVector(-170.0f, -320.0f, shelfZ), shelfSize);
	addBoxInstance(shelfInstances, FVector(180.0f, -320.0f, shelfZ), shelfSize);
	addBoxInstance(shelfInstances, FVector(530.0f, -320.0f, shelfZ), shelfSize);

	addBoxInstance(shelfInstances, FVector(-520.0f, 320.0f, shelfZ), shelfSize);
	addBoxInstance(shelfInstances, FVector(-170.0f, 320.0f, shelfZ), shelfSize);
	addBoxInstance(shelfInstances, FVector(180.0f, 320.0f, shelfZ), shelfSize);
	addBoxInstance(shelfInstances, FVector(530.0f, 320.0f, shelfZ), shelfSize);
}

void AWarehouseSceneActor::buildTestObstacles()
{
	const FVector obstacleSize(45.0f, 45.0f, 70.0f);
	const float obstacleZ = obstacleSize.Z * 0.5f;

	addBoxInstance(obstacleInstances, FVector(-780.0f, 0.0f, obstacleZ), obstacleSize);
	addBoxInstance(obstacleInstances, FVector(780.0f, 0.0f, obstacleZ), obstacleSize);
	addBoxInstance(obstacleInstances, FVector(0.0f, 600.0f, obstacleZ), obstacleSize);
}
