# V3 JSON 场景格式与约束

日期：2026-09-21  
状态：已实现固定格式加载器；场景执行器尚未实现。

> 实现范围调整：按当前 Demo 需求，加载器只保留文件读取、JSON 解析、必要字段读取和单机器人数量检查。下文中的 ID 格式、未知字段拒绝、版本白名单、数值范围等严格校验作为编写配置时的约定，不再要求加载器逐项实现。配置以 `Config/Scenarios/warehouse_baseline.json` 为模板；LiDAR 参数在后续应用到组件时复用组件校验。加载成功仅代表数据已读取，不代表场景可运行。

## 1. 目的与范围

场景文件描述“一次导航测试需要的输入条件”，包括机器人起点、目标、LiDAR 参数和运行上限。它不保存实时位置、当前导航状态或测试结果。

```text
Config/Scenarios/*.json
    ↓ ScenarioLoader：读取、解析、校验
FScenarioDefinition：与 JSON 解耦的内部数据
    ↓ ScenarioRunner：初始化、执行、结束与清理
现有机器人组件 / ROS Bridge / Nav2
    ↓
Saved/ScenarioResults/：每次运行的独立结果文件
```

第一版约定：

- 固定使用当前已经打开并验证过的仓库关卡，不自动切换 UE 关卡或 ROS 地图。
- 使用关卡中已有的一个机器人，不动态生成机器人。
- 用 `robots` 数组描述机器人，但版本 1 **必须且只能包含一个元素**。
- 一个机器人只有一个起点和一个导航目标。
- 支持配置现有 LiDAR 物理参数和随机种子。
- 一次加载执行一轮；重复运行和参数组合由后续批次执行功能管理。
- 动态障碍物事件、多个机器人、URDF 路径、道路格式暂不纳入版本 1。

数组是为后续扩展保留的表达方式，不代表目前支持多机器人。解析器不能静默忽略 `robots[1]` 及后续元素。

## 2. 文件约定

建议位置：`Config/Scenarios/warehouse_baseline.json`。

- UTF-8 编码，建议无 BOM。
- 标准 JSON：不使用注释、尾随逗号、NaN 或 Infinity。
- 字段名统一为 `snake_case`，区分大小写。
- 数字写成 JSON number，不能写成 `"10Hz"` 或 `"0.2"`。
- 布尔值使用 `true` / `false`，不接受 0/1 或字符串。
- 必填字段不能省略或为 `null`。
- 禁止重复键。实现解析器时需确认 JSON 库的重复键处理行为，不能声称反序列化成功就已完成该项校验。
- 未知字段报错，避免 `timeout_second` 等拼写错误被悄悄忽略。
- ID 满足 `[A-Za-z0-9][A-Za-z0-9_-]{0,63}`；展示说明使用 `description`。

开发阶段从项目 Config 目录读取。打包时需另行配置场景文件的拷贝与运行时路径，不假设它会随程序自动打包。

## 3. 完整示例

以下坐标仅演示结构，不保证在当前仓库内可达。开始执行前，应替换为已验证的起点与目标。

```json
{
  "schema_version": 1,
  "scenario_id": "warehouse_baseline",
  "description": "仓库单机器人理想 LiDAR 基准导航",
  "coordinate_system": "ue_world_cm_deg",
  "timeout_seconds": 120.0,
  "robots": [
    {
      "id": "robot_01",
      "actor_tag": "ScenarioRobot_01",
      "start": {
        "x": 0.0,
        "y": 0.0,
        "z": 80.0,
        "yaw": 0.0
      },
      "goal": {
        "x": 500.0,
        "y": 0.0,
        "z": 0.0,
        "yaw": 0.0
      },
      "lidar": {
        "scan_frequency_hz": 10.0,
        "angle_increment_degrees": 1.0,
        "horizontal_fov_degrees": 360.0,
        "range_min_meters": 0.1,
        "range_max_meters": 10.0,
        "noise_enabled": false,
        "noise_std_dev_meters": 0.0,
        "dropout_probability": 0.0,
        "fixed_delay_milliseconds": 0.0,
        "random_seed": 1234
      }
    }
  ]
}
```

## 4. 顶层字段

| 字段 | 类型 | 必填 | 约束与语义 |
|---|---|---|---|
| `schema_version` | 整数 | 是 | 当前只接受 1；未知版本报错 |
| `scenario_id` | 字符串 | 是 | 场景标识，满足 ID 格式；不等于运行 ID |
| `description` | 字符串 | 否 | 缺省为空字符串，仅供展示 |
| `coordinate_system` | 字符串 | 是 | 只接受 `ue_world_cm_deg` |
| `timeout_seconds` | 数值 | 是 | 有限且大于 0；导航执行阶段的仿真时间上限 |
| `robots` | 对象数组 | 是 | 版本 1 长度必须为 1 |

