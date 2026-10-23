#include "NavigationMapExporter.h"

#include "Components/BoxComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "HAL/FileManager.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	constexpr float CentimetersPerMeter = 100.0f;

	/** 将 Details 面板中的输出目录统一转换为绝对路径。 */
	FString MakeAbsoluteOutputDirectory(const FString& ConfiguredDirectory)
	{
		return FPaths::IsRelative(ConfiguredDirectory)
			? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ConfiguredDirectory)
			: FPaths::ConvertRelativePathToFull(ConfiguredDirectory);
	}
}

ANavigationMapExporter::ANavigationMapExporter()
{
	// 导出是编辑器按钮触发的一次性操作，不需要每帧执行 Tick。
	PrimaryActorTick.bCanEverTick = false;

	// BoxComponent 同时承担可视化范围框和地图范围数据源的职责。
	MapBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("MapBounds"));
	RootComponent = MapBounds;
	MapBounds->SetBoxExtent(FVector(1200.0f, 800.0f, 100.0f));
	MapBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// 这是编辑器中的地图范围框：运行游戏时隐藏，编辑器中保持可见并可直接拖拽。
	MapBounds->SetHiddenInGame(true);
	MapBounds->ShapeColor = FColor(40, 220, 80, 180);
	MapBounds->bDrawOnlyIfSelected = false;
	MapBounds->SetLineThickness(4.0f);
}

void ANavigationMapExporter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (MapBounds != nullptr)
	{
		// 范围框只用于编辑器选择和导出，不能参与自己的碰撞查询。
		MapBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool ANavigationMapExporter::IsExcludedActor(const AActor* Actor) const
{
	// OverlapResult 可能没有 Actor。没有 Actor 的结果不能作为有效障碍物。
	if (Actor == nullptr)
	{
		return true;
	}

	for (const FName& ExcludedTag : ExcludedActorTags)
	{
		if (!ExcludedTag.IsNone() && Actor->ActorHasTag(ExcludedTag))
		{
			return true;
		}
	}

	return false;
}

bool ANavigationMapExporter::IsOccupied(
	const FVector& WorldCellCenter,
	const FCollisionObjectQueryParams& ObjectQueryParams,
	const FCollisionQueryParams& QueryParams) const
{
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}

	// 允许用户把最小/最大高度填反，内部统一成有效范围。
	const float MinZ = FMath::Min(ObstacleMinZ, ObstacleMaxZ);
	const float MaxZ = FMath::Max(ObstacleMinZ, ObstacleMaxZ);
	// Z 中心取高度范围的中点；不使用 WorldCellCenter.Z。
	const FVector OverlapCenter(WorldCellCenter.X, WorldCellCenter.Y, (MinZ + MaxZ) * 0.5f);
	// MakeBox 接收半尺寸。例如分辨率 0.05m、Z 范围 10~200cm 时，
	// 完整检测盒为 5 × 5 × 190cm，HalfExtent 为 (2.5, 2.5, 95)cm。
	const FVector HalfExtent(
		ResolutionMeters * CentimetersPerMeter * 0.5f,
		ResolutionMeters * CentimetersPerMeter * 0.5f,
		(MaxZ - MinZ) * 0.5f);

	// 一个栅格被扩展成三维盒子：XY 是一个像素大小，Z 是障碍检测高度。
	TArray<FOverlapResult> Overlaps;
	if (!World->OverlapMultiByObjectType(
		Overlaps,
		OverlapCenter,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeBox(HalfExtent),
		QueryParams))
	{
		return false;
	}

	for (const FOverlapResult& Result : Overlaps)
	{
		const AActor* HitActor = Result.GetActor();
		if (!IsExcludedActor(HitActor))
		{
			// 只要有一个有效且未被排除的碰撞结果，就将此格标为占用。
			// 这里没有按机器人尺寸膨胀；整车能否通过由 Nav2 footprint 等继续判断。
			return true;
		}
	}

	return false;
}

bool ANavigationMapExporter::ResolveOutputDirectory(FString& OutDirectory) const
{
	if (OutputDirectory.TrimStartAndEnd().IsEmpty())
	{
		return false;
	}

	OutDirectory = MakeAbsoluteOutputDirectory(OutputDirectory);
	return IFileManager::Get().MakeDirectory(*OutDirectory, true);
}

