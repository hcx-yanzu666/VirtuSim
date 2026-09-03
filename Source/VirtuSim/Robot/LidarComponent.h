// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LidarScanState.h"
#include "LidarComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class VIRTUSIM_API ULidarComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	ULidarComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** 执行一帧完整扫描；后续在这里计算雷达位姿并执行逐束射线检测。 */
	void performScan();

	/** 每秒执行的完整扫描次数。 */
	UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "0.1"))
	float scanFrequencyHz = 10.0f;

	/** 距离上一次扫描已经累计的时间，单位为秒。 */
	float scanElapsedSeconds = 0.0f;

	FLidarScanState ScanState;
};
