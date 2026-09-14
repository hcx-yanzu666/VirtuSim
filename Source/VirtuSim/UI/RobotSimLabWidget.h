#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/SWidget.h"

#include "RobotSimLabWidget.generated.h"
class UTextBlock;
class UWidget;
class UButton;
class UCheckBox;
class USlider;
class USpinBox;

/**
 * 仿真运行时控制台。
 * 使用原生 UMG 生成覆盖在 3D 视口周围的操作面板，避免中心区域遮挡仿真场景。
 */
UCLASS()
class VIRTUSIM_API URobotSimLabWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	/**
	 * 在 Slate 视图树创建前构建 UMG 控件树。
	 * 根节点必须在此阶段建立，NativeConstruct 时再赋值不会刷新已创建的 Slate 内容。
	 */
	virtual TSharedRef<SWidget> RebuildWidget() override;

	/**
	 * 每帧更新运行时界面。
	 * @param MyGeometry 当前控件的布局信息。
	 * @param InDeltaTime 距离上一帧经过的秒数。
	 */
	virtual void NativeTick(
		const FGeometry& MyGeometry,
		float InDeltaTime) override;

private:
	/** 切换回导航监控页面。 */
	UFUNCTION()
	void HandleShowNavigationClicked();

	/** 显示由 UMGAutoBuilder 生成的传感器调试页面。 */
	UFUNCTION()
	void HandleShowSensorDebugClicked();

	/** 恢复传感器页面生成时记录的参数默认值。 */
	UFUNCTION()
	void HandleResetSensorParametersClicked();

	/** 读取、校验并把当前 UI 参数应用到场景中的 LiDAR 组件。 */
	UFUNCTION()
	void HandleApplySensorParametersClicked();

	UFUNCTION() void HandleSensorSlider0Changed(float Value);
	UFUNCTION() void HandleSensorSlider1Changed(float Value);
	UFUNCTION() void HandleSensorSlider2Changed(float Value);
	UFUNCTION() void HandleSensorSlider3Changed(float Value);
	UFUNCTION() void HandleSensorSlider4Changed(float Value);
	UFUNCTION() void HandleSensorSlider5Changed(float Value);
	UFUNCTION() void HandleSensorSlider6Changed(float Value);
	UFUNCTION() void HandleSensorSlider7Changed(float Value);
	UFUNCTION() void HandleSensorSlider8Changed(float Value);

	UFUNCTION() void HandleSensorSpinBox0Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox1Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox2Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox3Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox4Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox5Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox6Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox7Changed(float Value);
	UFUNCTION() void HandleSensorSpinBox8Changed(float Value);

	/** 响应设置导航点操作，并转交给当前 PlayerController 的选点流程。 */
	UFUNCTION()
	void HandleSetGoalClicked();

	/** 查找生成页面中的参数控件，记录默认值并绑定交互事件。 */
	void InitializeSensorParameterControls(UUserWidget* SensorWidget);

	void SynchronizeSensorParameterFromSlider(int32 ParameterIndex, float Value);
	void SynchronizeSensorParameterFromSpinBox(int32 ParameterIndex, float Value);

	bool bHasBuiltLayout = false;

	/** 导航监控页面中需要一起显示或隐藏的顶层控件。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UWidget>> NavigationPageWidgets;

	/** UMGAutoBuilder 生成的传感器调试页面实例。 */
	UPROPERTY(Transient)
	TObjectPtr<UWidget> SensorDebugPage = nullptr;

	/** 下标相同的 Slider 与 SpinBox 表示同一个参数。 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USlider>> SensorParameterSliders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<USpinBox>> SensorParameterSpinBoxes;

	/** 从 JSON 生成页面时读取的初始值，供“恢复默认”使用。 */
	TArray<float> SensorParameterDefaultValues;

	UPROPERTY(Transient)
	TObjectPtr<USpinBox> RandomSeedSpinBox = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCheckBox> DrawScanPointsCheckBox = nullptr;

	ECheckBoxState DrawScanPointsDefaultState = ECheckBoxState::Unchecked;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> SensorControlStatusText = nullptr;

	float RandomSeedDefaultValue = 0.0f;
	bool bSynchronizingSensorParameter = false;

	/** 导航任务状态对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NavigationStatusText = nullptr;

	/** 导航目标 ROS X 坐标对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoalXText = nullptr;

	/** 导航目标 ROS Y 坐标对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoalYText = nullptr;

	/** 机器人当前 ROS X 坐标对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RobotXText = nullptr;

	/** 机器人当前 ROS Y 坐标对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RobotYText = nullptr;

	/** 机器人当前 ROS 偏航角对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> RobotYawText = nullptr;

	/** 最近一次线速度指令对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CommandLinearText = nullptr;

	/** 最近一次角速度指令对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CommandAngularText = nullptr;

	/** 缓存已经显示的状态，避免每帧重复设置相同文本。 */
	FString DisplayedNavigationStatus;

	/** 缓存已经显示的 UE 导航目标，避免每帧重复格式化坐标文本。 */
	FVector DisplayedNavigationGoal = FVector::ZeroVector;

	/** UI 是否已经显示过一个有效导航目标。 */
	bool bHasDisplayedNavigationGoal = false;

	/** 已显示里程计状态的时间戳，用于按 /odom 更新频率刷新机器人状态。 */
	double DisplayedOdomTimestampSeconds = -1.0;
};