void ANavigationMapExporter::WriteMetadata(
	const FString& Directory,
	const int32 Width,
	const int32 Height,
	const FVector2D& RosOrigin,
	const int32 OccupiedCellCount) const
{
	// metadata 不是 Nav2 必需文件，用于记录导出输入和结果，方便定位地图错位问题。
	const FString Metadata = FString::Printf(
		TEXT("{\n")
		TEXT("  \"format\": \"virtusim_occupancy_map_v1\",\n")
		TEXT("  \"image\": \"%s.pgm\",\n")
		TEXT("  \"width\": %d,\n")
		TEXT("  \"height\": %d,\n")
		TEXT("  \"resolution_m\": %.6f,\n")
		TEXT("  \"origin_ros_m\": [%.6f, %.6f, 0.0],\n")
		TEXT("  \"obstacle_z_cm\": [%.3f, %.3f],\n")
		TEXT("  \"occupied_cells\": %d,\n")
		TEXT("  \"collision_source\": \"ECC_WorldStatic\",\n")
		TEXT("  \"note\": \"This is a 2D projection of collision geometry; Nav2 footprint and inflation remain separate.\"\n")
		TEXT("}\n"),
		*OutputBaseName,
		Width,
		Height,
		ResolutionMeters,
		RosOrigin.X,
		RosOrigin.Y,
		ObstacleMinZ,
		ObstacleMaxZ,
		OccupiedCellCount);

	FFileHelper::SaveStringToFile(Metadata, *(Directory / (OutputBaseName + TEXT("_metadata.json"))));
}

