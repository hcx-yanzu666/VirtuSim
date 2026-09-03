#pragma once

#include "CoreMinimal.h"

/**
 * UE 内部使用的一帧二维 LiDAR 扫描结果，不是 ROS 消息本体。
 * 先由射线检测填充，再由转换器映射为 ROS 的 sensor_msgs::msg::LaserScan。
 */
struct FLidarScanState
{
	/** 雷达数据所属坐标系，必须与 TF 树中的雷达坐标系名称一致。 */
	FString FrameId = TEXT("laser_link");

	/** 本次扫描对应的 UE 世界时间，单位为秒。 */
	double TimestampSeconds = 0.0;

	/** 扫描起始角度，单位为弧度。 */
	float AngleMinRadians = -PI;

	/** 扫描结束角度，单位为弧度。 */
	float AngleMaxRadians = PI;

	/** 相邻两条射线之间的角度间隔，单位为弧度。 */
	float AngleIncrementRadians = FMath::DegreesToRadians(1.0f);

	/** 可接受的最小测距，单位为厘米。 */
	float RangeMinCentimeters = 10.0f;

	/** 可接受的最大测距，单位为厘米。 */
	float RangeMaxCentimeters = 1000.0f;

	/** 每条射线对应的测距结果，顺序与扫描角度一一对应，单位为厘米。 */
	TArray<float> RangesCentimeters;
};
