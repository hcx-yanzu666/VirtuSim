#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/SWidget.h"

#include "RobotSimLabWidget.generated.h"
class UTextBlock;

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
	/** 响应设置导航点操作，并转交给当前 PlayerController 的选点流程。 */
	UFUNCTION()
	void HandleSetGoalClicked();

	bool bHasBuiltLayout = false;

	/** 导航任务状态对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NavigationStatusText = nullptr;

	/** 导航目标 ROS X 坐标对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoalXText = nullptr;

	/** 导航目标 ROS Y 坐标对应的文本控件。 */
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GoalYText = nullptr;

	/** 缓存已经显示的状态，避免每帧重复设置相同文本。 */
	FString DisplayedNavigationStatus;

	/** 缓存已经显示的 UE 导航目标，避免每帧重复格式化坐标文本。 */
	FVector DisplayedNavigationGoal = FVector::ZeroVector;

	/** UI 是否已经显示过一个有效导航目标。 */
	bool bHasDisplayedNavigationGoal = false;
};
