// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

class UStaticMeshComponent;
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

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* cubeMesh = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
	URobotMotionComponent* robotMotionComponent = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
    ULidarComponent* lidarComponent = nullptr;

	/** 读取通信子系统中的 Nav2 路径并绘制调试线。 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Robot", meta = (AllowPrivateAccess = "true"))
	UNavigationPathVisualizerComponent* navigationPathVisualizerComponent = nullptr;

};
