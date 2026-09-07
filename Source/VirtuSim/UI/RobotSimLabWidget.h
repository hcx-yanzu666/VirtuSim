#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/SWidget.h"

#include "RobotSimLabWidget.generated.h"

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

private:
	/** 响应设置导航点操作，并转交给当前 PlayerController 的选点流程。 */
	UFUNCTION()
	void HandleSetGoalClicked();

	bool bHasBuiltLayout = false;
};
