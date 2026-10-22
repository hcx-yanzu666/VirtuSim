# ROS2 工具与部署目录

这里保存需要部署到 Ubuntu 的脚本、参数和地图。完整启动步骤见 [运行指南](../../Docs/运行指南.md)，导航排错见 [ROS2 与 Nav2 排查](../../Docs/Troubleshooting/ROS2与Nav2排查.md)。

## 目录对应

| 仓库位置 | Ubuntu 位置 / 用途 |
|---|---|
| `scripts/goal_bridge.py` | `/home/ros2/virtusim_nav2/scripts/goal_bridge.py`，目标、取消与 Action 状态桥接 |
| `scripts/virtusim_nav_diagnostics.py` | 同名 `scripts/` 路径，采集运行日志 |
| `params/` | 对应 Ubuntu `params/`；当前只保存旧基线，见 [参数说明](params/README.md) |
| `maps/` | 对应 Ubuntu `maps/`；当前是候选快照，见 [地图说明](maps/README.md) |
| `tests/` | Bridge 离线测试，不需要部署到运行目录 |

Ubuntu 上只运行一个 `goal_bridge.py` 实例。UE 已通过 TempoROS 接入 ROS2；这里的 Python Bridge 专门把目标 / 取消 Topic 转交 Nav2 Action，并将执行状态回传 UE。

## 从共享文件夹更新脚本

以下命令在 Ubuntu 执行。先把仓库中的两个脚本复制到共享目录，再按实际挂载位置调整 `SHARE`。历史挂载点是 `/mnt/hgfs/ShareVmware`。

```bash
SHARE=/mnt/hgfs/ShareVmware
TARGET=/home/ros2/virtusim_nav2
BACKUP="$TARGET/backups/scripts_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$TARGET/scripts" "$TARGET/logs" "$BACKUP"

# 更新前先停止旧 Bridge 和诊断进程。
for name in goal_bridge.py virtusim_nav_diagnostics.py; do
  if [ -f "$TARGET/scripts/$name" ]; then
    cp -p "$TARGET/scripts/$name" "$BACKUP/"
  fi
done

python3 -m py_compile "$SHARE/goal_bridge.py" "$SHARE/virtusim_nav_diagnostics.py" && \
  cp "$SHARE/goal_bridge.py" "$SHARE/virtusim_nav_diagnostics.py" "$TARGET/scripts/"
```

若共享目录里保留了仓库层级，`SHARE` 应改为其 `Tools/ROS2/scripts` 目录。此步骤只更新脚本；地图和 Nav2 参数分别核对后更新。

## 离线测试

在项目根目录运行，不需要安装 ROS2：

```bash
python3 -m unittest discover -s Tools/ROS2/tests -p "test_*.py" -v
```

Windows 未配置 Python PATH 时，可使用本机 UE 附带的解释器：

```powershell
& 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\ThirdParty\Python3\Win64\python.exe' `
  -B -m unittest discover -s Tools/ROS2/tests -p 'test_*.py' -v
```

这些测试使用 ROS 接口替身验证取消、替换目标和异步终态处理，不等同于 Nav2 端到端验证。
