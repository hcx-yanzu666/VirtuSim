# 仓库地图部署快照

本目录保留当前 UE 关卡通过 `NavigationMapExporter` 导出的候选部署快照，包含 PGM、YAML、元数据和 PNG 预览。原始输出位于 `Saved/NavigationMaps/`；元数据文件在此统一命名为 `map_metadata.json`。用户已核对 Ubuntu 两个地图文件的复制哈希，完整导航验收仍待完成。

| 项目 | 当前文件记录 |
|---|---|
| 图像 | `warehouse_map.pgm`，720 × 560 像素 |
| 分辨率 | 0.05 米 / 像素 |
| 原点 | `[-18.006060, -14.000458, 0.0]` |
| 覆盖尺寸 | 36 × 28 米 |
| 几何与坐标假设 | `map_metadata.json` |
| 状态 | UE 静态碰撞导出快照，待完成导航验证 |

`warehouse_map.yaml` 中的 `image` 使用相对路径，必须与 PGM 放在一起。PNG 仅供预览。`Tools/WarehouseGenerator/Output/ROS2/` 保留 Blender 原始生成结果，与本目录不是同一份地图；修改 UE 关卡后应重新导出，而不是覆盖回旧版。

## 确认后部署

将本目录的 YAML、PGM 和 metadata 一起放入共享目录。在 Ubuntu 中执行，先备份现有地图：

```bash
SHARE=/mnt/hgfs/ShareVmware
TARGET=/home/ros2/virtusim_nav2
BACKUP="$TARGET/backups/maps_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TARGET/maps" "$BACKUP"

for name in warehouse_map.yaml warehouse_map.pgm map_metadata.json; do
  if [ -f "$TARGET/maps/$name" ]; then
    cp -p "$TARGET/maps/$name" "$BACKUP/"
  fi
done

cp "$SHARE/warehouse_map.yaml" "$SHARE/warehouse_map.pgm" \
  "$SHARE/map_metadata.json" "$TARGET/maps/"
```

停止并重新启动 Nav2 的 map_server 后检查：

```bash
ros2 topic info /map -v
ros2 topic echo /map --once --field info
```

检查发布者唯一、分辨率 / 宽高 / 原点正确，并在 RViz 中确认扫描与障碍物重合。尺寸相同不代表地图内容相同，还需要检查几何，必要时对比两端文件的 SHA256。

当前关卡的导出方式见 [UE 场景地图导出](../../WarehouseGenerator/UE场景地图导出.md)。Blender 原始地图的生成方式与坐标约定见 [静态地图说明](../../WarehouseGenerator/静态ROS2地图使用说明.md)。
