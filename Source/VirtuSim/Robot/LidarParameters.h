#pragma once

#include "CoreMinimal.h"

#include "LidarParameters.generated.h"

/**
 * 一台二维 LiDAR 的完整可调参数，描述“这个传感器是什么”，不是“这一刻测到了什么”。
 *
 * 与 FLidarScanState 的区别：
 *   FLidarParameters  长期不变的配置，由编辑器或运行时 UI 编辑，单向流入组件。
 *   FLidarScanState   一帧扫描快照，由组件产生，单向流向 ROS 转换器。
 * 组件在扫描时读取本结构体并把需要的量复制进 FLidarScanState，两者之间不互相引用，
 * 避免参数被改动时影响已经生成、正在延迟队列中等待发布的历史帧。
 *
 * 单位约定：字段名后缀即单位。对外统一使用米、度、秒这类 ROS/物理侧习惯单位，
 * 厘米与弧度的转换集中在组件填充 FLidarScanState 时完成，本结构体内不出现混用。
 */
USTRUCT(BlueprintType)
struct FLidarParameters
{
	GENERATED_BODY()

	// ---------------------------------------------------------------------
	// 几何与采样：决定扫描“打在哪里、多久打一次”
	// ---------------------------------------------------------------------

	/** 每秒执行多少次完整扫描。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Sampling", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float ScanFrequencyHz = 10.0f;

	/** 相邻两条射线的角度间隔，单位度。越小射线越密，单帧开销越大。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Sampling", meta = (ClampMin = "0.5", ClampMax = "5.0"))
	float AngleIncrementDegrees = 1.0f;

	/**
	 * 以雷达正前方为中心的水平视场角，单位度。
	 * 这里只存视场角这一个真相源，扫描起止角由 GetAngleMinDegrees/GetAngleMaxDegrees 派生，
	 * 避免“视场角”和“起止角”各存一份后出现不一致。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Sampling", meta = (ClampMin = "30.0", ClampMax = "360.0"))
	float HorizontalFovDegrees = 360.0f;

	/** 最小有效探测距离，单位米。近于该距离的回波视为无效。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Range", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float RangeMinMeters = 0.1f;

	/** 最大有效探测距离，单位米。超出该距离按无回波处理。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Range", meta = (ClampMin = "1.0", ClampMax = "30.0"))
	float RangeMaxMeters = 10.0f;

	// ---------------------------------------------------------------------
	// 误差模型：决定测量“准不准、丢不丢、及不及时”
	// 三项互相独立，实验时应逐项单独扫描，避免同时变动导致无法归因。
	// ---------------------------------------------------------------------

	/**
	 * 测距高斯噪声的标准差，单位米。0 表示理想无噪声。
	 * 噪声沿光束径向叠加，只让障碍物显得更近或更远，不会让它横向平移。
	 * 量级小于 costmap 分辨率时通常会被 inflation layer 吸收，成功率不受影响。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Error", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NoiseStdDevMeters = 0.0f;

	/**
	 * 单条命中射线丢失回波的概率，取值 0~1。0 表示不丢点。
	 * 只作用于本来打中物体的射线：没打中的射线已经在报告“无回波”，再丢一次没有物理意义。
	 * 丢点结果按最大量程表达，与真实打空完全一致——真实雷达同样无法区分
	 * “被黑色材质吸收”和“射向空旷处”，两者在接收端都只是没有回波。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Error", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DropoutProbability = 0.0f;

	/**
	 * 从扫描发生到扫描数据被发布的固定延迟，单位毫秒。0 表示立即发布。
	 * 正确语义是“延迟送达、保留原始时间戳”：消息在 T+Delay 发出，header.stamp 仍为 T，
	 * 这样下游才会用 T 时刻的 TF 去解释它。若把时间戳也改成 T+Delay，
	 * 下游会用当前位姿变换过去测得的数据，得到的是实现缺陷而不是延迟的真实影响。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Error", meta = (ClampMin = "0.0", ClampMax = "500.0"))
	float FixedDelayMilliseconds = 0.0f;

	// ---------------------------------------------------------------------
	// 实验控制：不是传感器的物理属性，真实雷达没有“随机种子”这种东西。
	// 放在这里是因为它和上面的误差模型共同决定一次实验能否复现，
	// 但读代码时要清楚它属于实验装置，不属于被测对象。
	// ---------------------------------------------------------------------

	/**
	 * 随机数种子。相同种子 + 相同参数 + 相同轨迹会得到完全相同的扫描序列。
	 * 噪声与丢点必须使用由该种子派生出的两条独立随机流，
	 * 否则只调整其中一项会让另一项的随机序列整体错位，破坏单变量实验的前提。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Experiment", meta = (ClampMin = "0"))
	int32 RandomSeed = 0;

	// ---------------------------------------------------------------------
	// 可视化：只影响 UE 内的调试显示，不参与 ROS 数据生成
	// ---------------------------------------------------------------------

	/** 是否绘制沿扫描方向排列的调试采样点。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Visualization")
	bool bDrawScanPoints = true;

	/** 调试采样点的显示直径，单位厘米。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LiDAR|Visualization", meta = (ClampMin = "1.0", ClampMax = "12.0"))
	float ScanPointSizeCentimeters = 5.0f;

	// ---------------------------------------------------------------------
	// 派生量与校验
	// ---------------------------------------------------------------------

	/** 扫描起始角，由视场角派生，单位度。 */
	float GetAngleMinDegrees() const
	{
		return -HorizontalFovDegrees * 0.5f;
	}

