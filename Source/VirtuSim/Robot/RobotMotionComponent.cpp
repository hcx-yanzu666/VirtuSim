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

	if (const UWorld* World = GetWorld(); World != nullptr)
	{
		lastCmdTimeSeconds = World->GetTimeSeconds();
	}
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

	const UWorld* World = GetWorld();
	if (World != nullptr && cmdVelTimeoutSeconds > 0.0f)
	{
		const double NowSeconds = World->GetTimeSeconds();
		if (lastCmdTimeSeconds > 0.0 && (NowSeconds - lastCmdTimeSeconds) > cmdVelTimeoutSeconds)
		{
			currentLinearX = 0.0;
			currentAngularZ = 0.0;
		}
	}

	const FVector PreviousLocation = OwnerActor->GetActorLocation();
	const FVector MovementForward = OwnerActor->GetActorForwardVector();
	if (!FMath::IsNearlyZero(currentLinearX) || !FMath::IsNearlyZero(currentAngularZ))
	{
		// 线速度 * 帧时间 = 本帧位移距离。
		// GetActorForwardVector 表示 Actor 当前正前方，所以机器人会沿自身朝向前进。
		const FVector DeltaLocation = MovementForward * static_cast<float>(currentLinearX * DeltaTime);
		// 当前阶段暂不让 UE 碰撞体阻挡导航运动；Nav2 负责根据 /scan 和 costmap
		// 规划避障。接入 URDF 并校准精准 footprint 后，再恢复 Sweep 碰撞验证。
		OwnerActor->AddActorWorldOffset(DeltaLocation, false);

		// FRotator(Pitch, Yaw, Roll)，地面机器人平面运动只需要修改 Yaw。
		// 当前阶段保留转向；精准碰撞和旋转碰撞留到 URDF/footprint 校准后验证。
		const FRotator DeltaRotation(0.0f, static_cast<float>(currentAngularZ * DeltaTime), 0.0f);
		OwnerActor->AddActorWorldRotation(DeltaRotation);
	}

	// 移动完成后再更新内部 odom 状态，确保保存的是最新位姿。
	// 真值里程计使用实际位移，避免被障碍挡住后仍报告指令线速度。
	currentOdom.LinearX = DeltaTime > SMALL_NUMBER
		? FVector::DotProduct(OwnerActor->GetActorLocation() - PreviousLocation, MovementForward) / DeltaTime
		: 0.0;
	currentOdom.AngularZ = currentAngularZ;
	currentOdom.Position = OwnerActor->GetActorLocation();
	currentOdom.Rotation = OwnerActor->GetActorRotation();
	currentOdom.TimestampSeconds = World != nullptr ? World->GetTimeSeconds() : 0.0;

	odomPublishElapsedSeconds += DeltaTime;

	if (odomPublishElapsedSeconds >= odomPublishIntervalSeconds)
	{
		if (World != nullptr)
		{
			if (URosCommunicationSubsystem* RosSubsystem = World->GetSubsystem<URosCommunicationSubsystem>())
			{
				RosSubsystem->PublishOdom(currentOdom);
				//取TF动态发布
				RosSubsystem->PublishOdomTransform(OwnerActor->GetActorTransform(), World->GetTimeSeconds());
			}
		}

		odomPublishElapsedSeconds = 0.0f;
	}
}
	
