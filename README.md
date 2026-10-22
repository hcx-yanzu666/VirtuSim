# VirtuSim · UE5 × ROS2 导航闭环仿真

基于 UE 5.6 C++、ROS2 Humble 和 Nav2 的个人机器人仿真项目。UE 提供仓库、机器人、虚拟 LiDAR 和交互界面，Nav2 在 Ubuntu 中完成导航，控制指令回传 UE 驱动机器人运动。

```text
UE 场景 / LiDAR → /scan、/odom、TF → Nav2
       ↑                              ↓
       └────────── /cmd_vel ───────────┘

UE 导航目标 / 取消 → Python goal_bridge → Nav2 Action → 状态回传 UE
场景 JSON → ScenarioRunner → 单轮执行 → JSON 结果
```

## 从哪里开始

| 想做什么 | 入口 |
|---|---|
| 重新运行一次导航 | [运行指南](Docs/运行指南.md) |
| 了解完成情况、已知问题和下一步 | [项目现状与下一步](Docs/项目现状与下一步.md) |
| 理解模块和数据流 | [系统架构](Docs/系统架构.md) |
| 修改自动场景输入 | [场景 JSON 格式](Docs/场景JSON格式.md) · [基准场景](Config/Scenarios/warehouse_baseline.json) |
| 部署 Ubuntu 脚本、核对参数和地图 | [ROS2 工具说明](Tools/ROS2/README.md) |
| 排查导航、TF、频率问题 | [ROS2 与 Nav2 排查](Docs/Troubleshooting/ROS2与Nav2排查.md) |
| 生成和导入仓库 | [仓库生成工具](Tools/WarehouseGenerator/README.md) |
| 从当前关卡导出导航地图 | [UE 场景地图导出](Tools/WarehouseGenerator/UE场景地图导出.md) |
| 查看版本规划 | [演进路线](Docs/演进路线.md) |

## 当前能力与边界

- 单机器人 LiDAR + Nav2 导航闭环，支持目标下发、取消、状态和路径显示。
- LiDAR 参数运行时调整，支持高斯距离噪声、丢点、固定延迟和随机种子。
- SFL_CBD16 机器人外观、固定路线动画行人；行人可被 LiDAR 检测。
- 单机器人 JSON 场景初始化、导航生命周期管理和独立运行报告。
- ROS2 诊断脚本与导航 Action Bridge 的离线生命周期测试。
- 编辑器可视范围框选地图，按静态碰撞导出 PGM、YAML、元数据和 PNG 预览。

目前采用运动学速度积分和真值里程计，移动时关闭 Sweep；机器人模型是 Mesh 组装，并未接入完整 URDF 动力学。动态交互、碰撞评价和批量回归仍待补齐；不将 Nav2 成功等同于无碰撞，不宣称已完成 Real2Sim / Sim2Real 验证。

## 目录约定

```text
Source/VirtuSim/           UE C++：通信、机器人、导航、场景、UI
Content/                  UE 地图、蓝图、模型、动画和材质
Plugins/                  TempoROS、UMGAutoBuilder 等依赖
Config/Scenarios/         自动场景输入，UE 世界厘米 / 角度
Config/UMGAutoBuilder/    UI 定义
Docs/                     当前文档、排障记录和历史归档
Tools/ROS2/               Ubuntu 脚本、参数、地图与离线测试
Tools/WarehouseGenerator/ 仓库生成、导入及地图导出工具
Skill/                    AI 协作与教学约定
Saved/ScenarioResults/    UE 单轮报告，本机输出，不提交 Git
```

Windows 侧使用 UE 5.6 和对应 C++ 构建环境；Ubuntu 侧使用 ROS2 Humble、Nav2 和 RViz2。依赖插件、Git 子模块和 LFS 资产需准备齐全；具体运行步骤见运行指南。当前工程默认地图不是导航测试地图，需要在编辑器中手动打开 `LidarTestMap`。

仓库保留的 Nav2 YAML 是旧圆形底盘基线，尚未取得 Ubuntu 上调好的完整配置快照；部署前请先阅读 [参数状态说明](Tools/ROS2/params/README.md)。演示视频与重复实验结果后续补充。

最新地图快照来自当前 UE 关卡，仍待完成导航验证。自动场景要求机器人标签唯一；新地图联调时曾因旧机器人与新机器人标签重复导致初始化失败，不能据此认定地图已验收。
