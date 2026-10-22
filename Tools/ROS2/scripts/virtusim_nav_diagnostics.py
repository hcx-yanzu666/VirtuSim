#!/usr/bin/env python3
"""Collect concise ROS2/Nav2 diagnostics for repeated VirtuSim navigation runs."""

import argparse
import json
import math
import signal
import sys
import time
from collections import deque
from datetime import datetime
from pathlib import Path
from typing import Any, Deque, Dict, Optional, Tuple

import rclpy
from geometry_msgs.msg import PoseStamped, Twist
from nav_msgs.msg import OccupancyGrid, Odometry, Path as NavPath
from rcl_interfaces.msg import Log
from rclpy.node import Node
from rclpy.qos import DurabilityPolicy, HistoryPolicy, QoSProfile, ReliabilityPolicy
from sensor_msgs.msg import LaserScan
from std_msgs.msg import String


SENSOR_QOS = QoSProfile(
    history=HistoryPolicy.KEEP_LAST,
    depth=10,
    reliability=ReliabilityPolicy.BEST_EFFORT,
    durability=DurabilityPolicy.VOLATILE,
)

DEFAULT_QOS = QoSProfile(
    history=HistoryPolicy.KEEP_LAST,
    depth=10,
    reliability=ReliabilityPolicy.RELIABLE,
    durability=DurabilityPolicy.VOLATILE,
)

NAV2_LOG_NODES = (
    "amcl",
    "behavior_server",
    "bt_navigator",
    "controller_server",
    "global_costmap",
    "local_costmap",
    "map_server",
    "planner_server",
    "smoother_server",
    "velocity_smoother",
    "waypoint_follower",
)

SUCCESS_WORDS = ("succeed", "success", "completed", "complete", "成功", "到达")
FAILURE_WORDS = ("fail", "abort", "cancel", "timeout", "失败", "取消", "超时")
ACTIVE_WORDS = ("navigat", "running", "accepted", "active", "执行", "导航中")


def finite(value: float) -> bool:
    return math.isfinite(value)


def path_length(message: NavPath) -> float:
    total = 0.0
    for previous, current in zip(message.poses, message.poses[1:]):
        dx = current.pose.position.x - previous.pose.position.x
        dy = current.pose.position.y - previous.pose.position.y
        dz = current.pose.position.z - previous.pose.position.z
        total += math.sqrt(dx * dx + dy * dy + dz * dz)
    return total


def status_kind(text: str) -> str:
    lowered = text.lower()
    if any(word in lowered for word in SUCCESS_WORDS):
        return "success"
    if any(word in lowered for word in FAILURE_WORDS):
        return "failure"
    if any(word in lowered for word in ACTIVE_WORDS):
        return "active"
    return "other"


class RateTracker:
    def __init__(self, window_seconds: float = 5.0) -> None:
        self.window_seconds = window_seconds
        self.samples: Deque[float] = deque()
        self.last_received: Optional[float] = None

    def mark(self, now: float) -> None:
        self.last_received = now
        self.samples.append(now)
        cutoff = now - self.window_seconds
        while self.samples and self.samples[0] < cutoff:
            self.samples.popleft()

    def rate(self) -> float:
        if len(self.samples) < 2:
            return 0.0
        duration = self.samples[-1] - self.samples[0]
        return (len(self.samples) - 1) / duration if duration > 0.0 else 0.0

    def age(self, now: float) -> Optional[float]:
        return None if self.last_received is None else now - self.last_received


