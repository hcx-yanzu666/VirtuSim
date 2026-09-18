// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LidarScanState.h"
#include "LidarParameters.h"
#include "Containers/Queue.h"
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

	/** 每帧发布到期扫描；独立于扫描周期，保留原始采样时间戳。 */
	void PublishReadyScans(double CurrentTimeSeconds);

	/** 当前生效的雷达参数。整组一起编辑、一起校验、一起替换。 */
	UPROPERTY(EditAnywhere, Category = "Robot|LiDAR", meta = (ShowOnlyInnerProperties))
	FLidarParameters Parameters;

	/** 距离噪声使用的独立随机流。相同种子可以复现相同噪声序列。 */
    FRandomStream NoiseRandomStream;

	/** 丢点模型使用的独立随机流，避免噪声开关改变丢点序列*/
	FRandomStream DropoutRandomStream;

	struct FPendingLidarScan
	{
		FLidarScanState ScanState; //一帧完整的扫描数据
		double PublishAtSeconds = 0.0; // 允许发布的绝对世界时间戳
	};
	TQueue<FPendingLidarScan, EQueueMode::Spsc> PendingScans;

	/** 30Hz、500ms 正常延迟约需 15 帧；预留余量，满队列时丢弃最旧帧。 */
	static constexpr int32 MaxPendingScanCount = 32;

	/** TQueue 不提供 Num()，在 Game Thread 上随入队、出队和清空同步维护。 */
	int32 PendingScanCount = 0;
    /** 根据当前参数重新初始化所有随机流。 */
    void ResetRandomStreams();

	/** 距离上一次扫描已经累计的时间，单位为秒。属于运行时状态，不是可配置参数。 */
	float scanElapsedSeconds = 0.0f;

	/**
	 * 生成均值为 0、标准差为 1 的标准高斯随机数。
	 * 只从 NoiseRandomStream 取数，因此同一种子必然复现同一噪声序列。
	 * 乘以 NoiseStdDevMeters 即可得到任意标准差的噪声。
	 */
	float GenerateStandardNormalSample();

	FLidarScanState ScanState;
};
