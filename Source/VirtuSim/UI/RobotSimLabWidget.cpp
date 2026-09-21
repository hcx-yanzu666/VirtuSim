#include "RobotSimLabWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/CheckBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/UserWidget.h"
#include "../Communication/RosCommunicationSubsystem.h"
#include "../Robot/LidarComponent.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

namespace
{
const FLinearColor kPanelColor(0.018f, 0.031f, 0.050f, 0.92f);
const FLinearColor kCardColor(0.045f, 0.075f, 0.105f, 0.96f);
const FLinearColor kPrimaryColor(0.02f, 0.40f, 0.56f, 1.0f);
const FLinearColor kDangerColor(0.42f, 0.12f, 0.15f, 1.0f);
const FLinearColor kDisabledColor(0.08f, 0.12f, 0.16f, 0.94f);
const FLinearColor kTitleColor(0.90f, 0.95f, 0.98f, 1.0f);
const FLinearColor kLabelColor(0.56f, 0.67f, 0.74f, 1.0f);
const FLinearColor kValueColor(0.85f, 0.92f, 0.96f, 1.0f);
const FLinearColor kSuccessColor(0.16f, 0.88f, 0.45f, 1.0f);
const FLinearColor kWarningColor(0.95f, 0.72f, 0.18f, 1.0f);
constexpr double kCentimetersPerMeter = 100.0;

FText GetNavigationStatusText(const FString& Status)
{
    if (Status == TEXT("Canceling"))
    {
        return FText::FromString(TEXT("正在取消导航"));
    }
    if (Status == TEXT("CancelFailed"))
    {
        return FText::FromString(TEXT("取消未确认，可重试"));
    }
	if (Status == TEXT("Idle"))
	{
		return FText::FromString(TEXT("空闲"));
	}
	if (Status == TEXT("GoalReceived"))
	{
		return FText::FromString(TEXT("已收到目标"));
	}
	if (Status == TEXT("GoalAccepted"))
	{
		return FText::FromString(TEXT("目标已接受"));
	}
	if (Status == TEXT("Navigating"))
	{
		return FText::FromString(TEXT("导航中"));
	}
	if (Status == TEXT("Succeeded"))
	{
		return FText::FromString(TEXT("导航成功"));
	}
	if (Status == TEXT("Failed"))
	{
		return FText::FromString(TEXT("导航失败"));
	}
	if (Status == TEXT("Canceled"))
	{
		return FText::FromString(TEXT("导航已取消"));
	}

	return FText::FromString(Status);
}

FLinearColor GetNavigationStatusColor(const FString& Status)
{
	if (Status == TEXT("Succeeded"))
	{
		return kSuccessColor;
	}
	if (Status == TEXT("Failed"))
	{
		return kDangerColor;
	}
	if (Status == TEXT("Canceled"))
	{
		return kWarningColor;
	}
	if (Status == TEXT("GoalReceived") ||
		Status == TEXT("GoalAccepted") ||
		Status == TEXT("Navigating"))
	{
		return kPrimaryColor;
	}

	return kValueColor;
}

UTextBlock* CreateText(UWidgetTree* WidgetTree, const FString& Text, int32 FontSize, const FLinearColor& Color)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
	TextBlock->SetText(FText::FromString(Text));
	TextBlock->SetColorAndOpacity(Color);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

UBorder* CreatePanel(UWidgetTree* WidgetTree)
{
	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>();
	Panel->SetBrushColor(kPanelColor);
	Panel->SetPadding(FMargin(12.0f));
	return Panel;
}

UBorder* CreateCard(UWidgetTree* WidgetTree, UWidget* Content)
{
	UBorder* Card = WidgetTree->ConstructWidget<UBorder>();
	Card->SetBrushColor(kCardColor);
	Card->SetPadding(FMargin(12.0f));
	Card->SetContent(Content);
	return Card;
}

void AddPanelTitle(UWidgetTree* WidgetTree, UVerticalBox* Container, const FString& Title)
{
	UTextBlock* TitleText = CreateText(WidgetTree, Title, 16, kTitleColor);
	Container->AddChildToVerticalBox(TitleText)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
}

UTextBlock* AddStatusRow(
    UWidgetTree* WidgetTree,
    UVerticalBox* Container,
    const FString& Label,
    const FString& Value,
    const FLinearColor& ValueColor = kValueColor)
{
    UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
    UTextBlock* LabelText = CreateText(
        WidgetTree,
        Label,
        13,
        kLabelColor);

    UTextBlock* ValueText = CreateText(
        WidgetTree,
        Value,
        13,
        ValueColor);

    ValueText->SetJustification(ETextJustify::Right);

    UHorizontalBoxSlot* LabelSlot =
        Row->AddChildToHorizontalBox(LabelText);

    LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
    Row->AddChildToHorizontalBox(ValueText);

    Container->AddChildToVerticalBox(Row)
        ->SetPadding(FMargin(0.0f, 2.0f));

    return ValueText;
}

UButton* AddActionButton(UWidgetTree* WidgetTree, UVerticalBox* Container, const FString& Label, const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(Color);
	Button->SetClickMethod(EButtonClickMethod::MouseDown);

	UTextBlock* LabelText = CreateText(WidgetTree, Label, 15, FLinearColor::White);
	LabelText->SetJustification(ETextJustify::Center);
	Button->AddChild(LabelText);
	Container->AddChildToVerticalBox(Button)->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
	return Button;
}

UButton* CreateHeaderButton(UWidgetTree* WidgetTree, const FString& Label, const FLinearColor& Color)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>();
	Button->SetBackgroundColor(Color);
	Button->SetClickMethod(EButtonClickMethod::MouseDown);

