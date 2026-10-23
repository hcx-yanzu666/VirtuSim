#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NavigationMapExporter.generated.h"

class UBoxComponent;

/**
 * 从当前 UE 关卡的静态碰撞几何导出 Nav2 使用的二维占用地图。
 *
 * 这个类不是导航规划器，也不是 SLAM 节点，而是一个“场景几何 -> ROS 栅格地图”的
 * 编辑器工具。使用时把 Actor 放入关卡，调整 MapBounds，点击 Details 面板中的
 * Export Map。类会按 ResolutionMeters 将 XY 范围切成网格，并逐格调用 UE 碰撞查询：
 * 有 WorldStatic 碰撞的格子写成黑色，没有碰撞的格子写成白色。
 *
 * 第一版的设计边界：
 * - MapBounds 定义世界轴对齐的 XY 导出范围；
 * - 只查询 ECC_WorldStatic；行人和机器人需配置为其他 Object Type 才能排除；
 * - ObstacleMinZ / ObstacleMaxZ 定义障碍检测的世界高度范围，通常排除地面；
 * - 输出 PGM、PNG 预览、ROS map YAML 和描述导出结果的 metadata JSON；
 * - ROS 坐标约定为 ROS X=UE X/100、ROS Y=-UE Y/100。
 */
UCLASS()
class VIRTUSIM_API ANavigationMapExporter : public AActor
{
	GENERATED_BODY()

public:
	/** 创建可视范围框并关闭 Tick；构造阶段不扫描关卡。 */
	ANavigationMapExporter();

	/** 在编辑器 Details 面板中执行一次完整导出，不需要运行游戏或启动 ROS2。 */
	UFUNCTION(CallInEditor, Category = "Navigation Map")
	void ExportMap();

protected:
	/** 编辑器放置或修改 Actor 后，确保范围框保持无碰撞。 */
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** 世界轴对齐的地图范围；Box 的世界包围盒 XY 中心和尺寸参与导出。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Navigation Map", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> MapBounds = nullptr;

	/** 每个像素对应的米数；0.05 表示 5cm × 5cm 的方格。 */
	UPROPERTY(EditAnywhere, Category = "Navigation Map", meta = (ClampMin = "0.01", ClampMax = "0.5"))
	float ResolutionMeters = 0.05f;

	/** 参与静态障碍检测的世界 Z 范围，单位为厘米，独立于 MapBounds 的 Z 变换。 */
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

	/** 判断碰撞结果是否属于用户要求排除的辅助 Actor。 */
	bool IsExcludedActor(const AActor* Actor) const;

	/**
	 * 查询一个栅格对应的三维检测盒是否碰到了静态障碍物。
	 * WorldCellCenter 使用 UE 厘米坐标；返回 true 表示该栅格应该标成占用。
	 */
	bool IsOccupied(const FVector& WorldCellCenter, const FCollisionObjectQueryParams& ObjectQueryParams, const FCollisionQueryParams& QueryParams) const;

	/** 把相对项目目录的输出路径转换成绝对路径，并确保目录存在。 */
	bool ResolveOutputDirectory(FString& OutDirectory) const;

	/** 写出调试元数据；RosOrigin 单位为米。Nav2 加载不依赖这个 JSON。 */
	void WriteMetadata(const FString& Directory, int32 Width, int32 Height, const FVector2D& RosOrigin, int32 OccupiedCellCount) const;
};
