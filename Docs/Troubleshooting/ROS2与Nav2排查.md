# ROS2 与 Nav2 排查

日常启动顺序见 [运行指南](../运行指南.md)，Ubuntu 文件部署见 [工具说明](../../Tools/ROS2/README.md)。本文集中保存传感器、节点、TF 和导航诊断方法。

以下命令在已执行 `source /opt/ros/humble/setup.bash`、`export ROS_DOMAIN_ID=0`、`unset ROS_LOCALHOST_ONLY` 的 Ubuntu 终端中运行。长时间监听的命令分别开终端，或按 Ctrl+C 后再执行下一条。

## 1. LiDAR 与丢点测试

检查发布频率和连接关系：

```bash
ros2 topic info /scan -v
ros2 topic hz /scan
```

查看一帧完整扫描：

```bash
ros2 topic echo /scan --once
```

持续查看距离数组：

```bash
ros2 topic echo /scan --field ranges
```

测试档位：

```text
0%：命中点完整。
30%：墙面出现随机缺口，但 ranges 数组长度不变。
100%：所有命中回波都按最大量程输出。
```

固定种子并重置随机流、保持相同采样调用顺序时，可复现随机序列。噪声与丢点使用独立随机流；这不保证异步 ROS 导航过程逐帧确定性。

## 2. Nav2 状态检查

```bash
ros2 lifecycle get /map_server
ros2 lifecycle get /amcl
ros2 lifecycle get /planner_server
ros2 lifecycle get /controller_server
ros2 lifecycle get /bt_navigator
ros2 lifecycle get /global_costmap/global_costmap
ros2 lifecycle get /local_costmap/local_costmap
```

正常运行时应处于 `active [3]`。

检查导航 Action 和 Bridge Topic：

```bash
ros2 action list
ros2 action info /navigate_to_pose
ros2 topic echo /virtusim/goal_pose
ros2 topic echo /virtusim/navigation_status
ros2 topic echo /plan --once
ros2 topic echo /cmd_vel
```

## 3. TF 检查

当前 TF 链：

```text
map -> odom -> base_link -> laser_link
```

检查动态 TF：

```bash
ros2 run tf2_ros tf2_echo odom base_link
```

检查 LiDAR 固定安装位姿：

```bash
ros2 run tf2_ros tf2_echo base_link laser_link
```

生成 TF 图：

```bash
ros2 run tf2_tools view_frames
```

## 4. 常见问题

### 看不到 UE Topic

```bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY
ros2 daemon stop
ros2 daemon start
ros2 topic list
```

同时检查 VMware 网络是否允许 Ubuntu 与 Windows 互相访问，以及防火墙是否阻止 DDS UDP 通信。

### 时间回跳或旧 TF

典型日志：

```text
TF_OLD_DATA
Detected jump back in time. Clearing TF buffer.
```

原因通常是 UE 重新 Play 后 `/clock` 从 0 开始，而旧 Nav2 仍保留上一次仿真时间。停止 Nav2、Bridge 和 RViz，重新 Play UE 后再依次启动它们。

### Nav2 不响应目标

依次检查：

```bash
ros2 topic echo /virtusim/goal_pose
ros2 action info /navigate_to_pose
ros2 topic echo /virtusim/navigation_status
ros2 lifecycle get /bt_navigator
```

### 动态障碍没有进入 Costmap

```bash
ros2 topic info /scan -v
ros2 topic echo /scan --once
ros2 lifecycle get /local_costmap/local_costmap
ros2 lifecycle get /global_costmap/global_costmap
```

确认 `nav2_params_full.yaml` 的 local/global costmap 都启用了 `ObstacleLayer`，并以 `/scan` 作为 observation source。

## 5. 自动导航诊断脚本

项目内提供：

```text
Tools/ROS2/scripts/virtusim_nav_diagnostics.py
```

将它放到 Ubuntu 的脚本目录：

```bash
/home/ros2/virtusim_nav2/scripts/virtusim_nav_diagnostics.py
```

运行：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

python3 /home/ros2/virtusim_nav2/scripts/virtusim_nav_diagnostics.py
```

脚本会订阅 `/scan`、`/odom`、`/cmd_vel`、`/plan`、导航目标、导航状态、local/global costmap 和 `/rosout`。每秒输出一次摘要；每收到一个 UE 导航目标就自动开始新 Run，并把关键事件保存到：

```text
/home/ros2/virtusim_nav2/logs/navigation_diagnostics.jsonl
```

完成多次导航后按 `Ctrl+C` 停止，然后查看每次运行汇总：

```bash
grep '"event": "run_finished"' \
  /home/ros2/virtusim_nav2/logs/navigation_diagnostics.jsonl
```

查看 Nav2 警告：

```bash
grep '"event": "nav2_warning"' \
  /home/ros2/virtusim_nav2/logs/navigation_diagnostics.jsonl
```

## 6. 路径变化但机器人不动

全局 `/plan` 存在不代表局部控制器能找到可执行轨迹。先保留零噪声、零丢点、零延迟基准，分别检查：

```bash
ros2 topic echo /cmd_vel --once
ros2 topic echo /odom --once
ros2 param get /local_costmap/local_costmap footprint
ros2 param get /global_costmap/global_costmap footprint
ros2 param get /controller_server FollowPath.critics
ros2 topic info /map -v
ros2 topic echo /map --once --field info
```

- cmd_vel 为零：检查局部 costmap 是否堵塞、footprint 与安全距离是否匹配，并结合控制器警告分析。
- cmd_vel 非零但位姿不变：检查 UE 是否收到命令、是否暂停，以及运动组件状态。
- 扫描与真实几何不一致：检查地图版本、坐标手性、TF、雷达安装位置及高度；参考 [LiDAR 镜像排查记录](LiDAR镜像问题.md)。
- 不通过强制最小前进速度掩盖不可行轨迹；行人掉头行为也不能代替 Nav2 失败分析。

UE 当前移动关闭 Sweep，行人仅有 LiDAR 查询代理，因此还需单独评价几何重叠，不能用物理阻挡作为安全保证。
