#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

class UTempoROSNode;
class URobotMotionComponent;
struct FRobotOdomState;

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
private:
    UPROPERTY()
    UTempoROSNode* RosNode = nullptr;

    bool bReady = false;
};