	UTextBlock* LabelText = CreateText(WidgetTree, Label, 14, kTitleColor);
	LabelText->SetJustification(ETextJustify::Center);
	Button->AddChild(LabelText);
	return Button;
}

void AddCanvasWidget(UCanvasPanel* RootCanvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size, const FAnchors& Anchors = FAnchors(0.0f, 0.0f), const FVector2D& Alignment = FVector2D::ZeroVector)
{
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Widget);
	Slot->SetAnchors(Anchors);
	Slot->SetAlignment(Alignment);
	Slot->SetPosition(Position);
	Slot->SetSize(Size);
}

void AddCanvasWidgetWithOffsets(UCanvasPanel* RootCanvas, UWidget* Widget, const FMargin& Offsets, const FAnchors& Anchors, const FVector2D& Alignment = FVector2D::ZeroVector)
{
	UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Widget);
	Slot->SetAnchors(Anchors);
	Slot->SetAlignment(Alignment);
	Slot->SetOffsets(Offsets);
}
}

TSharedRef<SWidget> URobotSimLabWidget::RebuildWidget()
{
	if (bHasBuiltLayout)
	{
		return Super::RebuildWidget();
	}

	bHasBuiltLayout = true;

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RobotSimLabRoot"));
	WidgetTree->RootWidget = RootCanvas;

	UBorder* HeaderPanel = CreatePanel(WidgetTree);
	UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	HeaderPanel->SetContent(HeaderRow);
	HeaderRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("Robot Sim Lab"), 22, kTitleColor));
	HeaderRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("  v0.1.0"), 12, kLabelColor));
	USpacer* HeaderSpacer = WidgetTree->ConstructWidget<USpacer>();
	HeaderRow->AddChildToHorizontalBox(HeaderSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* NavigationPageButton = CreateHeaderButton(WidgetTree, TEXT("导航监控"), kPrimaryColor);
	NavigationPageButton->OnClicked.AddDynamic(this, &URobotSimLabWidget::HandleShowNavigationClicked);
	HeaderRow->AddChildToHorizontalBox(NavigationPageButton)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

	UButton* SensorDebugButton = CreateHeaderButton(WidgetTree, TEXT("传感器调试"), kCardColor);
	SensorDebugButton->OnClicked.AddDynamic(this, &URobotSimLabWidget::HandleShowSensorDebugClicked);
	HeaderRow->AddChildToHorizontalBox(SensorDebugButton)->SetPadding(FMargin(0.0f, 0.0f, 16.0f, 0.0f));
	HeaderRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("    ROS2: Connected"), 14, kSuccessColor));
	AddCanvasWidgetWithOffsets(RootCanvas, HeaderPanel, FMargin(16.0f, 12.0f, 16.0f, 54.0f), FAnchors(0.0f, 0.0f, 1.0f, 0.0f));
	CastChecked<UCanvasPanelSlot>(HeaderPanel->Slot)->SetZOrder(10);

	UBorder* LeftPanel = CreatePanel(WidgetTree);
	UVerticalBox* LeftContent = WidgetTree->ConstructWidget<UVerticalBox>();
	LeftPanel->SetContent(LeftContent);
	UVerticalBox* TaskCardContent = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPanelTitle(WidgetTree, TaskCardContent, TEXT("任务控制"));
	AddStatusRow(WidgetTree, TaskCardContent, TEXT("导航任务"), TEXT("运行中"), kSuccessColor);
	NavigationStatusText = AddStatusRow(
		WidgetTree,
		TaskCardContent,
		TEXT("任务状态"),
		TEXT("空闲"));
	AddStatusRow(WidgetTree, TaskCardContent, TEXT("当前目标"), TEXT("未设置"));
	UButton* SetGoalButton = AddActionButton(WidgetTree, TaskCardContent, TEXT("设置导航点"), kPrimaryColor);
	SetGoalButton->OnClicked.AddDynamic(this, &URobotSimLabWidget::HandleSetGoalClicked);
	UButton* CancelButton = AddActionButton(WidgetTree, TaskCardContent, TEXT("取消导航"), kDangerColor);
	CancelButton->OnClicked.AddDynamic(this, &URobotSimLabWidget::HandleCancelNavigationClicked);
	LeftContent->AddChildToVerticalBox(CreateCard(WidgetTree, TaskCardContent))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	UVerticalBox* GoalCardContent = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPanelTitle(WidgetTree, GoalCardContent, TEXT("导航目标"));
	AddStatusRow(WidgetTree, GoalCardContent, TEXT("坐标系"), TEXT("map"));
	GoalXText = AddStatusRow(WidgetTree, GoalCardContent, TEXT("目标 X"), TEXT("-- m"));
	GoalYText = AddStatusRow(WidgetTree, GoalCardContent, TEXT("目标 Y"), TEXT("-- m"));
	AddStatusRow(WidgetTree, GoalCardContent, TEXT("目标朝向"), TEXT("0.0 deg"));
	LeftContent->AddChildToVerticalBox(CreateCard(WidgetTree, GoalCardContent))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));

	UVerticalBox* RobotCardContent = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPanelTitle(WidgetTree, RobotCardContent, TEXT("机器人状态"));
	RobotXText = AddStatusRow(WidgetTree, RobotCardContent, TEXT("当前位置 X"), TEXT("-- m"));
	RobotYText = AddStatusRow(WidgetTree, RobotCardContent, TEXT("当前位置 Y"), TEXT("-- m"));
	RobotYawText = AddStatusRow(WidgetTree, RobotCardContent, TEXT("当前朝向"), TEXT("-- deg"));
	CommandLinearText = AddStatusRow(WidgetTree, RobotCardContent, TEXT("指令线速度"), TEXT("-- m/s"));
	CommandAngularText = AddStatusRow(WidgetTree, RobotCardContent, TEXT("指令角速度"), TEXT("-- rad/s"));
	LeftContent->AddChildToVerticalBox(CreateCard(WidgetTree, RobotCardContent));
	AddCanvasWidget(RootCanvas, LeftPanel, FVector2D(16.0f, 80.0f), FVector2D(270.0f, 700.0f));
	NavigationPageWidgets.Add(LeftPanel);

	UBorder* RightPanel = CreatePanel(WidgetTree);
	UVerticalBox* RightContent = WidgetTree->ConstructWidget<UVerticalBox>();
	RightPanel->SetContent(RightContent);
	UVerticalBox* SystemCardContent = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPanelTitle(WidgetTree, SystemCardContent, TEXT("系统状态"));
	AddStatusRow(WidgetTree, SystemCardContent, TEXT("ROS2"), TEXT("Connected"), kSuccessColor);
	AddStatusRow(WidgetTree, SystemCardContent, TEXT("Nav2"), TEXT("Ready"), kSuccessColor);
	AddStatusRow(WidgetTree, SystemCardContent, TEXT("仿真时间"), TEXT("Running"));
	RightContent->AddChildToVerticalBox(CreateCard(WidgetTree, SystemCardContent))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	UVerticalBox* SensorCardContent = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPanelTitle(WidgetTree, SensorCardContent, TEXT("传感器状态"));
	AddStatusRow(WidgetTree, SensorCardContent, TEXT("LiDAR"), TEXT("Online"), kSuccessColor);
	AddStatusRow(WidgetTree, SensorCardContent, TEXT("扫描范围"), TEXT("10.0 m"));
	AddStatusRow(WidgetTree, SensorCardContent, TEXT("扫描角度"), TEXT("360 deg"));
	RightContent->AddChildToVerticalBox(CreateCard(WidgetTree, SensorCardContent))->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 10.0f));
	UVerticalBox* LidarCardContent = WidgetTree->ConstructWidget<UVerticalBox>();
	AddPanelTitle(WidgetTree, LidarCardContent, TEXT("LiDAR 参数"));
	AddStatusRow(WidgetTree, LidarCardContent, TEXT("扫描频率"), TEXT("10.0 Hz"));
	AddStatusRow(WidgetTree, LidarCardContent, TEXT("角度分辨率"), TEXT("1.0 deg"));
	AddStatusRow(WidgetTree, LidarCardContent, TEXT("噪声模型"), TEXT("未启用"));
	AddActionButton(WidgetTree, LidarCardContent, TEXT("参数调节（下一阶段）"), kDisabledColor)->SetIsEnabled(false);
	RightContent->AddChildToVerticalBox(CreateCard(WidgetTree, LidarCardContent));
	AddCanvasWidget(RootCanvas, RightPanel, FVector2D(-16.0f, 80.0f), FVector2D(280.0f, 560.0f), FAnchors(1.0f, 0.0f), FVector2D(1.0f, 0.0f));
	NavigationPageWidgets.Add(RightPanel);

	UBorder* ViewTitlePanel = CreatePanel(WidgetTree);
	ViewTitlePanel->SetPadding(FMargin(10.0f, 6.0f));
	ViewTitlePanel->SetContent(CreateText(WidgetTree, TEXT("3D 仿真视图  |  Warehouse_01"), 15, kTitleColor));
	AddCanvasWidget(RootCanvas, ViewTitlePanel, FVector2D(302.0f, 80.0f), FVector2D(400.0f, 36.0f));
	NavigationPageWidgets.Add(ViewTitlePanel);

	UBorder* FooterPanel = CreatePanel(WidgetTree);
	FooterPanel->SetPadding(FMargin(14.0f, 7.0f));
	UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	FooterPanel->SetContent(FooterRow);
	FooterRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("状态：系统已就绪，点击“设置导航点”后在场景中选择目标位置"), 13, kLabelColor));
	USpacer* FooterSpacer = WidgetTree->ConstructWidget<USpacer>();
	FooterRow->AddChildToHorizontalBox(FooterSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	FooterRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("FPS: --    LiDAR: 10 Hz    ROS2: Online"), 13, kLabelColor));
	AddCanvasWidgetWithOffsets(RootCanvas, FooterPanel, FMargin(16.0f, 0.0f, 16.0f, 38.0f), FAnchors(0.0f, 1.0f, 1.0f, 1.0f), FVector2D(0.0f, 1.0f));
	NavigationPageWidgets.Add(FooterPanel);

	// 传感器页面由 UMGAutoBuilder 从 JSON 生成；主 Widget 只负责加载和切换。
	UClass* SensorDebugWidgetClass = LoadClass<UUserWidget>(
		nullptr,
		TEXT("/Game/UI/Widgets/WBP_RobotSensorDebug.WBP_RobotSensorDebug_C"));
	if (SensorDebugWidgetClass != nullptr)
	{
		UUserWidget* SensorWidget = WidgetTree->ConstructWidget<UUserWidget>(
			SensorDebugWidgetClass,
			TEXT("SensorDebugPage"));
		SensorDebugPage = SensorWidget;
		SensorDebugPage->SetVisibility(ESlateVisibility::Collapsed);
		InitializeSensorParameterControls(SensorWidget);

		UCanvasPanelSlot* SensorSlot = RootCanvas->AddChildToCanvas(SensorDebugPage);
		SensorSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		SensorSlot->SetOffsets(FMargin(0.0f));
		SensorSlot->SetZOrder(5);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("无法加载传感器调试页面 WBP_RobotSensorDebug"));
	}

	return Super::RebuildWidget();
}

