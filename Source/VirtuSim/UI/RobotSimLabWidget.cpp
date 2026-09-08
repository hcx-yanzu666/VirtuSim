#include "RobotSimLabWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "../Communication/RosCommunicationSubsystem.h"
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
	HeaderRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("仿真监控    传感器调试    场景管理    数据分析    系统设置"), 15, kLabelColor));
	HeaderRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("    ROS2: Connected"), 14, kSuccessColor));
	AddCanvasWidgetWithOffsets(RootCanvas, HeaderPanel, FMargin(16.0f, 12.0f, 16.0f, 54.0f), FAnchors(0.0f, 0.0f, 1.0f, 0.0f));

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
	AddActionButton(WidgetTree, TaskCardContent, TEXT("取消导航"), kDangerColor)->SetIsEnabled(false);
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

	UBorder* ViewTitlePanel = CreatePanel(WidgetTree);
	ViewTitlePanel->SetPadding(FMargin(10.0f, 6.0f));
	ViewTitlePanel->SetContent(CreateText(WidgetTree, TEXT("3D 仿真视图  |  Warehouse_01"), 15, kTitleColor));
	AddCanvasWidget(RootCanvas, ViewTitlePanel, FVector2D(302.0f, 80.0f), FVector2D(400.0f, 36.0f));

	UBorder* FooterPanel = CreatePanel(WidgetTree);
	FooterPanel->SetPadding(FMargin(14.0f, 7.0f));
	UHorizontalBox* FooterRow = WidgetTree->ConstructWidget<UHorizontalBox>();
	FooterPanel->SetContent(FooterRow);
	FooterRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("状态：系统已就绪，点击“设置导航点”后在场景中选择目标位置"), 13, kLabelColor));
	USpacer* FooterSpacer = WidgetTree->ConstructWidget<USpacer>();
	FooterRow->AddChildToHorizontalBox(FooterSpacer)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	FooterRow->AddChildToHorizontalBox(CreateText(WidgetTree, TEXT("FPS: --    LiDAR: 10 Hz    ROS2: Online"), 13, kLabelColor));
	AddCanvasWidgetWithOffsets(RootCanvas, FooterPanel, FMargin(16.0f, 0.0f, 16.0f, 38.0f), FAnchors(0.0f, 1.0f, 1.0f, 1.0f), FVector2D(0.0f, 1.0f));

	return Super::RebuildWidget();
}

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