void ANavigationMapExporter::ExportMap()
{
	// 1. 检查运行环境和导出参数。
	if (MapBounds == nullptr || GetWorld() == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：MapBounds 或 World 为空。"));
		return;
	}

	if (ResolutionMeters <= 0.0f || FMath::IsNearlyEqual(ObstacleMinZ, ObstacleMaxZ))
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：分辨率或 Z 检测范围无效。"));
		return;
	}

	// 2. 将 UE 厘米单位的世界范围转换成栅格尺寸。
	// Bounds 是世界轴对齐包围盒，因此旋转 MapBounds 不会生成旋转栅格。
	const FBox Bounds = MapBounds->Bounds.GetBox();
	const float ResolutionCentimeters = ResolutionMeters * CentimetersPerMeter;
	// 向上取整，保证覆盖整个范围；最后一格可能略微超出范围框边缘。
	const int32 Width = FMath::CeilToInt(Bounds.GetSize().X / ResolutionCentimeters);
	const int32 Height = FMath::CeilToInt(Bounds.GetSize().Y / ResolutionCentimeters);

	if (Width <= 0 || Height <= 0 || static_cast<int64>(Width) * Height > MaxCellCount)
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：地图尺寸 %dx%d 超出 MaxCellCount=%d。"), Width, Height, MaxCellCount);
		return;
	}

	// 3. 确保 Saved/NavigationMaps 等输出目录存在。
	FString Directory;
	if (!ResolveOutputDirectory(Directory))
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：无法创建输出目录 %s。"), *OutputDirectory);
		return;
	}

	// 4. 建立 UE 与 ROS 的坐标关系。
	// TempoROS 的约定是 ROS X=UE X/100，ROS Y=-UE Y/100。
	// ROS 原点取范围的左下角；因为 UE Y 被反向，所以它对应 UE 的 Max Y。
	const float MinWorldX = Bounds.Min.X;
	const float MaxWorldY = Bounds.Max.Y;
	const FVector2D RosOrigin(MinWorldX / CentimetersPerMeter, -MaxWorldY / CentimetersPerMeter);

	FCollisionObjectQueryParams ObjectQueryParams;
	// 按碰撞 Object Type 过滤，不按 Actor 名称或 Mobility 自动分类。
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	// false 表示不显式请求复杂查询；实际查询几何仍受资产 Collision Complexity 设置影响。
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VirtuSimNavigationMapExport), false);
	QueryParams.AddIgnoredActor(this);

	// 5. 先把所有栅格初始化为自由空间，再把检测到碰撞的格子改成黑色。
	// 一维数组按行存储：索引 = 行号 * Width + 列号，每格占一个 uint8。
	// 当前不输出未知区域，也不检查地面是否存在，因此框外无支撑区域可能被标为自由。
	TArray<uint8> Pixels;
	Pixels.Init(254, Width * Height);
	int32 OccupiedCellCount = 0;

	// 6. 逐格采样。复杂度约为 Width * Height 次碰撞查询，分辨率越小成本越高。
	for (int32 RosRow = 0; RosRow < Height; ++RosRow)
	{
		// +0.5 取格子中心而非边界；先求 ROS 米坐标，再转回 UE 厘米坐标查询碰撞。
		const float RosY = RosOrigin.Y + (RosRow + 0.5f) * ResolutionMeters;
		const float WorldY = -RosY * CentimetersPerMeter;

		for (int32 Column = 0; Column < Width; ++Column)
		{
			const float WorldX = MinWorldX + (Column + 0.5f) * ResolutionCentimeters;
			// Z 只是占位，IsOccupied 会使用 ObstacleMinZ / ObstacleMaxZ 重建 Z 中心。
			const FVector CellCenter(WorldX, WorldY, 0.0f);
			if (IsOccupied(CellCenter, ObjectQueryParams, QueryParams))
			{
				// PGM 从顶部行开始，而 ROS 栅格行从地图底部开始。
				const int32 PgmRow = Height - 1 - RosRow;
				Pixels[PgmRow * Width + Column] = 0;
				++OccupiedCellCount;
			}
		}
	}

	// 7. 写出 PGM。PGM 使用顶部行优先，而 ROS 行索引从地图底部理解，前面已翻转行号。
	// P5 是二进制灰度格式；头部依次为格式、宽高、最大灰度值，后接原始像素字节。
	TArray<uint8> PgmData;
	const FTCHARToUTF8 Header(*FString::Printf(TEXT("P5\n%d %d\n255\n"), Width, Height));
	PgmData.Append(reinterpret_cast<const uint8*>(Header.Get()), Header.Length());
	PgmData.Append(Pixels);

	const FString PgmPath = Directory / (OutputBaseName + TEXT(".pgm"));
	const FString YamlPath = Directory / (OutputBaseName + TEXT(".yaml"));
	if (!FFileHelper::SaveArrayToFile(PgmData, *PgmPath))
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：无法写入 %s。"), *PgmPath);
		return;
	}

	// 8. YAML 只保存图片路径、分辨率和 ROS 原点，供 map_server 加载 PGM。
	// negate=0 时灰度越低占用概率越高；本类输出的 0/254 会分别落入占用/自由区间。
	// origin 是栅格左下角的 [x米, y米, yaw弧度]，不是机器人起点。
	const FString Yaml = FString::Printf(
		TEXT("image: %s.pgm\n")
		TEXT("mode: trinary\n")
		TEXT("resolution: %.6f\n")
		TEXT("origin: [%.6f, %.6f, 0.0]\n")
		TEXT("negate: 0\n")
		TEXT("occupied_thresh: 0.65\n")
		TEXT("free_thresh: 0.25\n"),
		*OutputBaseName,
		ResolutionMeters,
		RosOrigin.X,
		RosOrigin.Y);

	if (!FFileHelper::SaveStringToFile(Yaml, *YamlPath))
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：无法写入 %s。"), *YamlPath);
		return;
	}

	// 9. 写出调试元数据，并用同一份像素生成 PNG 预览，保证预览与 PGM 一致。
	WriteMetadata(Directory, Width, Height, RosOrigin, OccupiedCellCount);

	// 与 PGM 共用同一份顶部行优先的灰度像素，保证预览方向与障碍分布一致。
	const FString PreviewPath = Directory / (OutputBaseName + TEXT("_preview.png"));
	// FImageView 只引用 Pixels 的内存；同步保存期间 Pixels 仍在作用域内。
	const FImageView PreviewImage(Pixels.GetData(), Width, Height, ERawImageFormat::G8);
	if (!FImageUtils::SaveImageByExtension(*PreviewPath, PreviewImage))
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图已生成，但 PNG 预览写入失败：%s。"), *PreviewPath);
		return;
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("导航地图导出完成（含 PNG 预览）：%dx%d，障碍格=%d，目录=%s，ROS原点=(%.3f, %.3f)"),
		Width,
		Height,
		OccupiedCellCount,
		*Directory,
		RosOrigin.X,
		RosOrigin.Y);
}
