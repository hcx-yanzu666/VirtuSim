# ROS2 部署

需要替换 Ubuntu 文件时，只使用 [`deploy/`](deploy/) 文件夹。里面是当前可直接复制的运行文件：

```text
deploy/
├── nav2_params_full.yaml
├── warehouse_map.yaml
├── warehouse_map.pgm
├── goal_bridge.py
└── virtusim_nav_diagnostics.py
```

Windows 将 `deploy` 文件夹复制到 VMware 共享目录后，在 Ubuntu 执行：

```bash
source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
unset ROS_LOCALHOST_ONLY

SHARE=/mnt/hgfs/ShareVmware/deploy
TARGET=/home/ros2/virtusim_nav2
BACKUP="$TARGET/backups/deploy_$(date +%Y%m%d_%H%M%S)"

mkdir -p "$BACKUP" "$TARGET/params" "$TARGET/maps" "$TARGET/scripts"
cp -p "$TARGET/params/nav2_params_full.yaml" "$BACKUP/" 2>/dev/null || true
cp -p "$TARGET/maps/warehouse_map.yaml" "$TARGET/maps/warehouse_map.pgm" "$BACKUP/" 2>/dev/null || true
cp -p "$TARGET/scripts/goal_bridge.py" "$TARGET/scripts/virtusim_nav_diagnostics.py" "$BACKUP/" 2>/dev/null || true

python3 -m py_compile \
  "$SHARE/goal_bridge.py" \
  "$SHARE/virtusim_nav_diagnostics.py"

cp "$SHARE/nav2_params_full.yaml" "$TARGET/params/"
cp "$SHARE/warehouse_map.yaml" "$SHARE/warehouse_map.pgm" "$TARGET/maps/"
cp "$SHARE/goal_bridge.py" "$SHARE/virtusim_nav_diagnostics.py" "$TARGET/scripts/"
```

然后按 [运行指南](../../Docs/运行指南.md) 重启 Nav2 和 Bridge。Nav2 使用：

```bash
ros2 launch nav2_bringup bringup_launch.py \
  map:=/home/ros2/virtusim_nav2/maps/warehouse_map.yaml \
  params_file:=/home/ros2/virtusim_nav2/params/nav2_params_full.yaml \
  use_sim_time:=true
```

`deploy/` 是唯一需要复制到 Ubuntu 的目录；`tests/` 只保存不需要 ROS2 的离线测试。

`tests/` 中的 Bridge 测试不需要 ROS2：

```bash
python3 -m unittest discover -s Tools/ROS2/tests -p "test_*.py" -v
```
