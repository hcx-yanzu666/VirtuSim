// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UStaticMeshComponent;
class UStaticMesh;
class URobotMotionComponent;
class ULidarComponent;
class UNavigationPathVisualizerComponent;

#include "RobotBoxActor.generated.h"

UCLASS()
class VIRTUSIM_API ARobotBoxActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARobotBoxActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* robotBodyMesh = nullptr;

	/** 主体显示网格由机器人蓝图指定，避免把某个模型资源写死在 C++ 类中。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Robot|Appearance", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMesh> RobotBodyAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
	URobotMotionComponent* robotMotionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
    ULidarComponent* lidarComponent = nullptr;

	/** 读取通信子系统中的 Nav2 路径并绘制调试线。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
	UNavigationPathVisualizerComponent* navigationPathVisualizerComponent = nullptr;

};
