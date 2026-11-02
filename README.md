# VirtuSim · UE5 × ROS2 移动机器人仿真与导航评测

VirtuSim 是一个面向仓库移动机器人的 UE5 与 ROS2/Nav2 仿真项目。UE5 负责场景、机器人、虚拟 LiDAR 和交互界面，ROS2/Nav2 负责导航规划与控制，双方通过 ROS2 话题和 Action 组成完整的导航闭环。

```text
UE5 场景 / LiDAR ── /scan、/odom、TF ──> ROS2 / Nav2
      ^                                      |
      └────────────── /cmd_vel ──────────────┘

UE5 导航目标 / 取消 ──> Python Bridge ──> NavigateToPose
场景 JSON ──> ScenarioRunner ──> 单轮执行与结果报告
```

## 主要能力

- UE5 C++ 仿真场景、移动机器人和虚拟二维 LiDAR。
- LiDAR 扫描频率、角分辨率、视场、量程、噪声、丢点、延迟和随机种子等参数配置。
- ROS2 Humble 与 Nav2 集成，支持目标下发、`/cmd_vel`、`/odom`、`/scan`、TF 和导航状态回传。
- JSON 场景配置，支持机器人起点、目标、传感器参数和动态行人路线。
- 单轮导航状态管理与结果报告，记录终态、耗时、实际行驶距离、传感器参数及动态障碍物几何重叠指标。
- `NavigationMapExporter`：从 UE 关卡静态碰撞几何生成 Nav2 使用的 PGM、YAML、PNG 预览图和元数据。

## 目录

```text
Source/VirtuSim/       UE C++：通信、机器人、LiDAR、场景、评测和 UI
Content/               UE 关卡、蓝图、模型、动画和材质
Plugins/               TempoROS、UMGAutoBuilder 等插件
Config/Scenarios/      JSON 场景输入
Config/UMGAutoBuilder/ UI 配置
Docs/                  架构、运行、场景和地图导出说明
Tools/ROS2/deploy/     Ubuntu 侧 Nav2 参数、地图和 Bridge 文件
Saved/ScenarioResults/ UE 单轮运行报告（本地生成）
```

## 快速入口

- [运行指南](Docs/运行指南.md)：启动 ROS2/Nav2、Bridge 和 UE 仿真。
- [系统架构](Docs/系统架构.md)：查看模块职责和数据流。
- [场景 JSON 格式](Docs/场景JSON格式.md)：修改自动场景输入。
- [UE 场景地图导出](Docs/UE场景地图导出.md)：从当前关卡生成 Nav2 地图。
- [ROS2 部署说明](Tools/ROS2/README.md)：Ubuntu 侧需要复制的运行文件和命令。

## 运行环境

- Windows：Unreal Engine 5.6、Visual Studio C++ 构建环境。
- Ubuntu：ROS2 Humble、Nav2、RViz2 和 Python Bridge。
- 机器人与场景坐标采用 UE 世界厘米；发送到 ROS2 时转换为米，并按项目约定处理 Y 轴方向。

项目用于机器人导航仿真、传感器参数实验、场景执行和结果评测。模型、地图和传感器配置可按实际关卡与测试目标替换。
