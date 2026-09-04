#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WarehouseSceneActor.generated.h"

class UInstancedStaticMeshComponent;
class USceneComponent;

/**
 * 程序化生成第一版 Nav2 测试仓库。
 * 使用同一套尺寸生成 UE 场景和后续 2D 地图，避免手摆场景与导航地图不一致。
 */
UCLASS()
class VIRTUSIM_API AWarehouseSceneActor : public AActor
{
	GENERATED_BODY()

public:
	AWarehouseSceneActor();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** 把 UE 基础 Cube 按目标中心和尺寸生成一个盒体实例。 */
	void addBoxInstance(UInstancedStaticMeshComponent* TargetComponent, const FVector& Center, const FVector& Size) const;

	/** 生成地面和外围墙，提供 Nav2 全局地图的边界。 */
	void buildBoundary();

	/** 生成货架，形成可导航通道。 */
	void buildShelves();

	/** 生成少量测试障碍，用来验证 LiDAR 和 local costmap。 */
	void buildTestObstacles();

	UPROPERTY()
	USceneComponent* sceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Warehouse")
	UInstancedStaticMeshComponent* floorInstances = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Warehouse")
	UInstancedStaticMeshComponent* wallInstances = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Warehouse")
	UInstancedStaticMeshComponent* shelfInstances = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Warehouse")
	UInstancedStaticMeshComponent* obstacleInstances = nullptr;
};