同一场景可以运行多次，每轮由执行器生成独立 `run_id`。不要通过修改 `scenario_id` 区分每一轮。

`timeout_seconds` 从当前导航任务被接受时开始计时，到收到终态或达到时间上限为止。初始化阶段另设就绪等待上限，不能无限等待 Nav2。运行超时后必须请求取消并等待确认；取消未确认时不能开始下一轮。

这里采用仿真时间，因此暂停 UE 会暂停运行计时。通信失联和初始化等待应使用独立的单调墙钟超时，后续由执行器统一实现。场景重置不应将 ROS `/clock` 归零。

## 5. 机器人字段与绑定

| 字段 | 类型 | 必填 | 语义 |
|---|---|---|---|
| `id` | 字符串 | 是 | 逻辑机器人标识，用于报告；满足 ID 格式 |
| `actor_tag` | 字符串 | 是 | UE Actor Tags 中用于查找现有机器人的标签；满足 ID 格式 |
| `start` | 位姿对象 | 是 | 机器人 Actor 原点的起始位姿 |
| `goal` | 位姿对象 | 是 | 发送给现有导航接口的目标位姿 |
| `lidar` | 对象 | 是 | 本轮完整的 LiDAR 实验参数 |

例如，在关卡机器人的 **Actor Tags** 中添加 `ScenarioRobot_01`。它不是 Component Tags，也不是编辑器显示名称 Actor Label。

加载执行时，标签必须恰好找到一个机器人，并确认它拥有项目需要的运动和 LiDAR 组件。找不到或找到多个都应报错。不要自动选择第一个 Actor。

`id` 和 `actor_tag` 不自动生成 ROS namespace。版本 1 继续使用现有 `/scan`、`/odom`、`/cmd_vel` 和单个 Nav2 实例。未来多机器人需要同时实现话题、TF、Action 与运行状态隔离。

## 6. 位姿、坐标系与单位

`start` 与 `goal` 均包含必填数值字段：`x`、`y`、`z`、`yaw`。

- `x/y/z`：UE 世界坐标，厘米；全部必须有限。
- `yaw`：UE 朝向，度；范围为 `[-180, 180]`。
- UE 采用 X 前、Y 右、Z 上的坐标约定。
- 版本 1 限定平面移动，机器人初始化时 Pitch、Roll 为 0。
- `start.z` 是 Actor 原点高度，应结合碰撞体和地面位置设置，不能统一填 0。
- `goal.z` 随现有目标接口传递；Nav2 的二维规划主要使用平面位置和 Yaw，该字段不代表高度控制目标。

解析器保存 UE 坐标，不在读 JSON 时翻转 Y 或将厘米改为米。发送 ROS 消息时继续复用现有坐标转换器，避免二次转换。

LiDAR 参数单位按字段后缀解释。例如位姿是厘米，但 `range_max_meters` 是米。这与现有 `FLidarParameters` 一致。

## 7. LiDAR 字段与现有代码映射

以下字段在版本 1 全部必填，避免实验结果依赖编辑器内上一次留下的参数。数值均要求有限。

| JSON 字段 | `FLidarParameters` 字段 | 类型 / 合法范围 |
|---|---|---|
| `scan_frequency_hz` | `ScanFrequencyHz` | 数值，1～30 |
| `angle_increment_degrees` | `AngleIncrementDegrees` | 数值，0.5～5 |
| `horizontal_fov_degrees` | `HorizontalFovDegrees` | 数值，30～360 |
| `range_min_meters` | `RangeMinMeters` | 数值，0.01～1 |
| `range_max_meters` | `RangeMaxMeters` | 数值，1～30 |
| `noise_enabled` | `bNoiseEnabled` | 布尔值 |
| `noise_std_dev_meters` | `NoiseStdDevMeters` | 数值，0～1 |
| `dropout_probability` | `DropoutProbability` | 数值，0～1 |
| `fixed_delay_milliseconds` | `FixedDelayMilliseconds` | 数值，0～500 |
| `random_seed` | `RandomSeed` | 整数，0～2147483647 |

关联约束：

- 最小量程必须严格小于最大量程。
- 角度分辨率不能大于水平 FOV。
- `noise_enabled=false` 时允许保留非零标准差，但本轮不施加噪声。
- 丢点概率写 `0.5` 表示 50%，不能写 50。
- 整数值必须先检查是否为整数和是否越界，再转换到 `int32`，不能截断小数。

