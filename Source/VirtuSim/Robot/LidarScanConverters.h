#pragma once

#include "LidarScanState.h"

#include "TempoROSCommonConverters.h"

#include "sensor_msgs/msg/laser_scan.hpp"

// 告诉 TempoROS：FLidarScanState 是一个可用于发布/订阅的消息类型。
DEFINE_TEMPOROS_MESSAGE_TYPE_TRAITS(FLidarScanState)

/**
 * 把 UE 侧的一帧 LiDAR 扫描结果翻译成 ROS 侧的 sensor_msgs::msg::LaserScan。
 * ROS LaserScan 使用米和弧度，UE 内部扫描距离使用厘米，因此这里集中做单位转换。
 */
template <>
struct TImplicitToROSConverter<FLidarScanState> : TToROSConverter<sensor_msgs::msg::LaserScan, FLidarScanState>
{
	static ToType Convert(const FromType& State)
	{
		constexpr float CentimetersToMeters = 0.01f;

		sensor_msgs::msg::LaserScan ScanMessage;

		// frame_id 必须和 TF 树里的雷达坐标系一致，否则 RViz/Nav2 无法把扫描点投到机器人周围。
		ScanMessage.header.frame_id = TCHAR_TO_UTF8(*State.FrameId);
		ScanMessage.header.stamp = TToROSConverter<builtin_interfaces::msg::Time, double>::Convert(State.TimestampSeconds);

		// LaserScan 的角度字段天然使用弧度，正好复用 UE 内部保存的弧度参数。
		ScanMessage.angle_min = State.AngleMinRadians;
		ScanMessage.angle_max = State.AngleMaxRadians;
		ScanMessage.angle_increment = State.AngleIncrementRadians;
		ScanMessage.time_increment = 0.0f;
		ScanMessage.scan_time = 0.0f;

		// ROS 标准距离单位是米，UE 内部为了和 Actor 位移一致暂时保存厘米。
		ScanMessage.range_min = State.RangeMinCentimeters * CentimetersToMeters;
		ScanMessage.range_max = State.RangeMaxCentimeters * CentimetersToMeters;

		ScanMessage.ranges.reserve(State.RangesCentimeters.Num());
		for (const float RangeCentimeters : State.RangesCentimeters)
		{
			ScanMessage.ranges.push_back(RangeCentimeters * CentimetersToMeters);
		}

		return ScanMessage;
	}
};
