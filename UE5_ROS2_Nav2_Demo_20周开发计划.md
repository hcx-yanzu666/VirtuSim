# UE5 + ROS2 + Nav2 Demo 20 周开发计划

## 1. 项目目标

本项目的第一目标不是做一个功能很多的仿真平台，而是先完成一个可验证的机器人导航闭环：

```text
UE5 仓储场景
  ↓
虚拟机器人与 LiDAR
  ↓
ROS2 Topic / TF
  ↓
Nav2 规划与控制
  ↓
/cmd_vel
  ↓
UE5 机器人运动
```

项目完成后，应能证明：

- UE5 可以作为机器人仿真世界运行。
- UE5 生成的虚拟传感器数据可以被 ROS2/Nav2 消费。
- Nav2 输出的控制指令可以驱动 UE5 中的机器人运动。
- 动态障碍物、传感器噪声、环境参数变化可以影响导航结果。
- 多机器人和任务调度可以在同一套闭环基础上扩展。

## 2. 开发原则

- 先闭环，再美化。
- 先单机器人，再多机器人。
- 先 2D LiDAR + Nav2，再考虑 Camera、IMU 和复杂感知。
- 每一阶段必须有可运行验收，不只写代码。
- 所有关键 Topic、TF、坐标系、频率、延迟都要记录。
- 不修改 Nav2 源码，优先通过标准 Topic、参数和 launch 接入。

## 3. 阶段里程碑

| 里程碑 | 周期 | 验收目标 | 重要性 |
|---|---:|---|---|
| M1 | Week 4 | ROS2 可以通过 `/cmd_vel` 控制 UE5 机器人运动 | 必须 |
| M2 | Week 8 | UE5 发布 `/odom`、`/tf`、`/scan`，RViz2 可正确显示 | 必须 |
| M3 | Week 11 | Nav2 控制 UE5 机器人完成 NavigateToPose | 必须 |
| M4 | Week 15 | 动态障碍物和传感器噪声影响导航行为 | 强烈建议 |
| M5 | Week 18 | 3～10 台机器人使用 namespace 独立运行 | 建议 |
| M6 | Week 20 | 完成可展示 Demo、性能记录和项目文档 | 必须 |

## 4. 每周计划

### Week 1：UE5 项目与仓储场景

任务：

- 创建 UE5 C++ 项目。
- 搭建地面、墙体、货架、装卸区、充电区。
- 设置观察相机和调试视角。
- 建立项目目录结构。

验收：

- UE5 中可以自由观察仓库场景。
- 场景尺寸使用真实单位设计，方便与 ROS2 米制坐标对齐。

### Week 2：机器人 Actor 与本地运动

任务：

- 创建 `RobotActor`。
- 添加车体、轮子、LiDAR 安装点、相机安装点。
- 实现线速度、角速度控制。
- 建立机器人状态结构。

验收：

- 不依赖 ROS2，通过 UE5 本地输入控制机器人前进、后退、旋转。
- 输出位置、朝向、线速度、角速度。

### Week 3：TempoROS 与 ROS2 网络通信

任务：

- 集成 TempoROS。
- UE5 创建 ROS2 Node。
- UE5 发布 `/test`。
- Ubuntu 使用 `ros2 topic echo /test` 验证。
- 记录 ROS_DOMAIN_ID、DDS、IP、防火墙配置。

验收：

```text
UE5 /test → Ubuntu ros2 topic echo
```

重点风险：

- Windows 与 Ubuntu 跨主机 DDS 自动发现失败。
- 防火墙阻止 UDP 通信。
- ROS2 发行版与 TempoROS 支持版本不匹配。

### Week 4：`/cmd_vel` 控制 UE5 机器人

任务：

- UE5 订阅 `/cmd_vel`。
- 将 `geometry_msgs/Twist` 转换为机器人运动输入。
- 增加最大速度和最大角速度限制。
- 增加指令超时停止机制。

验收：

```text
Ubuntu ros2 topic pub /cmd_vel
  ↓
UE5 RobotActor 移动
```

M1 完成标准：

- Ubuntu 可以稳定控制 UE5 机器人。
- 停止、转弯、速度限制行为可解释。

### Week 5：里程计 `/odom`

任务：

- UE5 根据机器人位姿和速度发布 `/odom`。
- 明确 UE5 坐标系与 ROS2 坐标系转换。
- 设置 `frame_id` 和 `child_frame_id`。

验收：

- `ros2 topic echo /odom` 数据连续。
- 位置、朝向、速度方向与 UE5 画面一致。

### Week 6：TF 树

任务：

- 发布 `odom → base_link`。
- 发布 `base_link → laser` 静态变换。
- 预留 `base_link → camera`。
- 明确 `map → odom` 由 AMCL 或 SLAM 负责。

验收：

```bash
ros2 run tf2_tools view_frames
```

目标 TF：

```text
map
 ↓
odom
 ↓
base_link
 ↓
laser
```

### Week 7：虚拟 LiDAR

任务：

- UE5 中通过 Raycast 实现 2D LiDAR。
- 支持扫描角度、扫描数量、最大/最小距离、更新频率。
- 在 UE5 中可视化 Ray 和 Hit Point。

验收：

- 障碍物距离与可视化结果一致。
- LiDAR 更新频率稳定。

### Week 8：发布 `/scan`

任务：

- 将 LiDAR 数据转换为 `sensor_msgs/LaserScan`。
- 正确填充角度、距离、`frame_id` 和时间戳。
- 在 RViz2 中查看 `/scan`。

验收：

- RViz2 中的激光结果与 UE5 场景基本一致。
- `/scan` 的 `frame_id` 与 TF 树匹配。

