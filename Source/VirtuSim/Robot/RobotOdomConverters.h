#pragma once

#include "RobotOdomState.h"

#include "TempoROSCommonConverters.h"

#include "nav_msgs/msg/odometry.hpp"

// 告诉 TempoROS：FRobotOdomState 是一个可用于发布/订阅的消息类型。
DEFINE_TEMPOROS_MESSAGE_TYPE_TRAITS(FRobotOdomState)

/**
 * 把 UE 侧的 FRobotOdomState 翻译成 ROS 侧的 nav_msgs::msg::Odometry。
 * 这是 TempoROS 的隐式转换入口，发布 FRobotOdomState 时会自动走到这里。
 */
template <>
struct TImplicitToROSConverter<FRobotOdomState> : TToROSConverter<nav_msgs::msg::Odometry, FRobotOdomState>
{
	static ToType Convert(const FromType& State)
	{
		// 先创建 ROS 标准里程计消息，再逐字段填充。
		nav_msgs::msg::Odometry OdomMessage;

		// frame_id 表示这条里程计的父坐标系，child_frame_id 表示机器人底盘坐标系。
		OdomMessage.header.frame_id = "odom";
		OdomMessage.child_frame_id = "base_link";
		// 把 UE 世界时间转成 ROS 时间戳，供 TF 和 Nav2 做时序对齐。
		OdomMessage.header.stamp = TToROSConverter<builtin_interfaces::msg::Time, double>::Convert(State.TimestampSeconds);

		// position 在 Odometry 里是 Point，不是 Vector3，所以这里要显式转成 Point。
		OdomMessage.pose.pose.position = TToROSConverter<geometry_msgs::msg::Point, FVector>::Convert(State.Position);
		OdomMessage.pose.pose.orientation = TImplicitToROSConverter<FRotator>::Convert(State.Rotation);

		// 只给地面机器人常用的前向线速度和绕 Z 轴角速度赋值。
		// 这里先把标量速度包装成 FVector，再复用 TempoROS 已有的向量转换器。
		OdomMessage.twist.twist.linear = TImplicitToROSConverter<FVector>::Convert(FVector(State.LinearX, 0.0, 0.0));
		OdomMessage.twist.twist.angular = TImplicitToROSConverter<FVector>::Convert(FVector(0.0, 0.0, State.AngularZ));

		return OdomMessage;
	}
};