void URobotSimLabWidget::HandleShowNavigationClicked()
{
	for (UWidget* Widget : NavigationPageWidgets)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(ESlateVisibility::Visible);
		}
	}

	if (SensorDebugPage != nullptr)
	{
		SensorDebugPage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void URobotSimLabWidget::HandleCancelNavigationClicked()
{
    URosCommunicationSubsystem* Ros = GetWorld() ? GetWorld()->GetSubsystem<URosCommunicationSubsystem>() : nullptr;
    const bool bSent = Ros && Ros->CancelNavigation();
    if (NavigationStatusText)
    {
        NavigationStatusText->SetText(FText::FromString(bSent
            ? TEXT("取消请求已发送，等待确认") : TEXT("取消请求发送失败")));
    }
}

void URobotSimLabWidget::HandleShowSensorDebugClicked()
{
	for (UWidget* Widget : NavigationPageWidgets)
	{
		if (Widget != nullptr)
		{
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SensorDebugPage != nullptr)
	{
		SensorDebugPage->SetVisibility(ESlateVisibility::Visible);
	}
}

void URobotSimLabWidget::InitializeSensorParameterControls(UUserWidget* SensorWidget)
{
	static const TCHAR* SliderNames[] = {
		TEXT("ScanFrequencySlider"),
		TEXT("MaximumRangeSlider"),
		TEXT("MinimumRangeSlider"),
		TEXT("HorizontalFovSlider"),
		TEXT("AngleResolutionSlider"),
		TEXT("NoiseStdDevSlider"),
		TEXT("DropoutProbabilitySlider"),
		TEXT("FixedDelaySlider"),
		TEXT("ScanPointSizeSlider")
	};
	static const TCHAR* SpinBoxNames[] = {
		TEXT("ScanFrequencySpinBox"),
		TEXT("MaximumRangeSpinBox"),
		TEXT("MinimumRangeSpinBox"),
		TEXT("HorizontalFovSpinBox"),
		TEXT("AngleResolutionSpinBox"),
		TEXT("NoiseStdDevSpinBox"),
		TEXT("DropoutProbabilitySpinBox"),
		TEXT("FixedDelaySpinBox"),
		TEXT("ScanPointSizeSpinBox")
	};

	SensorParameterSliders.Reset();
	SensorParameterSpinBoxes.Reset();
	SensorParameterDefaultValues.Reset();

	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SliderNames); ++Index)
	{
		USlider* Slider = Cast<USlider>(SensorWidget->GetWidgetFromName(SliderNames[Index]));
		USpinBox* SpinBox = Cast<USpinBox>(SensorWidget->GetWidgetFromName(SpinBoxNames[Index]));
		if (Slider == nullptr || SpinBox == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("传感器参数控件不完整：%s / %s"), SliderNames[Index], SpinBoxNames[Index]);
			return;
		}

		SensorParameterSliders.Add(Slider);
		SensorParameterSpinBoxes.Add(SpinBox);
		SensorParameterDefaultValues.Add(SpinBox->GetValue());
	}

	SensorParameterSliders[0]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider0Changed);
	SensorParameterSliders[1]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider1Changed);
	SensorParameterSliders[2]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider2Changed);
	SensorParameterSliders[3]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider3Changed);
	SensorParameterSliders[4]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider4Changed);
	SensorParameterSliders[5]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider5Changed);
	SensorParameterSliders[6]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider6Changed);
	SensorParameterSliders[7]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider7Changed);
	SensorParameterSliders[8]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSlider8Changed);

	SensorParameterSpinBoxes[0]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox0Changed);
	SensorParameterSpinBoxes[1]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox1Changed);
	SensorParameterSpinBoxes[2]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox2Changed);
	SensorParameterSpinBoxes[3]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox3Changed);
	SensorParameterSpinBoxes[4]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox4Changed);
	SensorParameterSpinBoxes[5]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox5Changed);
	SensorParameterSpinBoxes[6]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox6Changed);
	SensorParameterSpinBoxes[7]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox7Changed);
	SensorParameterSpinBoxes[8]->OnValueChanged.AddDynamic(this, &URobotSimLabWidget::HandleSensorSpinBox8Changed);

	RandomSeedSpinBox = Cast<USpinBox>(SensorWidget->GetWidgetFromName(TEXT("RandomSeedSpinBox")));
	if (RandomSeedSpinBox != nullptr)
	{
		RandomSeedDefaultValue = RandomSeedSpinBox->GetValue();
	}
	NoiseEnabledCheckBox = Cast<UCheckBox>(SensorWidget->GetWidgetFromName(TEXT("NoiseEnabledCheckBox")));
	if (NoiseEnabledCheckBox != nullptr)
	{
		NoiseEnabledDefaultState = NoiseEnabledCheckBox->GetCheckedState();
	}
	DrawScanPointsCheckBox = Cast<UCheckBox>(SensorWidget->GetWidgetFromName(TEXT("DrawScanPointsCheckBox")));
	if (DrawScanPointsCheckBox != nullptr)
	{
		DrawScanPointsDefaultState = DrawScanPointsCheckBox->GetCheckedState();
	}
	SensorControlStatusText = Cast<UTextBlock>(SensorWidget->GetWidgetFromName(TEXT("SensorControlStatus")));

	if (UButton* ResetButton = Cast<UButton>(SensorWidget->GetWidgetFromName(TEXT("ResetParametersButton"))))
	{
		ResetButton->OnClicked.AddDynamic(this, &URobotSimLabWidget::HandleResetSensorParametersClicked);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("找不到传感器参数重置按钮 ResetParametersButton"));
	}

	if (UButton* ApplyButton = Cast<UButton>(SensorWidget->GetWidgetFromName(TEXT("ApplyParametersButton"))))
	{
		ApplyButton->OnClicked.AddDynamic(this, &URobotSimLabWidget::HandleApplySensorParametersClicked);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("找不到传感器参数应用按钮 ApplyParametersButton"));
	}
}

