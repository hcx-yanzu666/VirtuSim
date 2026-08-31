#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

class UTempoROSNode;

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

private:
    /** 创建并初始化 TempoROS 节点。 */
    bool CreateRosNode();

    /** 注册字符串测试话题的发布者。 */
    bool AddTestPublisher();

    /** 注册字符串测试话题的订阅者。 */
    bool AddTestSubscriber();

private:
    UPROPERTY()
    UTempoROSNode* RosNode = nullptr;

    bool bReady = false;
};
