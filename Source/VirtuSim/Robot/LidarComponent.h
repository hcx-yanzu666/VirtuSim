// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LidarScanState.h"
#include "LidarParameters.h"
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

	/**
	 * 校验并应用运行时扫描参数。仅在整组参数合法时一次性替换，避免留下部分生效状态。
	 * 校验规则住在 FLidarParameters::Validate 内部，这里不再逐个字段检查。
	 * @param InParameters 待应用的完整参数组。
	 * @param OutError 参数非法时返回可直接展示给用户的错误信息。
	 * @return 参数合法并已生效返回 true。
	 */
	bool ApplyRuntimeParameters(const FLidarParameters& InParameters, FString& OutError);

	/** 读取当前生效的参数，供 UI 初始化控件或回显使用。 */
	const FLidarParameters& GetParameters() const { return Parameters; }

private:
	/** 执行一帧完整扫描；后续在这里计算雷达位姿并执行逐束射线检测。 */
	void performScan();

	/** 当前生效的雷达参数。整组一起编辑、一起校验、一起替换。 */
	UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ShowOnlyInnerProperties))
	FLidarParameters Parameters;

	/** 距离上一次扫描已经累计的时间，单位为秒。属于运行时状态，不是可配置参数。 */
	float scanElapsedSeconds = 0.0f;

	FLidarScanState ScanState;
};
