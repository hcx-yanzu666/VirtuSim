# 静态 ROS 2 地图生成与使用

本方案不使用 SLAM，也不需要机器人扫描仓库。地图直接根据 Blender 仓库模型的导航占地生成。

## 输出文件

生成结果位于 `Output/ROS2`：

- `warehouse_map.pgm`：Nav2 占据栅格地图。
- `warehouse_map.yaml`：地图分辨率、原点和阈值配置。
- `map_metadata.json`：生成尺寸、障碍物占地和坐标假设，供检查和排错。

当前策略：

- 四面墙和结构柱为障碍物。
- A1、A2、A3 整排货架的完整占地为障碍物，机器人不会从货架底部空隙穿过。
- 三组散放货物为障碍物。
- 货架之间和仓库内部空地保持可通行。
- 地面标线、灯具和屋顶横梁不进入二维地图。
- 地图本身不按机器人半径膨胀，安全距离由 Nav2 Costmap 的 `inflation_layer` 处理。

## 重新生成

仓库模型发生变化后，在项目根目录执行：

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.0\blender.exe' `
  --background `
  'Tools\WarehouseGenerator\Output\Warehouse_Reference.blend' `
  --python 'Tools\WarehouseGenerator\export_static_map.py'
```

执行成功会输出 `STATIC_MAP_OK`。

该脚本假设六个模块在 UE 中的位置和旋转均为 `0`、缩放均为 `1`。如果在 UE 关卡中单独移动或旋转了货架，必须同步修改 Blender 源场景后重新生成地图，否则地图与实际碰撞不会重合。

## 坐标关系

当前仓库以世界原点为中心：

```text
地图范围：36 × 27 米
分辨率：0.05 米/像素
地图尺寸：720 × 540 像素
ROS 地图原点：[-18.0, -13.5, 0.0]
```

TempoROS 使用：

```text
ROS X = UE X / 100
ROS Y = -UE Y / 100
```

当前 Blender FBX 导入方向与该转换配合后，静态地图的 XY 坐标与 ROS 的 `map/odom` 坐标一致。UE 继续发布固定的 `map -> odom` 单位变换。

## 采用地图与部署到 Ubuntu

`Output/ROS2` 保存生成候选结果。确认候选地图与 UE 场景几何一致后，在项目根目录执行，将三份配套文件同步到统一部署目录：

```powershell
Copy-Item Tools/WarehouseGenerator/Output/ROS2/warehouse_map.yaml Tools/ROS2/maps/
Copy-Item Tools/WarehouseGenerator/Output/ROS2/warehouse_map.pgm Tools/ROS2/maps/
Copy-Item Tools/WarehouseGenerator/Output/ROS2/map_metadata.json Tools/ROS2/maps/
```

同时更新 [地图快照说明](../ROS2/maps/README.md) 中的尺寸、来源和验证状态。更新后将配套文件通过共享目录复制到 Ubuntu；备份与部署命令统一见该说明。

Ubuntu 最终加载路径保持为：

```text
/home/ros2/virtusim_nav2/maps/warehouse_map.pgm
/home/ros2/virtusim_nav2/maps/warehouse_map.yaml
```

YAML 中使用相对图片路径，两个文件必须位于同一目录。当前目录整理只保存了候选快照，没有确认 Ubuntu 已切换地图。

## 启动 Nav2

Nav2 启动时显式指定 Ubuntu 端已确认的 YAML；实际运行参数仍以 Ubuntu 文件为准：

```text
/home/ros2/virtusim_nav2/maps/warehouse_map.yaml
```

启动命令：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

ros2 launch nav2_bringup bringup_launch.py \
  map:=/home/ros2/virtusim_nav2/maps/warehouse_map.yaml \
  params_file:=/home/ros2/virtusim_nav2/params/nav2_params_full.yaml \
  use_sim_time:=true
```

RViz 设置：

```text
Fixed Frame: map
Map Topic: /map
LaserScan Topic: /scan
Path Topic: /plan
```

## 验证

```bash
ros2 topic echo /map --once
ros2 run tf2_ros tf2_echo map base_link
```

在 RViz 中确认：

- 四面墙闭合。
- 三排货架与 UE 场景位置一致。
- 货架之间的过道为白色可通行区域。
- 机器人位姿落在地图中的正确位置。

如果地图整体镜像或旋转，先确认 UE 中六个模块的 Actor Transform 是否全部为默认值，以及 FBX 导入时是否额外启用了 `Force Front XAxis` 或导入旋转。