void URobotSimLabWidget::SynchronizeSensorParameterFromSlider(const int32 ParameterIndex, const float Value)
{
	if (bSynchronizingSensorParameter || !SensorParameterSpinBoxes.IsValidIndex(ParameterIndex))
	{
		return;
	}

	TGuardValue<bool> SynchronizationGuard(bSynchronizingSensorParameter, true);
	SensorParameterSpinBoxes[ParameterIndex]->SetValue(Value);
}

void URobotSimLabWidget::SynchronizeSensorParameterFromSpinBox(const int32 ParameterIndex, const float Value)
{
	if (bSynchronizingSensorParameter || !SensorParameterSliders.IsValidIndex(ParameterIndex))
	{
		return;
	}

	TGuardValue<bool> SynchronizationGuard(bSynchronizingSensorParameter, true);
	SensorParameterSliders[ParameterIndex]->SetValue(Value);
}

void URobotSimLabWidget::HandleResetSensorParametersClicked()
{
	TGuardValue<bool> SynchronizationGuard(bSynchronizingSensorParameter, true);
	for (int32 Index = 0; Index < SensorParameterDefaultValues.Num(); ++Index)
	{
		if (SensorParameterSliders.IsValidIndex(Index) && SensorParameterSpinBoxes.IsValidIndex(Index))
		{
			SensorParameterSliders[Index]->SetValue(SensorParameterDefaultValues[Index]);
			SensorParameterSpinBoxes[Index]->SetValue(SensorParameterDefaultValues[Index]);
		}
	}

	if (RandomSeedSpinBox != nullptr)
	{
		RandomSeedSpinBox->SetValue(RandomSeedDefaultValue);
	}
	if (DrawScanPointsCheckBox != nullptr)
	{
		DrawScanPointsCheckBox->SetCheckedState(DrawScanPointsDefaultState);
	}
	if (NoiseEnabledCheckBox != nullptr)
	{
		NoiseEnabledCheckBox->SetCheckedState(NoiseEnabledDefaultState);
	}
	if (SensorControlStatusText != nullptr)
	{
		SensorControlStatusText->SetText(FText::FromString(TEXT("已恢复默认值，点击应用后生效")));
		SensorControlStatusText->SetColorAndOpacity(kLabelColor);
	}
}

