# VirtuSim Ubuntu / ROS2 配置与调试

本文只记录 Ubuntu/ROS2 端的环境、配置路径、启动命令和常用排障方法。

## 1. 当前环境

```text
Ubuntu：VMware 虚拟机
ROS2：Humble
ROS_DOMAIN_ID：0
UE 工程：Windows D:\Epic\UEPJ\VirtuSim
ROS 工作目录：/home/ros2/virtusim_nav2
```

每个新终端先执行：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY
```

`unset ROS_LOCALHOST_ONLY` 用于允许 VMware Ubuntu 与 Windows UE 通过局域网 DDS 通信。

## 2. ROS2 端文件

```text
Nav2 参数：/home/ros2/virtusim_nav2/params/nav2_params_full.yaml
地图 YAML：/home/ros2/virtusim_nav2/maps/warehouse_map.yaml
地图图像：/home/ros2/virtusim_nav2/maps/warehouse_map.pgm
目标 Bridge：/home/ros2/virtusim_nav2/scripts/goal_bridge.py
```

`goal_bridge.py` 的数据流：

```text
UE /virtusim/goal_pose
        -> goal_bridge.py
        -> Nav2 /navigate_to_pose Action
        -> /virtusim/navigation_status
        -> UE
```

## 3. 完整启动顺序

每次 UE 重新 Play 后，仿真 `/clock` 会从 0 开始。应停止旧的 Nav2、RViz 和 Bridge，再按以下顺序启动。

### 终端 1：检查 UE 发布

先启动 UE 并进入 Play，然后执行：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

ros2 topic list -t
ros2 topic hz /clock
ros2 topic hz /scan
```

项目主要 Topic：

```text
/clock
/scan
/odom
/tf
/tf_static
/cmd_vel
/plan
/virtusim/goal_pose
/virtusim/navigation_status
```

### 终端 2：启动 Nav2

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

ros2 launch nav2_bringup bringup_launch.py \
  map:=/home/ros2/virtusim_nav2/maps/warehouse_map.yaml \
  params_file:=/home/ros2/virtusim_nav2/params/nav2_params_full.yaml \
  use_sim_time:=true
```

### 终端 3：启动目标 Bridge

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

python3 /home/ros2/virtusim_nav2/scripts/goal_bridge.py
```

### 终端 4：启动 RViz

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

rviz2
```

RViz 建议配置：

```text
Fixed Frame：map
Map：/map
LaserScan：/scan
Path：/plan
TF：启用
Local Costmap：/local_costmap/costmap
Global Costmap：/global_costmap/costmap
```

如果 LaserScan 没有显示，检查其 QoS，并分别尝试 `Reliable` 和 `Best Effort`。

## 4. LiDAR 与丢点测试

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

固定同一个 Random Seed，重新开始相同实验时，丢点序列应能够复现。只切换噪声开关时，丢点序列不应改变。

## 5. Nav2 状态检查

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

## 6. TF 检查

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

## 7. 常见问题

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

## 8. 停止与重新启动

前台运行的 Nav2、Bridge 和 RViz 可在各自终端按 `Ctrl+C` 停止。

重新测试的推荐顺序：

```text
停止 Nav2、Bridge、RViz
-> 停止并重新 Play UE
-> 确认 /clock、/tf、/scan
-> 启动 Nav2
-> 启动 goal_bridge.py
-> 启动 RViz
-> 设置导航目标
```

## 9. 自动导航诊断脚本

项目内提供：

```text
Tools/ROS2/virtusim_nav_diagnostics.py
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
