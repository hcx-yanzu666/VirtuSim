#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

class UTempoROSNode;
class URobotMotionComponent;
struct FRobotOdomState;
struct FLidarScanState;

#include "RosCommunicationSubsystem.generated.h"

UCLASS(BlueprintType)
class VIRTUSIM_API URosCommunicationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /**
     * 仅在游戏运行 World 中创建通信子系统，避免编辑器加载地图时启动 ROS2。
     */
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    /**
     * 初始化当前 World 的 ROS2 通信探针。
     */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /**
     * 关闭当前 World 的 ROS2 通信探针。
     */
    virtual void Deinitialize() override;

    /**
     * 发布一条字符串测试消息。
     * @param Message 要发布的测试内容。
     * @return 发布成功返回 true。
     */
    UFUNCTION(BlueprintCallable, Category = "ROS")
    bool PublishTestMessage(const FString& Message);

    /**
     * 发布机器人里程计状态到 /odom。
     * @param State UE 内部保存的里程计状态，包含位置、朝向、线速度、角速度和时间戳。
     * @return 发布成功返回 true。
     */
    bool PublishOdom(const FRobotOdomState& State);

    /**
     * 发布一帧 LiDAR 扫描结果到 /scan。
     * @param State UE 内部保存的扫描结果，包含角度、量程、距离数组和时间戳。
     * @return 发布成功返回 true。
     */
    bool PublishScan(const FLidarScanState& State);

    /**
     * 发布 odom 到 base_link 的动态 TF。
     * @param WorldTransform 机器人 Actor 当前在 UE 世界中的位姿，当前阶段约定为 base_link 在 odom 下的位姿。
     * @param Time 当前 UE 世界时间，供 TF 按时间查询。
     * @return 发布成功返回 true。
     */
    bool PublishOdomTransform(const FTransform& WorldTransform, double Time);

    /**
     * 发布 map 到 odom 的静态 TF。
     * 第一版先让 map 和 odom 完全重合，保证 Nav2/RViz 需要的 TF 树完整。
     * @return 发布成功返回 true。
     */
    bool PublishMapStaticTransform();

    /**
     * 发布 base_link 到 laser_link 的静态 TF。
     * 该变换表示 LiDAR 相对机器人底盘的固定安装位置。
     * @return 发布成功返回 true。
     */
    bool PublishLaserStaticTransform();

    /**
     * 发布 UE 中选定的导航目标到 ROS。
    * @param GoalLocation UE 世界坐标，单位厘米。
    * @param GoalYawDegrees UE 中的目标朝向，单位度。
    * @return 发布成功返回 true。
    */
    UFUNCTION(BlueprintCallable, Category = "ROS")
    bool PublishNavigationGoal(const FVector& GoalLocation, float GoalYawDegrees = 0.0f);
    

    /**
     * 为机器人运动组件注册 /cmd_vel 订阅。
     * @param MotionComponent 接收速度命令的运动组件。
     * @return 注册成功返回 true。
     */
    bool AddCmdVelSubscriber(URobotMotionComponent* MotionComponent);

private:
    /** 创建并初始化 TempoROS 节点。 */
    bool CreateRosNode();

    /** 注册字符串测试话题的发布者。 */
    bool AddTestPublisher();

    /** 注册字符串测试话题的订阅者。 */
    bool AddTestSubscriber();

    /** 注册 /odom 发布者。 */
    bool AddOdomPublisher();

    /** 注册 /scan 发布者。 */
    bool AddScanPublisher();

    /** 注册 /virtusim/goal_pose 发布者。 */
    bool AddNavigationGoalPublisher();
private:
    UPROPERTY()
    UTempoROSNode* RosNode = nullptr;

    bool bReady = false;
};
