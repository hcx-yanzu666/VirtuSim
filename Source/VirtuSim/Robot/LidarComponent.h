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

	/** 每秒执行多少次扫描。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float scanFrequencyHz = 10.0f;

    /** 相邻射线的角度间隔，单位度。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "0.5", ClampMax = "5.0"))
    float angleIncrementDegrees = 1.0f;

	  /** 最大探测距离，单位米。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "1.0", ClampMax = "30.0"))
    float rangeMaxMeters = 10.0f;

	/** 最小探测距离，单位米。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "0.01", ClampMax = "1.0"))
    float rangeMinMeters = 0.1f;

    /** 扫描起始角度，单位度。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "-180.0", ClampMax = "0.0"))
    float angleMinDegrees = -180.0f;

    /** 扫描结束角度，单位度。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ClampMin = "0.0", ClampMax = "180.0"))
    float angleMaxDegrees = 180.0f;

    /** 是否绘制调试射线。只影响 UE 显示，不影响 ROS 数据。 */
    UPROPERTY(EditAnywhere, Category = "Robot|LiDAR")
    bool bDrawDebugRays = true;

	/** 距离上一次扫描已经累计的时间，单位为秒。 */
	float scanElapsedSeconds = 0.0f;

	FLidarScanState ScanState;
};
