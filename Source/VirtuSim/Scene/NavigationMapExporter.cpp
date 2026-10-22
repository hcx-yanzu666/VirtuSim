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

	FString MakeAbsoluteOutputDirectory(const FString& ConfiguredDirectory)
	{
		return FPaths::IsRelative(ConfiguredDirectory)
			? FPaths::ConvertRelativePathToFull(FPaths::ProjectDir(), ConfiguredDirectory)
			: FPaths::ConvertRelativePathToFull(ConfiguredDirectory);
	}
}

ANavigationMapExporter::ANavigationMapExporter()
{
	PrimaryActorTick.bCanEverTick = false;

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
		MapBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}

bool ANavigationMapExporter::IsExcludedActor(const AActor* Actor) const
{
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

	const float MinZ = FMath::Min(ObstacleMinZ, ObstacleMaxZ);
	const float MaxZ = FMath::Max(ObstacleMinZ, ObstacleMaxZ);
	const FVector OverlapCenter(WorldCellCenter.X, WorldCellCenter.Y, (MinZ + MaxZ) * 0.5f);
	const FVector HalfExtent(
		ResolutionMeters * CentimetersPerMeter * 0.5f,
		ResolutionMeters * CentimetersPerMeter * 0.5f,
		(MaxZ - MinZ) * 0.5f);

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

	const FBox Bounds = MapBounds->Bounds.GetBox();
	const float ResolutionCentimeters = ResolutionMeters * CentimetersPerMeter;
	const int32 Width = FMath::CeilToInt(Bounds.GetSize().X / ResolutionCentimeters);
	const int32 Height = FMath::CeilToInt(Bounds.GetSize().Y / ResolutionCentimeters);

	if (Width <= 0 || Height <= 0 || static_cast<int64>(Width) * Height > MaxCellCount)
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：地图尺寸 %dx%d 超出 MaxCellCount=%d。"), Width, Height, MaxCellCount);
		return;
	}

	FString Directory;
	if (!ResolveOutputDirectory(Directory))
	{
		UE_LOG(LogTemp, Error, TEXT("导航地图导出失败：无法创建输出目录 %s。"), *OutputDirectory);
		return;
	}

	const float MinWorldX = Bounds.Min.X;
	const float MaxWorldY = Bounds.Max.Y;
	// TempoROS 的约定是 ROS X=UE X/100，ROS Y=-UE Y/100。
	const FVector2D RosOrigin(MinWorldX / CentimetersPerMeter, -MaxWorldY / CentimetersPerMeter);

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(VirtuSimNavigationMapExport), false);
	QueryParams.AddIgnoredActor(this);

	TArray<uint8> Pixels;
	Pixels.Init(254, Width * Height);
	int32 OccupiedCellCount = 0;

	for (int32 RosRow = 0; RosRow < Height; ++RosRow)
	{
		const float RosY = RosOrigin.Y + (RosRow + 0.5f) * ResolutionMeters;
		const float WorldY = -RosY * CentimetersPerMeter;

		for (int32 Column = 0; Column < Width; ++Column)
		{
			const float WorldX = MinWorldX + (Column + 0.5f) * ResolutionCentimeters;
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

	WriteMetadata(Directory, Width, Height, RosOrigin, OccupiedCellCount);

	// 与 PGM 共用同一份顶部行优先的灰度像素，保证预览方向与障碍分布一致。
	const FString PreviewPath = Directory / (OutputBaseName + TEXT("_preview.png"));
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
