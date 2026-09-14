#pragma once

#include "CoreMinimal.h"

/**
 * UE 内部使用的 Nav2 路径状态，不是 ROS 消息本体。
 * 路径点已经转换为 UE 世界坐标和厘米单位，可直接交给调试绘制或样条可视化组件使用。
 */
struct FNavigationPathState
{
	/** 路径所属坐标系，Nav2 全局规划通常使用 map。 */
	FString FrameId;

	/** ROS Path 消息头中的时间戳，单位为秒。 */
	double TimestampSeconds = 0.0;

	/** 按规划顺序排列的 UE 路径点，单位为厘米。 */
	TArray<FVector> Points;
};