void URobotSimLabWidget::HandleApplySensorParametersClicked()
{
	constexpr int32 ScanFrequencyIndex = 0;
	constexpr int32 MaximumRangeIndex = 1;
	constexpr int32 MinimumRangeIndex = 2;
	constexpr int32 HorizontalFovIndex = 3;
	constexpr int32 AngleResolutionIndex = 4;
	constexpr int32 NoiseStdDevIndex = 5;
	constexpr int32 DropoutProbabilityIndex = 6;
	constexpr int32 FixedDelayIndex = 7;
	constexpr int32 ScanPointSizeIndex = 8;

	if (SensorParameterSpinBoxes.Num() <= ScanPointSizeIndex ||
		NoiseEnabledCheckBox == nullptr || DrawScanPointsCheckBox == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("无法应用 LiDAR 参数：传感器 UI 控件未完整初始化"));
		if (SensorControlStatusText != nullptr)
		{
			SensorControlStatusText->SetText(FText::FromString(TEXT("应用失败：UI 控件未初始化")));
			SensorControlStatusText->SetColorAndOpacity(kDangerColor);
		}
		return;
	}

	ULidarComponent* LidarComponent = nullptr;
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> ActorIt(World); ActorIt; ++ActorIt)
		{
			LidarComponent = ActorIt->FindComponentByClass<ULidarComponent>();
			if (LidarComponent != nullptr)
			{
				break;
			}
		}
	}

	if (LidarComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("无法应用 LiDAR 参数：当前世界中没有 ULidarComponent"));
		if (SensorControlStatusText != nullptr)
		{
			SensorControlStatusText->SetText(FText::FromString(TEXT("应用失败：未找到 LiDAR")));
			SensorControlStatusText->SetColorAndOpacity(kDangerColor);
		}
		return;
	}

	// 先把 UI 上的九个参数装进结构体，再整组交给组件校验。
	// 校验失败时组件不会改动任何字段，界面上的值也保持原样等待修正。
	FLidarParameters NewParameters;
	NewParameters.ScanFrequencyHz = SensorParameterSpinBoxes[ScanFrequencyIndex]->GetValue();
	NewParameters.AngleIncrementDegrees = SensorParameterSpinBoxes[AngleResolutionIndex]->GetValue();
	NewParameters.HorizontalFovDegrees = SensorParameterSpinBoxes[HorizontalFovIndex]->GetValue();
	NewParameters.RangeMinMeters = SensorParameterSpinBoxes[MinimumRangeIndex]->GetValue();
	NewParameters.RangeMaxMeters = SensorParameterSpinBoxes[MaximumRangeIndex]->GetValue();
	NewParameters.bNoiseEnabled = NoiseEnabledCheckBox->GetCheckedState() == ECheckBoxState::Checked;
	NewParameters.NoiseStdDevMeters = SensorParameterSpinBoxes[NoiseStdDevIndex]->GetValue();
	// UI 使用百分比（0~100%），组件参数使用概率（0~1）。
	NewParameters.DropoutProbability =
		SensorParameterSpinBoxes[DropoutProbabilityIndex]->GetValue() / 100.0f;
	NewParameters.FixedDelayMilliseconds = SensorParameterSpinBoxes[FixedDelayIndex]->GetValue();
	NewParameters.ScanPointSizeCentimeters = SensorParameterSpinBoxes[ScanPointSizeIndex]->GetValue();
	NewParameters.bDrawScanPoints = DrawScanPointsCheckBox->GetCheckedState() == ECheckBoxState::Checked;
	if (RandomSeedSpinBox != nullptr)
	{
		NewParameters.RandomSeed = FMath::RoundToInt(RandomSeedSpinBox->GetValue());
	}

	FString ValidationError;
	const bool bApplied = LidarComponent->ApplyRuntimeParameters(NewParameters, ValidationError);

	if (!bApplied)
	{
		UE_LOG(LogTemp, Warning, TEXT("LiDAR 参数校验失败：%s"), *ValidationError);
		if (SensorControlStatusText != nullptr)
		{
			SensorControlStatusText->SetText(FText::FromString(FString::Printf(TEXT("应用失败：%s"), *ValidationError)));
			SensorControlStatusText->SetColorAndOpacity(kDangerColor);
		}
		return;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("LiDAR 参数已应用：频率=%.2fHz，量程=%.2f-%.2fm，视场=%.1fdeg，分辨率=%.2fdeg，")
		TEXT("噪声=%s/%.3fm，丢点=%.1f%%，延迟=%.1fms，种子=%d"),
		NewParameters.ScanFrequencyHz,
		NewParameters.RangeMinMeters,
		NewParameters.RangeMaxMeters,
		NewParameters.HorizontalFovDegrees,
		NewParameters.AngleIncrementDegrees,
		NewParameters.bNoiseEnabled ? TEXT("开启") : TEXT("关闭"),
		NewParameters.NoiseStdDevMeters,
		NewParameters.DropoutProbability * 100.0f,
		NewParameters.FixedDelayMilliseconds,
		NewParameters.RandomSeed);
	if (SensorControlStatusText != nullptr)
	{
		SensorControlStatusText->SetText(FText::FromString(TEXT("参数已应用")));
		SensorControlStatusText->SetColorAndOpacity(kSuccessColor);
	}
}