class VirtuSimDiagnostics(Node):
    def __init__(self, output_path: Path, summary_period: float) -> None:
        super().__init__("virtusim_nav_diagnostics")
        self.output_path = output_path
        self.output_path.parent.mkdir(parents=True, exist_ok=True)
        self.output_file = self.output_path.open("a", encoding="utf-8")
        self.started_at = time.monotonic()
        self.last_summary_at = self.started_at
        self.summary_period = summary_period

        self.rates = {
            name: RateTracker()
            for name in ("scan", "odom", "cmd_vel", "plan", "local_costmap", "global_costmap")
        }

        self.latest_scan: Dict[str, Any] = {}
        self.latest_odom: Dict[str, Any] = {}
        self.latest_cmd: Dict[str, Any] = {"linear": 0.0, "angular": 0.0}
        self.latest_path: Dict[str, Any] = {}
        self.latest_costmaps: Dict[str, Any] = {}
        self.navigation_status = "unknown"

        self.run_id = 0
        self.run_active = False
        self.run_started_at: Optional[float] = None
        self.run_goal: Optional[Tuple[float, float]] = None
        self.run_start_odom: Optional[Tuple[float, float]] = None
        self.run_travel_distance = 0.0
        self.run_plan_messages = 0
        self.run_significant_plan_changes = 0
        self.run_first_warning: Optional[Dict[str, Any]] = None
        self.run_warning_count = 0
        self.run_zero_cmd_since: Optional[float] = None
        self.run_max_zero_cmd_seconds = 0.0
        self.previous_odom_position: Optional[Tuple[float, float]] = None
        self.previous_path_length: Optional[float] = None

        self.create_subscription(LaserScan, "/scan", self.on_scan, SENSOR_QOS)
        self.create_subscription(Odometry, "/odom", self.on_odom, SENSOR_QOS)
        self.create_subscription(Twist, "/cmd_vel", self.on_cmd_vel, DEFAULT_QOS)
        self.create_subscription(NavPath, "/plan", self.on_plan, DEFAULT_QOS)
        self.create_subscription(PoseStamped, "/virtusim/goal_pose", self.on_goal, DEFAULT_QOS)
        self.create_subscription(String, "/virtusim/navigation_status", self.on_status, DEFAULT_QOS)
        self.create_subscription(
            OccupancyGrid, "/local_costmap/costmap", lambda msg: self.on_costmap("local", msg), DEFAULT_QOS
        )
        self.create_subscription(
            OccupancyGrid, "/global_costmap/costmap", lambda msg: self.on_costmap("global", msg), DEFAULT_QOS
        )
        self.create_subscription(Log, "/rosout", self.on_rosout, DEFAULT_QOS)
        self.create_timer(0.2, self.on_timer)

        self.record("diagnostics_started", {"output": str(self.output_path)})
        print(f"[diagnostics] writing {self.output_path}", flush=True)
        print("[diagnostics] send a goal from UE; Ctrl+C stops and writes the last summary", flush=True)

    def now_wall(self) -> float:
        return time.monotonic()

    def elapsed(self, now: Optional[float] = None) -> float:
        value = self.now_wall() if now is None else now
        return value - self.started_at

    def record(self, event: str, data: Dict[str, Any]) -> None:
        payload = {
            "timestamp": datetime.now().astimezone().isoformat(timespec="milliseconds"),
            "elapsed_seconds": round(self.elapsed(), 3),
            "event": event,
            "run_id": self.run_id if self.run_active else None,
            **data,
        }
        self.output_file.write(json.dumps(payload, ensure_ascii=False) + "\n")
        self.output_file.flush()

    def on_scan(self, message: LaserScan) -> None:
        now = self.now_wall()
        self.rates["scan"].mark(now)
        valid = [
            value
            for value in message.ranges
            if finite(value) and message.range_min <= value < message.range_max - 1e-4
        ]
        self.latest_scan = {
            "count": len(message.ranges),
            "valid": len(valid),
            "valid_ratio": len(valid) / len(message.ranges) if message.ranges else 0.0,
            "minimum": min(valid) if valid else None,
            "range_max": message.range_max,
        }

    def on_odom(self, message: Odometry) -> None:
        now = self.now_wall()
        self.rates["odom"].mark(now)
        position = (message.pose.pose.position.x, message.pose.pose.position.y)
        self.latest_odom = {
            "x": position[0],
            "y": position[1],
            "linear": message.twist.twist.linear.x,
            "angular": message.twist.twist.angular.z,
        }
        if self.run_active and self.previous_odom_position is not None:
            self.run_travel_distance += math.dist(position, self.previous_odom_position)
        self.previous_odom_position = position

    def on_cmd_vel(self, message: Twist) -> None:
        now = self.now_wall()
        self.rates["cmd_vel"].mark(now)
        linear = message.linear.x
        angular = message.angular.z
        self.latest_cmd = {"linear": linear, "angular": angular}
        moving_command = abs(linear) > 0.01 or abs(angular) > 0.02
        if self.run_active:
            if moving_command:
                if self.run_zero_cmd_since is not None:
                    self.run_max_zero_cmd_seconds = max(
                        self.run_max_zero_cmd_seconds, now - self.run_zero_cmd_since
                    )
                self.run_zero_cmd_since = None
            elif self.run_zero_cmd_since is None:
                self.run_zero_cmd_since = now

    def on_plan(self, message: NavPath) -> None:
        now = self.now_wall()
        self.rates["plan"].mark(now)
        length = path_length(message)
        significant = False
        if self.previous_path_length is not None:
            significant = abs(length - self.previous_path_length) >= 0.25
        self.previous_path_length = length
        self.latest_path = {"poses": len(message.poses), "length": length}
        if self.run_active:
            self.run_plan_messages += 1
            if significant:
                self.run_significant_plan_changes += 1
                self.record("significant_plan_change", {"path_length": round(length, 3)})

    def on_goal(self, message: PoseStamped) -> None:
        goal = (message.pose.position.x, message.pose.position.y)
        if self.run_active:
            self.finish_run("replaced_by_new_goal")
        self.run_id += 1
        self.run_active = True
        self.run_started_at = self.now_wall()
        self.run_goal = goal
        self.run_start_odom = (
            (self.latest_odom["x"], self.latest_odom["y"]) if self.latest_odom else None
        )
        self.run_travel_distance = 0.0
        self.run_plan_messages = 0
        self.run_significant_plan_changes = 0
        self.run_first_warning = None
        self.run_warning_count = 0
        self.run_zero_cmd_since = self.run_started_at
        self.run_max_zero_cmd_seconds = 0.0
        self.previous_path_length = None
        self.record("run_started", {"goal_x": goal[0], "goal_y": goal[1]})
        print(f"\n[run {self.run_id}] START goal=({goal[0]:.2f}, {goal[1]:.2f})", flush=True)

    def on_status(self, message: String) -> None:
        previous = self.navigation_status
        self.navigation_status = message.data
        kind = status_kind(message.data)
        if message.data != previous:
            self.record("navigation_status", {"status": message.data, "kind": kind})
            print(f"[status] {message.data}", flush=True)
        if self.run_active and kind in ("success", "failure"):
            self.finish_run(kind)

    def on_costmap(self, name: str, message: OccupancyGrid) -> None:
        now = self.now_wall()
        self.rates[f"{name}_costmap"].mark(now)
        known = [value for value in message.data if value >= 0]
        occupied = sum(value >= 65 for value in known)
        lethal = sum(value >= 99 for value in known)
        self.latest_costmaps[name] = {
            "width": message.info.width,
            "height": message.info.height,
            "occupied_ratio": occupied / len(known) if known else 0.0,
            "lethal_cells": lethal,
        }

    def on_rosout(self, message: Log) -> None:
        # Humble installations may expose Log.WARN as bytes. The message field
        # itself is numeric, so compare against the ROS WARN level directly.
        level = int(message.level)
        if level < 30:
            return
        node_name = message.name.lower()
        if not any(name in node_name for name in NAV2_LOG_NODES):
            return
        item = {
            "node": message.name,
            "level": level,
            "message": message.msg,
        }
        self.record("nav2_warning", item)
        if self.run_active:
            self.run_warning_count += 1
            if self.run_first_warning is None:
                self.run_first_warning = item
        print(f"[NAV2 WARN] {message.name}: {message.msg}", flush=True)

    def finish_run(self, result: str) -> None:
        if not self.run_active or self.run_started_at is None:
            return
        now = self.now_wall()
        if self.run_zero_cmd_since is not None:
            self.run_max_zero_cmd_seconds = max(
                self.run_max_zero_cmd_seconds, now - self.run_zero_cmd_since
            )
        summary = {
            "result": result,
            "duration_seconds": round(now - self.run_started_at, 3),
            "goal": self.run_goal,
            "start_odom": self.run_start_odom,
            "end_odom": (
                (self.latest_odom.get("x"), self.latest_odom.get("y")) if self.latest_odom else None
            ),
            "travel_distance_meters": round(self.run_travel_distance, 3),
            "plan_messages": self.run_plan_messages,
            "significant_plan_changes": self.run_significant_plan_changes,
            "max_zero_cmd_seconds": round(self.run_max_zero_cmd_seconds, 3),
            "nav2_warning_count": self.run_warning_count,
            "first_nav2_warning": self.run_first_warning,
            "last_status": self.navigation_status,
        }
        self.record("run_finished", summary)
        print(
            f"[run {self.run_id}] END result={result} duration={summary['duration_seconds']:.1f}s "
            f"travel={summary['travel_distance_meters']:.2f}m "
            f"plans={self.run_plan_messages} significant_changes={self.run_significant_plan_changes} "
            f"max_zero_cmd={self.run_max_zero_cmd_seconds:.1f}s warnings={self.run_warning_count}",
            flush=True,
        )
        self.run_active = False
        self.run_started_at = None

    def on_timer(self) -> None:
        now = self.now_wall()
        if now - self.last_summary_at < self.summary_period:
            return
        self.last_summary_at = now

        def rate(name: str) -> str:
            return f"{self.rates[name].rate():.1f}"

        scan_ratio = self.latest_scan.get("valid_ratio")
        scan_text = "--" if scan_ratio is None else f"{scan_ratio * 100.0:.0f}%"
        path_value = self.latest_path.get("length")
        path_text = "--" if path_value is None else f"{path_value:.2f}m"
        local = self.latest_costmaps.get("local", {})
        local_text = (
            "--" if not local else f"{local.get('occupied_ratio', 0.0) * 100.0:.1f}%/{local.get('lethal_cells', 0)}"
        )
        run_text = str(self.run_id) if self.run_active else "-"
        print(
            f"[summary] run={run_text} status={self.navigation_status!r} "
            f"Hz scan/odom/cmd/plan={rate('scan')}/{rate('odom')}/{rate('cmd_vel')}/{rate('plan')} "
            f"scan_valid={scan_text} path={path_text} "
            f"cmd=({self.latest_cmd.get('linear', 0.0):.2f},{self.latest_cmd.get('angular', 0.0):.2f}) "
            f"local_occ/lethal={local_text}",
            flush=True,
        )
        self.record(
            "periodic_summary",
            {
                "navigation_status": self.navigation_status,
                "rates_hz": {name: round(tracker.rate(), 3) for name, tracker in self.rates.items()},
                "scan": self.latest_scan,
                "odom": self.latest_odom,
                "cmd_vel": self.latest_cmd,
                "path": self.latest_path,
                "costmaps": self.latest_costmaps,
            },
        )

    def close(self) -> None:
        if self.run_active:
            self.finish_run("interrupted")
        self.record("diagnostics_stopped", {})
        self.output_file.close()


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--output",
        type=Path,
        default=Path.home() / "virtusim_nav2" / "logs" / "navigation_diagnostics.jsonl",
        help="JSONL output file",
    )
    parser.add_argument(
        "--summary-period",
        type=float,
        default=1.0,
        help="seconds between terminal summaries",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    rclpy.init()
    node = VirtuSimDiagnostics(args.output.expanduser(), args.summary_period)
    stopping = False

    def request_stop(_signum: int, _frame: Any) -> None:
        nonlocal stopping
        stopping = True

    signal.signal(signal.SIGINT, request_stop)
    signal.signal(signal.SIGTERM, request_stop)
    try:
        while rclpy.ok() and not stopping:
            rclpy.spin_once(node, timeout_sec=0.2)
    finally:
        node.close()
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()
    return 0


if __name__ == "__main__":
    sys.exit(main())
