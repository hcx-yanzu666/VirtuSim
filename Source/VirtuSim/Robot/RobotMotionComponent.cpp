#include "RobotMotionComponent.h"

#include "../Communication/RosCommunicationSubsystem.h"
#include "GameFramework/Actor.h"

URobotMotionComponent::URobotMotionComponent()
{
	// 启用组件 Tick，后面才能每帧把速度积分成位移和转向。
	PrimaryComponentTick.bCanEverTick = true;
}

void URobotMotionComponent::BeginPlay()
{
	Super::BeginPlay();

	// 先拿到当前 World，子系统就挂在 World 上。
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RobotMotionComponent 初始化失败，World 为空"));
		return;
	}

	// 再从 World 里找到 ROS 通信子系统。
	URosCommunicationSubsystem* RosSubsystem = World->GetSubsystem<URosCommunicationSubsystem>();
	if (RosSubsystem == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RobotMotionComponent 初始化失败，未找到 ROS 通信子系统"));
		return;
	}

	// 把自己注册成 /cmd_vel 的接收者。
	if (!RosSubsystem->AddCmdVelSubscriber(this))
	{
		UE_LOG(LogTemp, Error, TEXT("RobotMotionComponent 订阅 /cmd_vel 失败"));
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("RobotMotionComponent 已订阅 /cmd_vel"));
}

void URobotMotionComponent::SetCmdVel(double InLinearX, double InAngularZ)
{
	// ROS 回调只保存最新速度，不直接移动 Actor，避免通讯层和运动层耦合。
	currentLinearX = InLinearX;
	currentAngularZ = InAngularZ;
}

void URobotMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// 组件本身没有世界位置，真正被移动的是挂载这个组件的 Actor。
	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	// 没有速度命令时直接返回，避免每帧做无意义的位移和旋转计算。
	if (FMath::IsNearlyZero(currentLinearX) && FMath::IsNearlyZero(currentAngularZ))
	{
		return;
	}

	// 线速度 * 帧时间 = 本帧位移距离。
	// GetActorForwardVector 表示 Actor 当前正前方，所以机器人会沿自身朝向前进。
	const FVector DeltaLocation = OwnerActor->GetActorForwardVector() * static_cast<float>(currentLinearX * DeltaTime);
	OwnerActor->AddActorWorldOffset(DeltaLocation, false);

	// FRotator(Pitch, Yaw, Roll)，地面机器人平面运动只需要修改 Yaw。
	const FRotator DeltaRotation(0.0f, static_cast<float>(currentAngularZ * DeltaTime), 0.0f);
	OwnerActor->AddActorWorldRotation(DeltaRotation);
}
