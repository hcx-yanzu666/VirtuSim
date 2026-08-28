# UE5 + ROS2 + Nav2 多机器人仿真 Demo 架构与岗位定位

## 1. Demo 定位

项目名称：

```text
基于 UE5 + ROS2 的机器人闭环仿真与数字孪生 Demo
```

英文名：

```text
UE5-based Robot Closed-loop Simulation Demo with ROS2 and Nav2
```

本项目面向机器人仿真、自动驾驶仿真、数字孪生、导航算法验证和 HMI 可视化等岗位能力建设。

项目核心不是“做一个好看的 UE 场景”，而是实现一条可以被验证的数据闭环：

```text
UE5 物理/场景
  ↓
虚拟传感器
  ↓
ROS2 Topic + TF
  ↓
Nav2
  ↓
控制指令
  ↓
UE5 机器人运动
```

## 2. 项目边界

第一版必须完成：

- UE5 仓储场景。
- 单机器人运动模型。
- ROS2 双向通信。
- `/cmd_vel`、`/odom`、`/tf`、`/scan`。
- RViz2 验证。
- Nav2 NavigateToPose 闭环。

第一版不做：

- 修改 Nav2 源码。
- 复杂 Fleet Manager。
- 高精度动力学仿真。
- 复杂视觉算法。
- 大规模交通流或城市级自动驾驶场景。

这样收敛后，项目更像真实工程中的仿真接入、传感器建模和导航闭环验证，而不是堆功能。

## 3. 总体架构

```text
┌─────────────────────────────────────────────────────────────┐
│                         UE5 仿真端                           │
│                                                             │
│  ┌──────────────┐   ┌──────────────┐   ┌─────────────────┐ │
│  │ Warehouse    │   │ RobotActor   │   │ Sensor System   │ │
│  │ Scene        │   │ Motion Model │   │ LiDAR / Camera  │ │
│  └──────┬───────┘   └──────┬───────┘   └────────┬────────┘ │
│         │                  │                    │          │
│         └──────────────────┼────────────────────┘          │
│                            ↓                               │
│                     TempoROS Bridge                         │
└────────────────────────────┬────────────────────────────────┘
                             │
                         ROS2 / DDS
                             │
┌────────────────────────────▼────────────────────────────────┐
│                       Ubuntu ROS2 端                         │
│                                                             │
│  ┌──────────────┐   ┌──────────────┐   ┌─────────────────┐ │
│  │ TF / Odom    │   │ Nav2         │   │ RViz2 / Tools   │ │
│  │ Localization │   │ Planner/Ctrl │   │ Debug & Verify  │ │
│  └──────────────┘   └──────┬───────┘   └─────────────────┘ │
│                            │                               │
│                         /cmd_vel                            │
└────────────────────────────┴────────────────────────────────┘
```

## 4. 模块职责

### 4.1 UE5 Scene System

职责：

- 维护仓库、货架、墙体、装卸区和充电区。
- 维护静态障碍物和动态障碍物。
- 提供可重复测试的导航路线。
- 为 LiDAR Raycast 提供碰撞对象。

设计重点：

- 场景单位与 ROS2 米制单位可转换。
- 障碍物碰撞层清晰。
- 保留固定测试路线，方便参数对比。

### 4.2 Robot System

职责：

- 表示机器人实体。
- 接收 `/cmd_vel`。
- 根据线速度和角速度更新位姿。
- 发布 `/odom`。
- 维护电量、任务状态和导航状态等展示字段。

机器人结构：

```text
RobotActor
├── Body
├── Wheels
├── LaserMount
├── CameraMount
├── MotionComponent
└── Ros2Component
```

第一版运动模型使用差速或简化底盘模型，不追求复杂动力学。

### 4.3 Sensor System

LiDAR 是第一优先级。

数据链路：

```text
UE5 Raycast
  ↓
Range Array
  ↓
sensor_msgs/LaserScan
  ↓
/scan
  ↓
Nav2 Costmap
```

必须处理：

- 扫描角度和分辨率。
- 最小/最大距离。
- 更新频率。
- `frame_id`。
- 时间戳。
- 噪声与丢点。

Camera 只作为展示和后续扩展，不阻塞导航闭环。

### 4.4 ROS2 Bridge

TempoROS 负责 UE5 与 ROS2 通信。

核心 Topic：

| Topic | 方向 | 消息类型 | 作用 |
|---|---|---|---|
| `/cmd_vel` | ROS2 → UE5 | `geometry_msgs/Twist` | 控制机器人速度 |
| `/odom` | UE5 → ROS2 | `nav_msgs/Odometry` | 提供里程计 |
| `/scan` | UE5 → ROS2 | `sensor_msgs/LaserScan` | 提供 LiDAR |
| `/tf` | UE5/ROS2 → ROS2 | `tf2_msgs/TFMessage` | 坐标变换 |
| `/camera/image` | UE5 → ROS2 | `sensor_msgs/Image` | 后期图像展示 |

