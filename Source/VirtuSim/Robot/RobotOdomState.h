#pragma once

#include "CoreMinimal.h"

/**
 * UE 内部使用的里程计状态，不是 ROS 消息本体。
 * 这份状态只描述机器人当前在仿真世界里的位姿和速度，后续由转换器映射成 ROS 的 /odom 消息。
 */
struct FRobotOdomState
{
	/** 机器人当前位置，单位为 UE 默认的厘米。 */
	FVector Position = FVector::ZeroVector;
	/** 机器人当前朝向。 */
	FRotator Rotation = FRotator::ZeroRotator;
	/** 机器人当前线速度，单位为厘米每秒。 */
	double LinearX = 0.0;
	/** 机器人当前绕 Z 轴角速度，单位为度每秒。 */
	double AngularZ = 0.0;
	/** 当前状态对应的世界时间，单位为秒。 */
	double TimestampSeconds = 0.0;
};
