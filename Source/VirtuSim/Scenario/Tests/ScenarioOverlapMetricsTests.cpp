#include "../ScenarioOverlapMetrics.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenarioOverlapGeometryTest,
    "VirtuSim.Scenario.Overlap.Geometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioOverlapGeometryTest::RunTest(const FString& Parameters)
{
    // 非中心对称底盘，单位 cm；覆盖侧边与角点，避免用扩大矩形误报角点。
    const FBox2D Box(FVector2D(-150.0, -45.0), FVector2D(20.0, 45.0));
    TestTrue(TEXT("Center inside"), IsScenarioCircleOverlappingBox(Box, FVector2D(-50.0, 0.0), 34.0));
    TestTrue(TEXT("Tangent counts"), IsScenarioCircleOverlappingBox(Box, FVector2D(54.0, 0.0), 34.0));
    TestFalse(TEXT("Outside front"), IsScenarioCircleOverlappingBox(Box, FVector2D(55.0, 0.0), 34.0));
    TestFalse(TEXT("Corner outside circle"), IsScenarioCircleOverlappingBox(Box, FVector2D(50.0, 75.0), 34.0));
    TestTrue(TEXT("Corner intersects"), IsScenarioCircleOverlappingBox(Box, FVector2D(40.0, 65.0), 34.0));
    TestFalse(TEXT("Invalid geometry"), IsScenarioCircleOverlappingBox(FBox2D(ForceInit), FVector2D::ZeroVector, 34.0));

    // 转到跟随机器人 yaw 的无缩放坐标系后，旋转/平移不影响判定。
    const FTransform Frame(FRotator(0.0, 90.0, 0.0), FVector(800.0, -500.0, 80.0));
    const FVector WorldCenter = Frame.TransformPosition(FVector(54.0, 0.0, 0.0));
    const FVector Center = Frame.InverseTransformPosition(WorldCenter);
    TestTrue(TEXT("Rotated robot"), IsScenarioCircleOverlappingBox(Box, FVector2D(Center.X, Center.Y), 34.001));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenarioOverlapEpisodesTest,
    "VirtuSim.Scenario.Overlap.EpisodesAndDuration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FScenarioOverlapEpisodesTest::RunTest(const FString& Parameters)
{
    FScenarioOverlapMetrics Metrics;
    Metrics.Sample(false, 10.0);
    Metrics.Sample(true, 11.0);
    Metrics.Sample(true, 12.0);
    Metrics.Sample(true, 12.0); // 同帧终态补采或暂停不重复累计。
    TestEqual(TEXT("Continuous overlap counted once"), Metrics.EpisodeCount, 1);
    TestEqual(TEXT("No duplicate elapsed time"), Metrics.DurationSeconds, 1.0);
    Metrics.Sample(false, 13.0);
    Metrics.Sample(false, 16.0);
    Metrics.Sample(true, 17.0);
    Metrics.Sample(true, 17.5); // 报告落盘时仍重叠，也累计最后一段。
    TestEqual(TEXT("Separation then reentry"), Metrics.EpisodeCount, 2);
    TestEqual(TEXT("Disjoint union duration"), Metrics.DurationSeconds, 2.5);

    FScenarioOverlapMetrics InitiallyOverlapping;
    InitiallyOverlapping.Sample(true, 100.0);
    TestEqual(TEXT("Initial overlap counted"), InitiallyOverlapping.EpisodeCount, 1);
    TestEqual(TEXT("No pre-navigation duration"), InitiallyOverlapping.DurationSeconds, 0.0);
    InitiallyOverlapping.Sample(false, 100.25);
    TestEqual(TEXT("Initial episode duration"), InitiallyOverlapping.DurationSeconds, 0.25);
    return true;
}
#endif