	/** 扫描结束角，由视场角派生，单位度。 */
	float GetAngleMaxDegrees() const
	{
		return HorizontalFovDegrees * 0.5f;
	}

	/**
	 * 校验全部字段是否构成一组合法参数。
	 * 校验集中在结构体内部，新增字段时只需要改这里一处，
	 * 调用方无需知道有多少个参数、各自的取值范围是什么。
	 * @param OutError 校验失败时返回可直接展示给用户的错误信息。
	 * @return 全部字段合法返回 true。
	 */
	bool Validate(FString& OutError) const
	{
		if (!FMath::IsWithinInclusive(ScanFrequencyHz, 1.0f, 30.0f))
		{
			OutError = TEXT("扫描频率必须在 1 到 30 Hz 之间");
			return false;
		}
		if (!FMath::IsWithinInclusive(AngleIncrementDegrees, 0.5f, 5.0f))
		{
			OutError = TEXT("角度分辨率必须在 0.5 到 5 度之间");
			return false;
		}
		if (!FMath::IsWithinInclusive(RangeMinMeters, 0.01f, 1.0f))
		{
			OutError = TEXT("最小量程必须在 0.01 到 1 米之间");
			return false;
		}
		if (!FMath::IsWithinInclusive(RangeMaxMeters, 1.0f, 30.0f))
		{
			OutError = TEXT("最大量程必须在 1 到 30 米之间");
			return false;
		}
		if (RangeMinMeters >= RangeMaxMeters)
		{
			OutError = TEXT("最小量程必须小于最大量程");
			return false;
		}
		if (!FMath::IsWithinInclusive(HorizontalFovDegrees, 30.0f, 360.0f))
		{
			OutError = TEXT("水平视场必须在 30 到 360 度之间");
			return false;
		}
		if (AngleIncrementDegrees > HorizontalFovDegrees)
		{
			OutError = TEXT("角度分辨率不能大于水平视场");
			return false;
		}
		if (!FMath::IsWithinInclusive(ScanPointSizeCentimeters, 1.0f, 12.0f))
		{
			OutError = TEXT("扫描点大小必须在 1 到 12 厘米之间");
			return false;
		}
		if (!FMath::IsWithinInclusive(NoiseStdDevMeters, 0.0f, 1.0f))
		{
			OutError = TEXT("噪声必须在 0 到 1 米之间");
			return false;
		}
		if (!FMath::IsWithinInclusive(DropoutProbability, 0.0f, 1.0f))
		{
			OutError = TEXT("丢点概率必须在 0 到 1 之间");
			return false;
		}
		if (!FMath::IsWithinInclusive(FixedDelayMilliseconds, 0.0f, 500.0f))
		{
			OutError = TEXT("延迟必须在 0 到 500 毫秒之间");
			return false;
		}
		if (RandomSeed < 0)
		{
			OutError = TEXT("随机种子不能为负数");
			return false;
		}
		return true;
	}
};