#define DEFINE_SENSOR_PARAMETER_HANDLERS(Index) \
	void URobotSimLabWidget::HandleSensorSlider##Index##Changed(const float Value) { SynchronizeSensorParameterFromSlider(Index, Value); } \
	void URobotSimLabWidget::HandleSensorSpinBox##Index##Changed(const float Value) { SynchronizeSensorParameterFromSpinBox(Index, Value); }

DEFINE_SENSOR_PARAMETER_HANDLERS(0)
DEFINE_SENSOR_PARAMETER_HANDLERS(1)
DEFINE_SENSOR_PARAMETER_HANDLERS(2)
DEFINE_SENSOR_PARAMETER_HANDLERS(3)
DEFINE_SENSOR_PARAMETER_HANDLERS(4)
DEFINE_SENSOR_PARAMETER_HANDLERS(5)
DEFINE_SENSOR_PARAMETER_HANDLERS(6)
DEFINE_SENSOR_PARAMETER_HANDLERS(7)
DEFINE_SENSOR_PARAMETER_HANDLERS(8)

#undef DEFINE_SENSOR_PARAMETER_HANDLERS

void URobotSimLabWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (NavigationStatusText == nullptr || GoalXText == nullptr || GoalYText == nullptr ||
		RobotXText == nullptr || RobotYText == nullptr || RobotYawText == nullptr ||
		CommandLinearText == nullptr || CommandAngularText == nullptr)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}

	const URosCommunicationSubsystem* RosSubsystem =
		World->GetSubsystem<URosCommunicationSubsystem>();
	if (RosSubsystem == nullptr)
	{
		return;
	}

	const FString CurrentStatus = RosSubsystem->GetNavigationStatus();
	if (CurrentStatus != DisplayedNavigationStatus)
	{
		DisplayedNavigationStatus = CurrentStatus;
		NavigationStatusText->SetText(GetNavigationStatusText(CurrentStatus));
		NavigationStatusText->SetColorAndOpacity(GetNavigationStatusColor(CurrentStatus));
	}

	FVector CurrentGoal;
	if (RosSubsystem->TryGetLastNavigationGoal(CurrentGoal) &&
		(!bHasDisplayedNavigationGoal || !CurrentGoal.Equals(DisplayedNavigationGoal)))
	{
		// 与 NavigationGoalConverters 使用相同规则：UE 厘米转 ROS 米，并翻转 Y 轴。
		const double GoalXMetres = CurrentGoal.X / kCentimetersPerMeter;
		const double GoalYMetres = -CurrentGoal.Y / kCentimetersPerMeter;

		GoalXText->SetText(FText::FromString(FString::Printf(TEXT("%.2f m"), GoalXMetres)));
		GoalYText->SetText(FText::FromString(FString::Printf(TEXT("%.2f m"), GoalYMetres)));

		DisplayedNavigationGoal = CurrentGoal;
		bHasDisplayedNavigationGoal = true;
	}

	FRobotOdomState CurrentOdom;
	if (!RosSubsystem->TryGetLatestOdom(CurrentOdom) ||
		FMath::IsNearlyEqual(CurrentOdom.TimestampSeconds, DisplayedOdomTimestampSeconds))
	{
		return;
	}

	// 位姿和速度均按 ROS 的右手坐标系及标准单位显示。
	const double RobotXMetres = CurrentOdom.Position.X / kCentimetersPerMeter;
	const double RobotYMetres = -CurrentOdom.Position.Y / kCentimetersPerMeter;
	const double RobotYawDegrees = FMath::UnwindDegrees(-CurrentOdom.Rotation.Yaw);
	const double CommandLinearMetresPerSecond = CurrentOdom.LinearX / kCentimetersPerMeter;
	const double CommandAngularRadiansPerSecond = FMath::DegreesToRadians(-CurrentOdom.AngularZ);

	RobotXText->SetText(FText::FromString(FString::Printf(TEXT("%.2f m"), RobotXMetres)));
	RobotYText->SetText(FText::FromString(FString::Printf(TEXT("%.2f m"), RobotYMetres)));
	RobotYawText->SetText(FText::FromString(FString::Printf(TEXT("%.1f deg"), RobotYawDegrees)));
	CommandLinearText->SetText(FText::FromString(FString::Printf(TEXT("%.2f m/s"), CommandLinearMetresPerSecond)));
	CommandAngularText->SetText(FText::FromString(FString::Printf(TEXT("%.2f rad/s"), CommandAngularRadiansPerSecond)));

	DisplayedOdomTimestampSeconds = CurrentOdom.TimestampSeconds;
}

void URobotSimLabWidget::HandleSetGoalClicked()
{
	APlayerController* PlayerController = GetOwningPlayer();
	if (PlayerController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("导航控制台无法开始选点：未找到当前 PlayerController"));
		return;
	}

	UFunction* StartPickGoalFunction = PlayerController->FindFunction(TEXT("StartPickGoal"));
	if (StartPickGoalFunction == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("导航控制台无法开始选点：PlayerController 未实现 StartPickGoal"));
		return;
	}

	PlayerController->ProcessEvent(StartPickGoalFunction, nullptr);
}
