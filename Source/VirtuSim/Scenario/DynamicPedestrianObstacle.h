#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DynamicPedestrianObstacle.generated.h"

class UCapsuleComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class AActor;
struct FScenarioPedestrianDefinition;

/**
 * 可重复的固定轨迹行人障碍物。
 *
 * 胶囊体是 LiDAR 使用的查询代理，骨骼网格只负责视觉表现。
 * 场景 JSON 可直接提供起点和终点；未由场景控制时才使用蓝图中的相对终点偏移。
 */
UCLASS()
class VIRTUSIM_API ADynamicPedestrianObstacle : public AActor
{
	GENERATED_BODY()

public:
	ADynamicPedestrianObstacle();
	void ApplyScenarioParameters(const FScenarioPedestrianDefinition& Definition);
	void ResetToStart();
	void StartMovement();

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

	/** 用于寻找机器人 Actor 的标签；与机器人场景配置中的标签保持一致。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pedestrian|Yield", meta = (AllowPrivateAccess = "true"))
	FName RobotActorTag = FName(TEXT("ScenarioRobot_01"));

	FVector StartLocation = FVector::ZeroVector;
	FVector EndLocation = FVector::ZeroVector;
	float ResumeClearElapsed = 0.0f;
	bool bMovingToEnd = true;
	bool bStopped = false;
	bool bYieldingToRobot = false;
	bool bScenarioControlled = false;
	TWeakObjectPtr<AActor> RobotActor;

	void ApplyMeshAsset();
	/** 暂停或恢复骨骼动画；Actor 停止时保留当前姿态，避免原地继续走路。 */
	void SetPedestrianAnimationPaused(bool bPaused);
	void HandleEndpointReached();
	void FindRobotActor();
	bool IsRobotBlockingPath(const FVector& CurrentLocation, const FVector& TravelDirection, float DistanceThresholdCm) const;
};
