#include "RosCommunicationSubsystem.h"

#include "TempoROSCommonConverters.h"
#include "TempoROSNode.h"
#include "rclcpp/utilities.hpp"

// UE 类型与 ROS2 类型之间的转换
// FString ↔ std_msgs::msg::String
// FVector ↔ geometry_msgs::msg::Vector3
// FRotator ↔ geometry_msgs::msg::Quaternion
// FTransform ↔ geometry_msgs::msg::Transform
// FTwist ↔ geometry_msgs::msg::Twist

namespace
{
const FString kTestTopic = TEXT("/virtusim/test");
}

bool URosCommunicationSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    const UWorld* World = Cast<UWorld>(Outer);
    return World != nullptr && World->IsGameWorld();
}

void URosCommunicationSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bReady = false;

    if (!CreateRosNode())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针创建节点失败"));
        return;
    }

    if (!AddTestPublisher())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针创建发布者失败，Topic=%s"), *kTestTopic);
        RosNode = nullptr;
        return;
    }

    if (!AddTestSubscriber())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针创建订阅者失败，Topic=%s"), *kTestTopic);
        RosNode = nullptr;
        return;
    }

    bReady = true;
    UE_LOG(LogTemp, Display, TEXT("ROS通信探针初始化成功，Node=virtusim_probe，Topic=%s"), *kTestTopic);
}

void URosCommunicationSubsystem::Deinitialize()
{
    bReady = false;
    RosNode = nullptr;

    Super::Deinitialize();
}

bool URosCommunicationSubsystem::PublishTestMessage(const FString& Message)
{
    if (!bReady || RosNode == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针未就绪，无法发布测试消息"));
        return false;
    }

    const bool bPublished = RosNode->Publish<FString>(kTestTopic, Message);
    if (!bPublished)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS测试消息发布失败，Topic=%s，Message=%s"), *kTestTopic, *Message);
        return false;
    }

    UE_LOG(LogTemp, Display, TEXT("已发布ROS测试消息，Topic=%s，Message=%s"), *kTestTopic, *Message);
    return true;
}

bool URosCommunicationSubsystem::CreateRosNode()
{
    if (!rclcpp::ok())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS运行时未初始化，跳过TempoROS节点创建"));
        return false;
    }

    RosNode = UTempoROSNode::Create(TEXT("virtusim_probe"), this, true);
    return RosNode != nullptr;
}

bool URosCommunicationSubsystem::AddTestPublisher()
{
    if (RosNode == nullptr)
    {
        return false;
    }

    return RosNode->AddPublisher<FString>(kTestTopic, FROSQOSProfile(), false);
}

bool URosCommunicationSubsystem::AddTestSubscriber()
{
    if (RosNode == nullptr)
    {
        return false;
    }

    const auto MessageCallback = TROSSubscriptionDelegate<FString>::CreateLambda(
        [](const FString& Message)
        {
            UE_LOG(LogTemp, Display, TEXT("收到ROS测试消息，Topic=%s，Message=%s"), *kTestTopic, *Message);
        });

    return RosNode->AddSubscription<FString>(kTestTopic, MessageCallback);
}