解析后复用 `FLidarParameters::Validate()`，并补上 JSON 类型与有限数值检查。未来修改组件范围时，应同步更新本约定。

扫描点显示开关、点大小属于可视化设置，不写入本版测试输入。运行器应使用明确的展示配置，报告中可记录影响性能的展示条件。

固定 Seed 用于复现随机序列，但不能单独保证整个 ROS/Nav2 闭环逐帧确定。地图、初始条件、参数版本、事件时序和运行性能同样会影响结果。

## 8. 校验分层与失败行为

### 8.1 文件与格式校验：不修改世界

1. 文件存在且能读到非空文本。
2. JSON 语法合法且根节点为对象。
3. 字段完整、类型正确、无未知字段。
4. 版本和坐标约定受支持。
5. robots 数量合法，ID、数值范围和关联关系合法。

全部解析到临时结构体，所有检查成功后再赋值给输出。失败时保留原配置，不能部分应用。

错误信息需包含文件和字段路径，例如：

```text
warehouse_baseline.json: robots[0].lidar.dropout_probability 必须在 [0, 1]，实际为 50
warehouse_baseline.json: robots 当前仅支持 1 个元素，实际为 2
warehouse_baseline.json: robots[0].start.x 缺失或不是数值
```

### 8.2 运行前检查：验证场景与外部服务

- 机器人标签唯一，必需组件存在。
- 起点不存在阻挡碰撞或明显离地问题。
- 当前 UE 关卡与 ROS 地图对应。版本 1 由操作者确认；自动地图身份校验尚未实现。
- 旧任务已结束或已确认取消，残留命令与延迟扫描已清理。
- Bridge、Nav2、TF 和传感器链路满足运行条件。

数值合法不等于位置可达。目标在障碍物中或路径不可达，可能是测试本身的预期条件，应以导航结果体现，不能由 JSON 解析器武断判定。

运行前检查失败应输出 `SetupFailed` 和原因，不计为一次导航算法失败。尚未完成取消清理时，不启动新任务。

## 9. 输入与输出分离

场景输入文件不能被执行器改写来保存状态。每轮报告独立写入 `Saved/ScenarioResults/`，至少关联：

- `scenario_id`、`run_id`、机器人 `id`。
- 实际生效的场景配置快照，后续可加入配置哈希。
- UE / ROS 地图与 Nav2 配置版本或可追溯标识。
- 结束原因：成功、导航失败、运行超时、用户取消或初始化失败。
- 耗时和机器人实际累计行驶距离。
- 后续接入的碰撞指标与日志位置。

指标必须先定义再实现：实际行驶距离来自连续位姿增量，不能用最后一条 `/plan` 的长度替代；`/plan` 消息数量不能直接当成“路径改变次数”；LiDAR 最近回波距离不能直接等价于机器人外壳的最小安全间距。

运行超时会触发 Action 取消，但报告结束原因仍是 `TimedOut`，不应被最终 `Canceled` 状态覆盖。报告字段的完整格式在结果采集模块阶段单独确定。

## 10. 扩展边界与版本策略

后续需要时再设计：

- 动态障碍物：稳定 ID、初始位姿、移动轨迹、时间触发、速度与复位规则。
- 批次：输入场景列表、重复次数、参数变化和每轮结果关联。
- 多机器人：允许多个 robots 元素，同时增加命名空间、TF 和 Nav2 资源绑定。
- 其他输入格式：转换为同一内部场景数据，不让执行器依赖 JSON API。

当前文件中不要提前写 `events`、`repeat_count`、`ros_namespace` 等未实现字段。当前版本遇到它们应明确报错，而不是假装支持。

改变字段含义、单位、必填条件或增加执行能力时，应更新 `schema_version` 并同步解析器和文档。旧文件迁移应显式完成，不能静默按新含义执行。

## 11. 推荐实现顺序与第一步验收

1. 定义位姿、机器人配置和场景配置结构体。
2. 实现文件读取、严格字段提取与参数校验。
3. 用正确文件和错误文件验证，只打印解析结果，不移动机器人。
4. 接入场景执行器：绑定 Actor、取消旧任务、清理、复位、应用参数。
5. 接入目标发送、结束状态、超时和结果输出。
6. 单轮稳定后，再扩展动态事件与重复运行。

第一步验收：正确文件可读取；缺字段、错误类型、未知字段、不支持的版本、两个机器人、非法参数能给出准确错误；失败不会修改原数据或当前仿真世界。

本次仅确定文档约定，不代表上述运行、重置、报告与校验能力已经实现。
