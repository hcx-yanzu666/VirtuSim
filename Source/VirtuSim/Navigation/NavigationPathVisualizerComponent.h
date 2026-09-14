#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NavigationPathVisualizerComponent.generated.h"

/**
 * 从通信子系统读取 UE 世界坐标路径，用调试线连接相邻点。
 * 不再转换坐标或直接订阅 ROS。普通 ActorComponent 没有空间变换，无需附着到根组件。
 */
UCLASS(ClassGroup=(Navigation), meta=(BlueprintSpawnableComponent))
class VIRTUSIM_API UNavigationPathVisualizerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UNavigationPathVisualizerComponent();

	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** 是否绘制导航路径；只影响显示，不影响导航和 ROS 通信。 */
	UPROPERTY(EditAnywhere, Category = "Navigation|Path")
	bool bVisualizationEnabled = true;

	/** 加在路径 Z 坐标上的高度，单位厘米，避免线与地面重叠。 */
	UPROPERTY(EditAnywhere, Category = "Navigation|Path", meta = (ClampMin = "0.0"))
	float HeightOffsetCentimeters = 5.0f;
};
