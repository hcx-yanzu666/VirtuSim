#include "RosCommunicationSubsystem.h"

#include "TempoROSCommonConverters.h"
#include "TempoROSNode.h"
#include "../Robot/RobotMotionComponent.h"
#include "../Robot/RobotOdomConverters.h"
#include "../Robot/LidarScanConverters.h"
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
const FString kCmdVelTopic = TEXT("/cmd_vel");
const FString kOdomTopic = TEXT("/odom");
const FString kScanTopic = TEXT("/scan");

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

    if (!AddOdomPublisher())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针创建发布者失败，Topic=%s"), *kOdomTopic);
        RosNode = nullptr;
        return;
    }
    if (!AddScanPublisher())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针创建发布者失败，Topic=%s"), *kScanTopic);
        RosNode = nullptr;
        return;
    }

    if (!AddTestSubscriber())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针创建订阅者失败，Topic=%s"), *kTestTopic);
        RosNode = nullptr;
        return;
    }
    if (!PublishMapStaticTransform())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针发布静态TF失败，From=map，To=odom"));
        RosNode = nullptr;
        return;
    }

    if (!PublishLaserStaticTransform())
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针发布静态TF失败，From=base_link，To=laser_link"));
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

bool URosCommunicationSubsystem::PublishOdom(const FRobotOdomState& State)
{
    if (!bReady || RosNode == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针未就绪，无法发布里程计消息"));
        return false;
    }

    const bool bPublished = RosNode->Publish<FRobotOdomState>(kOdomTopic, State);
    if (!bPublished)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS里程计发布失败，Topic=%s"), *kOdomTopic);
        return false;
    }

    return true;
}

bool URosCommunicationSubsystem::PublishScan(const FLidarScanState& State)
{
    if (!bReady || RosNode == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针未就绪，无法发布激光扫描消息"));
        return false;
    }

    const bool bPublished = RosNode->Publish<FLidarScanState>(kScanTopic, State);
    if (!bPublished)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS激光扫描发布失败，Topic=%s"), *kScanTopic);
        return false;
    }

    return true;
}

bool URosCommunicationSubsystem::PublishOdomTransform(const FTransform& WorldTransform, double Time)
{
    if (RosNode == nullptr)
    {
        return false;
    }
    return (RosNode->PublishDynamicTransform(WorldTransform, "base_link", "odom", Time));
}

bool URosCommunicationSubsystem::PublishMapStaticTransform()
{
    if (RosNode == nullptr)
    {
        return false;
    }

    // 第一版没有 AMCL/SLAM 修正，先让 map 与 odom 重合，保证 TF 树完整可用。
    return RosNode->PublishStaticTransform(FTransform::Identity, "odom", "map");
}

bool URosCommunicationSubsystem::PublishLaserStaticTransform()
{
    if (RosNode == nullptr)
    {
        return false;
    }
    const FTransform BaseToLaser(
        FRotator::ZeroRotator,
        FVector(20.0f, 0.0f, 30.0f),
        FVector::OneVector
    );
    return (RosNode->PublishStaticTransform(BaseToLaser, "laser_link", "base_link"));
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

bool URosCommunicationSubsystem::AddOdomPublisher()
{
    if (RosNode == nullptr)
    {
        return false;
    }

    return RosNode->AddPublisher<FRobotOdomState>(kOdomTopic, FROSQOSProfile(), false);
}

bool URosCommunicationSubsystem::AddScanPublisher()
{
    if (RosNode == nullptr)
    {
        return false;
    }
    return RosNode->AddPublisher<FLidarScanState>(kScanTopic, FROSQOSProfile(), false);
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

bool URosCommunicationSubsystem::AddCmdVelSubscriber(URobotMotionComponent* MotionComponent)
{
    if (RosNode == nullptr || MotionComponent == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("ROS通信探针注册/cmd_vel失败，RosNode或MotionComponent为空"));
        return false;
    }

    const TWeakObjectPtr<URobotMotionComponent> WeakMotionComponent(MotionComponent);
    const auto MessageCallback = TROSSubscriptionDelegate<FTwist>::CreateLambda(
        [WeakMotionComponent](const FTwist& Msg)
        {
            if (!WeakMotionComponent.IsValid())
            {
                return;
            }

            WeakMotionComponent->SetCmdVel(Msg.LinearVelocity.X, Msg.AngularVelocity.Z);
        });

    const bool bAdded = RosNode->AddSubscription<FTwist>(kCmdVelTopic, MessageCallback);
    if (bAdded)
    {
        UE_LOG(LogTemp, Display, TEXT("ROS通信探针已注册/cmd_vel订阅，Topic=%s"), *kCmdVelTopic);
    }
    return bAdded;
}
