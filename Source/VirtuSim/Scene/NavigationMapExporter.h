#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigationMapExporter.generated.h"

class UBoxComponent;

/**
 * 从当前关卡的静态碰撞几何导出 Nav2 使用的二维占用地图。
 *
 * 第一版只支持世界轴对齐的单层地图：
 * - BoxComponent 定义 XY 导出范围；
 * - 只查询 ECC_WorldStatic，动态行人和机器人不会被烘焙进静态地图；
 * - 输出 PGM、PNG 预览、ROS map YAML 和描述坐标/检测范围的 metadata JSON。
 */
UCLASS()
class VIRTUSIM_API ANavigationMapExporter : public AActor
{
	GENERATED_BODY()

public:
	ANavigationMapExporter();

	/** 在编辑器 Details 面板中执行一次导出。 */
	UFUNCTION(CallInEditor, Category = "Navigation Map")
	void ExportMap();

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** 世界轴对齐的地图范围。Box 的 XY 中心和尺寸参与导出。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation Map", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> MapBounds = nullptr;

	/** 每个像素对应的米数。 */
	UPROPERTY(EditAnywhere, Category = "Navigation Map", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ResolutionMeters = 0.05f;

	/** 参与静态障碍检测的世界 Z 范围，单位为厘米。 */
	UPROPERTY(EditAnywhere, Category = "Navigation Map|Collision", meta = (ClampMin = "-100000.0"))
	float ObstacleMinZ = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Navigation Map|Collision", meta = (ClampMin = "-100000.0"))
	float ObstacleMaxZ = 200.0f;

	/** 相对项目目录的输出目录；绝对路径也可以使用。 */
	UPROPERTY(EditAnywhere, Category = "Navigation Map|Output")
	FString OutputDirectory = TEXT("Saved/NavigationMaps");

	UPROPERTY(EditAnywhere, Category = "Navigation Map|Output")
	FString OutputBaseName = TEXT("warehouse_map");

	/** 防止误设置极小分辨率导致编辑器长时间卡住。 */
	UPROPERTY(EditAnywhere, Category = "Navigation Map|Output", meta = (ClampMin = "1"))
	int32 MaxCellCount = 4000000;

	/** 导出时按 Actor Tag 排除静态辅助物体。 */
	UPROPERTY(EditAnywhere, Category = "Navigation Map|Collision")
	TArray<FName> ExcludedActorTags;

	bool IsExcludedActor(const AActor* Actor) const;
	bool IsOccupied(const FVector& WorldCellCenter, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& QueryParams) const;
	bool ResolveOutputDirectory(FString& OutDirectory) const;
	void WriteMetadata(const FString& Directory, int32 Width, int32 Height, const FVector2D& RosOrigin, int32 OccupiedCellCount) const;
};
