#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicPedestrianObstacle.generated.h"

class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class AActor;

/**
 * 可重复的固定轨迹行人障碍物。
 *
 * 胶囊体是 LiDAR 使用的查询代理，骨骼网格只负责视觉表现。
 * Actor 初始位置为路线起点，EndOffset 为相对起点的终点偏移。
 */
UCLASS()
class VIRTUSIM_API ADynamicPedestrianObstacle : public AActor
{
	GENERATED_BODY()

public:
	ADynamicPedestrianObstacle();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	/** LiDAR 查询代理。该组件只阻挡 Visibility，不参与物理模拟。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> CollisionCapsule = nullptr;

	/** 行人外观；可在蓝图默认值中指定 Manny 或 Quinn。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pedestrian", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> PedestrianMesh = nullptr;

	/** 蓝图指定的骨骼网格资源。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pedestrian|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMesh> PedestrianMeshAsset = nullptr;

	/** 骨骼网格相对于胶囊中心的偏移；默认让脚底落到胶囊底部附近。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Appearance", meta = (AllowPrivateAccess = "true"))
	FVector MeshRelativeLocation = FVector(0.0f, 0.0f, -88.0f);

	/** 网格相对于胶囊的旋转；不同资产的前向轴可能不同，默认不旋转。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Appearance", meta = (AllowPrivateAccess = "true"))
	FRotator MeshRelativeRotation = FRotator::ZeroRotator;

	/** 终点相对 Actor 起点的偏移，单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Movement", meta = (AllowPrivateAccess = "true"))
	FVector EndOffset = FVector(0.0f, 500.0f, 0.0f);

	/** 行走速度，单位为厘米每秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Movement", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float MoveSpeedCmPerSecond = 100.0f;

	/** 到达端点后的停留时间，单位为秒。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Movement", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float EndpointWaitSeconds = 1.0f;

	/** 是否在终点和起点之间持续往返。关闭时到达终点后停止。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Movement", meta = (AllowPrivateAccess = "true"))
	bool bLoop = true;

	/** 是否让行人朝向当前运动方向。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Movement", meta = (AllowPrivateAccess = "true"))
	bool bRotateAlongPath = true;

	/** 用于寻找机器人 Actor 的标签；与机器人场景配置中的标签保持一致。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Yield", meta = (AllowPrivateAccess = "true"))
	FName RobotActorTag = FName(TEXT("ScenarioRobot_01"));

	/** 行人和机器人中心的水平距离小于该值时，行人停止。单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Yield", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float YieldDistanceCm = 180.0f;

	/** 行人恢复移动前需要达到的更大距离。单位为厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Yield", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float ResumeDistanceCm = 260.0f;

	/** 机器人离开后保持安全的时间，避免行人在临界距离反复启停。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Yield", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float ResumeClearSeconds = 0.5f;

	/** 允许机器人位于行人身后的一小段范围，避免行人被身后的机器人触发让行。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Yield", meta = (ClampMin = "0.0", AllowPrivateAccess = "true"))
	float RearAllowanceCm = 50.0f;

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float EndpointWaitRemaining = 0.0f;
	float ResumeClearElapsed = 0.0f;
	bool bMovingToEnd = true;
	bool bStopped = false;
	bool bYieldingToRobot = false;
	TWeakObjectPtr<AActor> RobotActor;

	void ApplyMeshAsset();
	void HandleEndpointReached();
	void FindRobotActor();
	bool IsRobotBlockingPath(const FVector& CurrentLocation, const FVector& TravelDirection, float DistanceThresholdCm) const;
};