调试工具：

```bash
ros2 topic list
ros2 topic echo /scan
ros2 topic hz /scan
ros2 run tf2_tools view_frames
rviz2
```

### 4.5 Nav2 System

Nav2 运行在 Ubuntu。

主要组件：

```text
map_server
amcl
planner_server
controller_server
bt_navigator
costmap_2d
```

本项目重点是接入和验证，不修改 Nav2 源码。

核心验证：

- Nav2 能消费 UE5 发布的 `/scan`。
- Nav2 能根据 TF 和 odom 判断机器人位姿。
- Nav2 能发布 `/cmd_vel`。
- UE5 机器人能按 `/cmd_vel` 移动并反馈状态。

## 5. TF 与坐标系

目标 TF 树：

```text
map
 ↓
odom
 ↓
base_link
 ↓
laser
```

责任划分：

| Transform | 发布者 | 说明 |
|---|---|---|
| `map → odom` | AMCL 或 SLAM | 全局定位修正 |
| `odom → base_link` | UE5 机器人里程计 | 局部连续运动 |
| `base_link → laser` | UE5 静态 TF | LiDAR 安装位置 |
| `base_link → camera` | UE5 静态 TF | 后期相机扩展 |

UE5 与 ROS2 坐标转换必须单独记录，避免左右手系、轴向和单位错误。

## 6. 闭环验证标准

最小闭环通过标准：

- RViz2 能显示地图、机器人、TF 和 LaserScan。
- 设置目标点后，Nav2 输出 `/cmd_vel`。
- UE5 机器人开始运动。
- 机器人运动后 `/odom` 和 TF 持续更新。
- LiDAR 扫描结果随机器人位置变化。
- 机器人最终到达目标点，或给出可解释失败原因。

建议记录指标：

| 指标 | 含义 |
|---|---|
| 到达成功率 | 多次 NavigateToPose 中成功次数 |
| 平均导航耗时 | 从发出目标到到达目标 |
| 路径长度 | Nav2 路径或机器人实际轨迹长度 |
| 碰撞次数 | 是否碰到障碍物 |
| `/scan` 频率 | LiDAR 发布稳定性 |
| `/odom` 频率 | 里程计稳定性 |
| FPS | UE5 渲染性能 |
| CPU/GPU | 性能瓶颈分析 |

## 7. 动态障碍与环境噪声

动态障碍物链路：

```text
动态障碍物
  ↓
UE5 LiDAR 命中变化
  ↓
/scan 变化
  ↓
Nav2 local costmap 变化
  ↓
局部路径调整
  ↓
机器人绕行或等待
```

环境噪声链路：

```text
Fog / Rain / Dark
  ↓
LiDAR 噪声、丢点、频率下降
  ↓
Costmap 不确定性增加
  ↓
路径变化、耗时增加、成功率下降
```

这部分能证明仿真不是静态动画，而是在影响下游算法行为。

## 8. 多机器人扩展

多机器人不是第一版核心，但适合用于提升岗位竞争力。

命名示例：

```text
/robot1/cmd_vel
/robot1/odom
/robot1/scan

/robot2/cmd_vel
/robot2/odom
/robot2/scan
```

扩展重点：

- namespace 管理。
- TF frame 管理。
- 多机器人 Tick 与传感器频率调度。
- ROS2 Topic 频率和 DDS 压力。
- RViz2 与 UE5 状态一致性。

## 9. 与个人背景的结合

从当前工作背景和仓库内容看，你已有的优势包括：

- Qt/C++ 工程经验。
- 插件化客户端架构经验。
- 机器人状态、参数配置、任务流程相关业务经验。
- URDF、TF、机器人模型展示相关经验。
- OpenGL/OSG/3D 可视化经验。
- 相机标定、图像展示、地图/点云相关模块经验。
- CMake、MSVC 和复杂工程构建经验。

对应关系：

| 既有能力 | 本项目对应能力 |
|---|---|
| Qt/C++ 客户端 | UE5 C++ 工程实现 |
| 插件化工程 | UE5 模块化组件设计 |
| 机器人状态配置 | RobotActor 状态建模 |
| URDF/TF 可视化 | ROS2 TF 树设计与调试 |
| OpenGL/3D | UE5 场景、可视化和性能优化 |
| 相机/标定 | Camera Sensor 扩展 |
| 地图/点云 | LiDAR、costmap、SLAM/Nav2 理解 |
| CMake/复杂构建 | UE5 + ROS2 跨平台联调 |

你的路线不是从零转行，而是从“机器人客户端/配置/可视化工程”升级到“机器人仿真平台工程”。

## 10. 岗位竞争力判断

### 10.1 机器人仿真岗位

完成 M3，即单机器人 Nav2 闭环后，竞争力为中等偏上。它能证明：

