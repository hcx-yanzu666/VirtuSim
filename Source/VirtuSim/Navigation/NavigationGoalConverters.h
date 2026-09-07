#pragma once

#include "NavigationGoalState.h"

#include "TempoROSCommonConverters.h"

#include "geometry_msgs/msg/pose_stamped.hpp"

// 告诉 TempoROS：FNavigationGoalState 是一个可用于发布的消息类型。
DEFINE_TEMPOROS_MESSAGE_TYPE_TRAITS(FNavigationGoalState)

/**
 * 把 UE 侧的 FNavigationGoalState 翻译成 ROS 侧的 geometry_msgs::msg::PoseStamped。
 * 发布 FNavigationGoalState 时，TempoROS 会自动使用这个隐式转换器。
 */
template <>
struct TImplicitToROSConverter<FNavigationGoalState> : TToROSConverter<geometry_msgs::msg::PoseStamped, FNavigationGoalState>
{
	static ToType Convert(const FromType& State)
	{
		geometry_msgs::msg::PoseStamped GoalMessage;

		// Nav2 的全局导航目标必须在 map 坐标系下表达。
		GoalMessage.header.frame_id = "map";
		GoalMessage.header.stamp = TToROSConverter<builtin_interfaces::msg::Time, double>::Convert(State.TimestampSeconds);

		// 统一复用 TempoROS 的厘米转米、UE Y 轴转 ROS Y 轴规则。
		GoalMessage.pose.position = TToROSConverter<geometry_msgs::msg::Point, FVector>::Convert(State.Position);
		GoalMessage.pose.orientation = TImplicitToROSConverter<FRotator>::Convert(
			FRotator(0.0, State.YawDegrees, 0.0));

		return GoalMessage;
	}
};
