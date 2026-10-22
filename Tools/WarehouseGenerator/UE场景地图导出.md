# UE 当前场景地图导出

`ANavigationMapExporter` 是第一版的静态占用地图导出工具。它在编辑器中逐格查询当前关卡的 `WorldStatic` 碰撞体，并输出 Nav2 可读取的 PGM、YAML、元数据 JSON 和 PNG 预览。

## 使用

1. 重新编译工程后，在关卡中放置 `NavigationMapExporter`（Place Actors → All Classes）。
2. 选中它，视口中会显示绿色的三维范围框。使用移动工具拖动 Actor 改变地图中心，使用缩放工具拖动范围框边缘调整 XY 范围；也可以在 Details → `MapBounds` → `Box Extent` 中输入精确尺寸。
3. 在 Details 中设置：
   - `Resolution Meters`：通常为 `0.05`。
   - `Obstacle Min Z / Max Z`：单位厘米，排除地面但覆盖机器人会撞到的障碍。仓库可先使用 `10 / 200`。
   - `Output Directory`：默认 `Saved/NavigationMaps`。
   - `Output Base Name`：默认 `warehouse_map`。
4. 在 Details → Navigation Map 点击 `Export Map`。

输出位置为：

```text
<项目目录>/Saved/NavigationMaps/
  warehouse_map.pgm
  warehouse_map_preview.png
  warehouse_map.yaml
  warehouse_map_metadata.json
```

YAML 的原点与当前工程坐标约定一致：`ROS X = UE X / 100`，`ROS Y = -UE Y / 100`。PGM 黑色为占用，白色为自由区域。

每次点击 `Export Map` 会同时生成或覆盖 PNG 预览。它与 PGM 使用相同的像素、尺寸和方向，可直接用图片查看器打开；部署到 Nav2 仍只需 PGM 和 YAML。

## 重要边界

- 第一版只查询 `ECC_WorldStatic`。动态行人、机器人和其他 `WorldDynamic` 物体不会被写入静态地图；它们继续通过 LiDAR 和 Nav2 obstacle layer 处理。
- 范围框取世界坐标下的轴对齐包围盒；旋转范围框不会生成旋转栅格。Z 检测范围使用世界高度，独立于可视框的 Z 位置和高度。
- 未检测到静态碰撞的格子统一视为自由区域，不判断地面支撑或未知区域。范围框应贴合需要导航的有效地面，避免把仓库外部或悬空区域导出为可通行区域。
- 导出的是三维碰撞几何在 XY 平面的二维投影，不会把质量、摩擦、关节或完整三维碰撞模型写进 YAML。
- `WorldStatic` 的碰撞体质量决定地图质量。没有碰撞体的可见网格不会被导出；错误的凸包可能封住过道。
- 机器人 footprint 和 inflation 不在 PGM 中处理，仍由 Nav2 参数负责。
- 当前导出是编辑器手动操作，不是运行时持续更新；移动货架后需要重新点击导出并重新部署地图。

## 验收顺序

先用一个带碰撞体的箱子验证黑色区域位置，再对当前仓库导出并在 RViz 中确认地图、LiDAR 和障碍物重合。修改一个货架后再次导出，确认地图随场景变化。
