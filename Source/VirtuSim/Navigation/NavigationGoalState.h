#pragma once

#include "CoreMinimal.h"

/**
 * UE 内部使用的导航目标状态，不是 ROS 消息本体。
 * 该状态描述用户在 UE 场景中选中的目标位置和最终朝向，后续由转换器映射成 ROS 的 PoseStamped。
 */
struct FNavigationGoalState
{
	/** 目标位置，单位为 UE 默认的厘米，且位于当前简化约定下的 map 坐标系中。 */
	FVector Position = FVector::ZeroVector;
	/** 目标最终偏航角，单位为 UE 的度。第一版由 UI 传入固定值 0。 */
	double YawDegrees = 0.0;
	/** 目标创建时的 UE 世界时间，单位为秒。 */
	double TimestampSeconds = 0.0;
};