M2 完成标准：

- `/odom`、`/tf`、`/scan` 同时稳定发布。
- RViz2 能显示地图、机器人、TF 和激光数据。

### Week 9：Nav2 最小环境

任务：

- 在 Ubuntu 配置 ROS2 与 Nav2。
- 准备仓库 2D 栅格地图。
- 配置 map_server、AMCL、planner、controller。
- 验证 RViz2 和 NavigateToPose。

验收：

- 不连接 UE5 时 Nav2 节点能正常启动。
- RViz2 中可以设置初始位姿和目标点。

### Week 10：Nav2 与 UE5 联调

任务：

- 接入 UE5 的 `/scan`、`/odom`、`/tf`。
- 调整 costmap、footprint、速度和加速度参数。
- 检查时间戳、坐标轴、单位和 QoS。

验收：

- Nav2 能看到 UE5 机器人及传感器数据。
- 设置目标点后 Nav2 可以输出 `/cmd_vel`。

### Week 11：导航闭环

任务：

- 完成 NavigateToPose 闭环。
- 调整机器人运动模型与 controller 参数。
- 记录震荡、碰撞、定位漂移、TF 报错等失败案例。

验收：

```text
RViz2 设置目标点
  ↓
Nav2 发布 /cmd_vel
  ↓
UE5 机器人运动
  ↓
UE5 发布 /odom、/tf、/scan
  ↓
Nav2 持续规划
  ↓
机器人到达目标点
```

M3 完成标准：

- 至少在 3 个不同目标点完成自主导航。
- 记录导航耗时、路径长度、碰撞次数和到达结果。

### Week 12：动态障碍物

- 增加移动障碍物和临时障碍物。
- 验证 LiDAR 能扫描动态障碍。
- 观察 local costmap 和局部路径变化。

验收：

- 障碍物出现在路径上时，机器人可以减速、绕行或等待。

### Week 13：导航参数实验

- 测试 inflation radius、robot footprint、controller frequency、max velocity、acceleration。
- 建立参数与导航效果对照表。

验收：

- 能解释参数变化导致的导航行为变化。

### Week 14：LiDAR 噪声模型

- 加入高斯噪声、随机 dropout、最大距离截断。
- 支持不同噪声等级切换。

验收：

- 噪声等级升高时，costmap 和导航行为发生可解释变化。

### Week 15：环境影响导航

- 实现 Normal、Fog、Rain、Dark 等环境模式。
- 为每种模式配置不同噪声、丢帧率或频率。
- 记录导航成功率和路径变化。

M4 完成标准：

- 同一路线在不同环境模式下产生不同导航结果。
- 能说明“环境 → 传感器 → Nav2 → 行为”的因果链。

### Week 16：多机器人基础

- 从 1 台扩展到 3 台机器人。
- 为每台机器人建立 namespace。
- 区分 `/robot1/cmd_vel`、`/robot1/odom`、`/robot1/scan`。
- 区分 `robot1/base_link` 等 TF frame。

验收：

- 3 台机器人可以在 RViz2 中同时显示并独立控制。

### Week 17：多机器人导航与性能基线

- 扩展到 5～10 台机器人。
- 记录 FPS、CPU、GPU、内存和 Topic 频率。
- 降低传感器频率并观察性能变化。

验收：

- 能指出主要瓶颈来自渲染、Tick、Raycast、DDS 还是 Nav2。

### Week 18：简单任务调度

- 实现任务队列、机器人状态、任务分配、任务完成回报。
- 只使用最近机器人优先、空闲优先、电量不足不接任务等简单规则。

验收：

- 手动添加任务点后，Task Manager 可以选择机器人并发送 NavigateToPose。

M5 完成标准：

- 3～10 台机器人能基于简单规则执行任务。

### Week 19：性能优化

- 优化 Tick 调度。
- 优化 LiDAR Raycast 和 Topic 发布频率。
- 优化渲染 LOD。
- 对比 1、5、10、20 台机器人的性能。

验收：

- 输出优化前后性能对比，并保留可复现实验配置。

### Week 20：Demo 打磨与项目材料

- 完成 HMI，显示机器人状态、导航状态、LiDAR 状态、ROS2 连接状态和 FPS。
- 整理架构图、Topic 表、TF 树、性能表和 Demo 视频脚本。
- 编写 README 和简历项目描述。

M6 完成标准：

- 能完整演示单机器人导航闭环。
- 能演示动态障碍或噪声影响导航。
- 能演示多机器人基础运行或任务调度。
- 有文档、视频和关键指标。

## 5. 功能优先级

### S 级：必须完成

```text
UE5 仓储场景
RobotActor
/cmd_vel
/odom
/tf
虚拟 LiDAR
/scan
Nav2 NavigateToPose
导航闭环
RViz2 验证
```

### A 级：强烈建议

```text
动态障碍物
LiDAR 噪声
环境模式
HMI
参数实验记录
```

### B 级：有时间再做

```text
多机器人
Task Manager
性能优化
Camera
```

### C 级：最后考虑

```text
100 台机器人
复杂 Fleet Manager
IMU
视觉算法
自定义 Behavior Tree
高精物理仿真
```

## 6. 每周记录模板

```text
本周目标：
完成内容：
验证方式：
关键截图/视频：
Topic：
TF：
参数：
问题：
下周计划：
```

## 7. 最终交付物

- UE5 工程。
- ROS2/Nav2 launch 与参数文件。
- 仓库地图。
- Topic 与 TF 说明。
- 导航闭环演示视频。
- 动态障碍/噪声影响演示视频。
- 性能测试表。
- 项目 README。
- 简历项目描述。