- UE5 仿真工程能力。
- ROS2 通信接入能力。
- TF、odom、LaserScan 等机器人数据链路理解。
- RViz2 调试能力。
- 导航闭环验证能力。

完成 M4，即动态障碍和噪声影响导航后，竞争力会明显增强，因为这更接近“仿真支撑算法验证”。

完成 M5，即多机器人和性能基线后，对仓储机器人、AMR、AGV、数字孪生岗位更有吸引力。

### 10.2 自动驾驶仿真岗位

当前项目竞争力为中等。

它能证明你理解仿真、传感器、数据闭环和可视化，但当前主题偏移动机器人和仓储导航，不是典型的自动驾驶全栈仿真。

智驾仿真岗位通常还关注：

- Camera、LiDAR、Radar 多传感器。
- OpenSCENARIO、OpenDRIVE 或类似地图/场景格式。
- 场景生成与回放。
- 感知、预测、规控算法接口。
- 数据闭环、回归测试和评测指标。
- Carla、LGSVL、AirSim 或 Unreal 仿真插件经验。

后期可以增加车辆模型、Camera + LiDAR、场景事件脚本、轨迹回放和碰撞/TTC 等指标，但不应阻塞第一版导航闭环。

## 11. 竞争力评分

| 完成度 | 简历竞争力 | 说明 |
|---|---:|---|
| UE5 场景和机器人移动 | 40/100 | 更像 UE 练习 |
| ROS2 `/cmd_vel`、`/odom`、`/tf` | 60/100 | 证明通信和机器人基础链路 |
| LiDAR + Nav2 闭环 | 75/100 | 对机器人仿真岗位有明显竞争力 |
| 动态障碍、噪声、参数实验 | 85/100 | 体现仿真验证价值 |
| 多机器人、调度、性能基线 | 90/100 | 对 AMR/仓储/数字孪生岗位较强 |
| 增加智驾场景、多传感器和评测 | 90 分以上 | 更贴近智驾仿真岗位 |

结合你的背景，完成 M3 后已经值得写进简历；完成 M4 后可以作为重点项目讲；完成 M5 后可以作为转向机器人/仿真平台岗位的主项目。

## 12. 简历表达建议

项目名称：

```text
基于 UE5 + ROS2 + Nav2 的机器人闭环仿真平台
```

项目描述：

```text
基于 UE5 C++ 构建仓储机器人仿真场景，实现虚拟 LiDAR Raycast、/scan、/odom、/tf 与 /cmd_vel 数据链路，并通过 ROS2 接入 Nav2，完成从 RViz2 目标点下发到 UE5 机器人运动反馈的闭环导航验证。
```

项目亮点：

- 实现 UE5 与 ROS2/Nav2 的跨平台闭环通信，完成 `/cmd_vel`、`/odom`、`/tf`、`/scan` 数据接入。
- 基于 UE5 Raycast 实现 2D LiDAR，并在 RViz2 中验证 LaserScan 与场景障碍物一致性。
- 构建动态障碍物和 LiDAR 噪声模型，验证环境变化对 Nav2 costmap 与路径规划结果的影响。
- 支持多机器人 namespace 与 TF 管理，记录不同机器人数量下的 FPS、CPU/GPU 和 Topic 频率。
- 结合 HMI 展示机器人状态、导航状态、传感器状态和性能指标。

## 13. 面试讲述主线

```text
为什么做：
  想把机器人仿真从纯可视化推进到算法闭环验证。

系统怎么分：
  UE5 负责世界、机器人和传感器；ROS2 负责通信；Nav2 负责导航；RViz2 负责验证。

最难的是什么：
  坐标系、TF、时间戳、Topic 频率、跨平台 DDS 通信、LiDAR 与 costmap 一致性。

怎么验证：
  RViz2 看 TF 和 LaserScan；NavigateToPose 看闭环；动态障碍和噪声看路径变化；多机器人看性能基线。

有什么结果：
  单机器人闭环成功率、平均耗时、路径长度、碰撞次数、FPS 和 Topic 频率。
```

## 14. 最终判断

这个项目有搞头。

对机器人仿真、AMR/AGV、数字孪生、机器人 HMI、导航系统集成岗位，竞争力可以做到中高。对自动驾驶仿真岗位，它是很好的切入项目，但需要后期补充车辆、道路场景、多传感器和评测体系，才能从“机器人仿真项目”升级成“智驾仿真项目”。

最短成功路径是：

```text
UE5 仓库
  ↓
单机器人
  ↓
ROS2 通信
  ↓
/cmd_vel + /odom + /tf
  ↓
LiDAR /scan
  ↓
Nav2 NavigateToPose
  ↓
动态障碍与噪声影响
```

只要这条线跑通，你展示的就不是一个 UE Demo，而是一个机器人闭环仿真系统。

