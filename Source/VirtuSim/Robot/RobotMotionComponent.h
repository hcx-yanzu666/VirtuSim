// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
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
};
