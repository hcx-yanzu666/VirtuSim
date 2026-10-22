#!/usr/bin/env python3
"""UE goal topic -> Nav2 action. Run ONE instance, in a single-thread executor."""

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node
from action_msgs.msg import GoalStatus
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import NavigateToPose
from std_msgs.msg import String


class VirtuSimGoalBridge(Node):
    def __init__(self):
        super().__init__("virtusim_goal_bridge")
        self.goal_handle = None
        self.busy = False  # Includes the interval before goal acceptance.
        self.cancel_requested = False
        self.cancel_in_flight = False
        self.cancel_accepted = False
        self.next_goal = None
        self.generation = 0
        self.status = "Idle"
        self.status_publisher = self.create_publisher(
            String, "/virtusim/navigation_status", 10)
        self.goal_subscription = self.create_subscription(
            PoseStamped, "/virtusim/goal_pose", self.on_goal_pose, 10)
        self.cancel_subscription = self.create_subscription(
            String, "/virtusim/cancel_navigation", self.on_cancel, 10)
        self.navigate_to_pose_client = ActionClient(
            self, NavigateToPose, "/navigate_to_pose")
        self.get_logger().info("Goal bridge ready (navigation + cancellation).")

    def publish_status(self, status):
        self.status = status
        message = String()
        message.data = status
        self.status_publisher.publish(message)
        self.get_logger().info(f"Navigation status: {status}")

    def on_goal_pose(self, pose):
        if self.busy:
            # Serialize replacement: finish the old action before sending the latest goal.
            self.next_goal = pose
            self.request_cancel()
            return
        self.start_goal(pose)

    def start_goal(self, pose):
        if not self.navigate_to_pose_client.server_is_ready():
            self.get_logger().error("Nav2 /navigate_to_pose is unavailable; retry when ready.")
            self.publish_status("Failed")
            return
        self.busy = True
        self.generation += 1
        generation = self.generation
        self.publish_status("GoalReceived")
        goal = NavigateToPose.Goal()
        goal.pose = pose
        try:
            future = self.navigate_to_pose_client.send_goal_async(goal)
            future.add_done_callback(lambda f: self.on_goal_response(f, generation))
        except Exception as error:
            self.get_logger().error(f"Sending goal failed: {error}")
            self.finish("Failed")

    def on_goal_response(self, future, generation):
        if generation != self.generation or not self.busy:
            return
        try:
            handle = future.result()
        except Exception as error:
            # Delivery is uncertain: do not silently start another task.
            self.get_logger().error(f"Goal response unavailable: {error}; inspect Nav2 before restarting bridge.")
            self.publish_status("CancelFailed")
            return
        if not handle.accepted:
            self.finish("Failed")
            return
        self.goal_handle = handle
        handle.get_result_async().add_done_callback(
            lambda f: self.on_navigation_result(f, generation))
        if self.cancel_requested:
            self.request_cancel()
        else:
            self.publish_status("GoalAccepted")
            self.publish_status("Navigating")

    def on_cancel(self, message):
        if message.data != "cancel":
            return
        self.next_goal = None  # Explicit cancel also removes a queued replacement.
        if not self.busy:
            self.publish_status(self.status)  # Do not fabricate a CANCELED result.
            return
        self.request_cancel()

    def request_cancel(self):
        self.cancel_requested = True
        self.publish_status("Canceling")
        if self.goal_handle is None or self.cancel_in_flight or self.cancel_accepted:
            return
        self.cancel_in_flight = True
        generation = self.generation
        try:
            future = self.goal_handle.cancel_goal_async()
            future.add_done_callback(lambda f: self.on_cancel_response(f, generation))
        except Exception as error:
            self.cancel_failed(str(error))

    def cancel_failed(self, reason):
        self.cancel_in_flight = False
        self.cancel_requested = False
        self.next_goal = None
        self.get_logger().warning(f"Cancellation not confirmed: {reason}")
        self.publish_status("CancelFailed")

    def on_cancel_response(self, future, generation):
        # A result can arrive before the cancel response, or a new task may have started.
        if generation != self.generation or not self.busy:
            return
        self.cancel_in_flight = False
        try:
            response = future.result()
            if not response.goals_canceling:
                self.cancel_failed(f"server return_code={response.return_code}")
                return
        except Exception as error:
            self.cancel_failed(str(error))
            return
        self.cancel_accepted = True
        # Acceptance means CANCELING, not CANCELED. Wait for the action result.
        self.publish_status("Canceling")

    def on_navigation_result(self, future, generation):
        if generation != self.generation or not self.busy:
            return
        try:
            status = future.result().status
        except Exception as error:
            self.get_logger().error(f"Result unavailable: {error}; task state is unknown.")
            self.publish_status("CancelFailed")
            return
        final_status = {
            GoalStatus.STATUS_SUCCEEDED: "Succeeded",
            GoalStatus.STATUS_CANCELED: "Canceled",
        }.get(status, "Failed")
        self.finish(final_status)

    def finish(self, status):
        self.goal_handle = None
        self.busy = False
        self.cancel_requested = False
        self.cancel_in_flight = False
        self.cancel_accepted = False
        queued = self.next_goal
        self.next_goal = None
        self.publish_status(status)
        if queued is not None:
            self.start_goal(queued)


def main():
    rclpy.init()
    node = VirtuSimGoalBridge()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.try_shutdown()


if __name__ == "__main__":
    main()
