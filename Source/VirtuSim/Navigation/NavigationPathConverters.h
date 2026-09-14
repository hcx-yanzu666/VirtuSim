#pragma once

#include "NavigationPathState.h"

#include "TempoROSCommonConverters.h"

#include "nav_msgs/msg/path.hpp"

// 告诉 TempoROS：FNavigationPathState 是一个可用于订阅的消息类型。
DEFINE_TEMPOROS_MESSAGE_TYPE_TRAITS(FNavigationPathState)

/**
 * 把 ROS 侧的 nav_msgs::msg::Path 转换为 UE 侧的 FNavigationPathState。
 * Path 中每个位姿的位置由 ROS 的米制右手坐标转换为 UE 的厘米制左手坐标。
 */
template <>
struct TImplicitFromROSConverter<FNavigationPathState>
	: TFromROSConverter<nav_msgs::msg::Path, FNavigationPathState>
{
	static ToType Convert(const FromType& PathMessage)
	{
		FNavigationPathState PathState;
		PathState.FrameId = TFromROSConverter<std::string, FString>::Convert(
			PathMessage.header.frame_id.c_str());
		PathState.TimestampSeconds = TFromROSConverter<builtin_interfaces::msg::Time, double>::Convert(
			PathMessage.header.stamp);

		PathState.Points.Reserve(static_cast<int32>(PathMessage.poses.size()));
		for (const geometry_msgs::msg::PoseStamped& StampedPose : PathMessage.poses)
		{
			// TempoROS 的 FVector 反向转换器接收 Vector3，而 Pose 使用 Point；
			// 两者的坐标含义相同，先复制字段，再复用统一的坐标系和单位转换。
			geometry_msgs::msg::Vector3 RosPosition;
			RosPosition.x = StampedPose.pose.position.x;
			RosPosition.y = StampedPose.pose.position.y;
			RosPosition.z = StampedPose.pose.position.z;

			PathState.Points.Add(TImplicitFromROSConverter<FVector>::Convert(RosPosition));
		}

		return PathState;
	}
};
