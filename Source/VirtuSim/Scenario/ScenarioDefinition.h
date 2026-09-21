#pragma once

#include "CoreMinimal.h"
#include "../Robot/LidarParameters.h"

/**
 * 场景中的平面位姿，使用 UE 世界坐标；不在配置层转换为 ROS 坐标。
 * 普通 C++ 数据结构，不需要 UObject 生命周期或蓝图反射。
 */
struct FScenarioPose
{
    /** JSON x/y/z：单位厘米。起点表示 Actor 原点，而不是碰撞体底部。 */
    FVector LocationCentimeters = FVector::ZeroVector;

    /** JSON yaw：UE 朝向，单位度，合法范围 [-180, 180]；第一版 Pitch/Roll 为 0。 */
    float YawDegrees = 0.0f;
};

/** 一个机器人的测试输入；不保存 Actor 指针、实时位姿或导航任务状态。 */
struct FScenarioRobotDefinition
{
    /** JSON id：用于配置和结果报告的逻辑标识，不自动生成 ROS namespace。 */
    FString Id;

    /**
     * JSON actor_tag：匹配关卡机器人 Actor Tags，不是 Actor Label 或 Component Tags。
     * 保留原始字符串以便加载器校验；执行器负责确保恰好匹配一个机器人。
     */
    FString ActorTag;

    /** JSON start：初始化时恢复的位姿，Z 必须根据模型原点与地面校准。 */
    FScenarioPose Start;

    /** JSON goal：导航目标，执行时交给现有导航接口进行 ROS 坐标转换。 */
    FScenarioPose Goal;

    /**
     * JSON lidar：复用现有参数类型与 Validate()，避免维护重复的参数定义。
     * 第一版文件必须明确提供约定中的全部实验参数，不能用默认值掩盖缺字段。
     * 显示开关、点大小不属于场景输入，由展示逻辑单独管理。
     */
    FLidarParameters LidarParameters;
};

/**
 * 一轮场景的输入定义，与 JSON API 和执行逻辑解耦。
 * Loader 负责解析、校验，全部成功后输出；Runner 负责应用和任务生命周期。
 * 成员初始化仅保证构造安全，不代表默认对象已通过校验或可以直接执行。
 */
struct FScenarioDefinition
{
    /** JSON schema_version：第一版只接受整数 1；0 表示尚未加载。 */
    int32 SchemaVersion = 0;

    /** JSON scenario_id：场景标识；每次执行生成的 run_id 另存于运行结果。 */
    FString ScenarioId;

    /** JSON description：可选的人类可读说明，缺省为空。 */
    FString Description;

    /** JSON coordinate_system：第一版只接受 ue_world_cm_deg。 */
    FString CoordinateSystem;

    /**
     * JSON timeout_seconds：有限且大于 0，从导航目标被接受时开始计算的仿真秒数。
     * 不包含初始化等待；通信与取消确认的墙钟超时由执行器另外管理。
     */
    double TimeoutSeconds = 0.0;

    /** JSON robots：第一版必须恰好有一个元素；不允许静默忽略多余机器人。 */
    TArray<FScenarioRobotDefinition> Robots;
};
