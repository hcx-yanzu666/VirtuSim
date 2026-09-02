// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RobotOdomState.h"
#include "RobotMotionComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class VIRTUSIM_API URobotMotionComponent : public UActorComponent
{
	GENERATED_BODY()

protected:
    // Called when the game starts or when spawned
    virtual void BeginPlay() override;

public:	
	URobotMotionComponent();

	/**
	 * 更新最新的 /cmd_vel 指令。
	 * 这里只负责接收和保存，不直接做运动计算。
	 * @param InLinearX 线速度，单位厘米每秒。
	 * @param InAngularZ 角速度，单位度每秒。
	 */
	void SetCmdVel(double InLinearX, double InAngularZ);

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** 当前线速度，单位厘米每秒。 */
	double currentLinearX = 0.0;

	/** 当前角速度，单位度每秒。 */
	double currentAngularZ = 0.0;

	/** 最近一次收到 /cmd_vel 的世界时间。 */
	double lastCmdTimeSeconds = 0.0;

	/** 命令超时后自动刹停的时间阈值。 */
	UPROPERTY(EditAnywhere, Category = "Robot|Motion")
	float cmdVelTimeoutSeconds = 0.5f;

	/** 当前内部里程计状态。 */
	FRobotOdomState currentOdom;
	float odomPublishIntervalSeconds = 1.0f /30.0f; //30帧发一次 /odom
	float odomPublishElapsedSeconds = 0.0f; //距离上一次发布过了多久
};
