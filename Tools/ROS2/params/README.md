# Nav2 参数状态

`nav2_params_virtusim_baseline.yaml` 是保留的旧基线，本次整理只移动位置，没有修改参数。

它仍使用 `robot_radius: 0.45` 和 `BaseObstacle` critic，不是当前 SFL_CBD16 非对称底盘完整配置。不要将它直接覆盖到已经调好的 Ubuntu 配置。

当前实际运行文件路径：

```text
/home/ros2/virtusim_nav2/params/nav2_params_full.yaml
```

之前联调记录中的 local / global footprint 为：

```yaml
footprint: "[[0.20, 0.45], [-1.50, 0.45], [-1.50, -0.45], [0.20, -0.45]]"
footprint_padding: 0.05
```

这只能说明底盘几何，不能据此还原全部运行参数。后续从 Ubuntu 复制已验证的完整 `nav2_params_full.yaml` 到本目录，再记录地图、定位模式和验证日期；当前没有创建同名文件冒充已验证版本。

在 Ubuntu Nav2 运行时核对：

```bash
ros2 param get /local_costmap/local_costmap footprint
ros2 param get /global_costmap/global_costmap footprint
ros2 param get /local_costmap/local_costmap footprint_padding
ros2 param get /global_costmap/global_costmap footprint_padding
ros2 param get /controller_server FollowPath.critics
ros2 param get /amcl tf_broadcast
```

当前 UE 已发布固定 `map → odom`，AMCL 不应再广播同名 TF。此约定属于真值定位模式，并非完成了 AMCL 定位验证。

需要取回完整配置时，可在 Ubuntu 中先复制到共享目录：

```bash
cp /home/ros2/virtusim_nav2/params/nav2_params_full.yaml \
  /mnt/hgfs/ShareVmware/nav2_params_full.yaml
```

随后在 Windows 将该文件复制进本目录，核对内容后再决定是否纳入版本管理。
