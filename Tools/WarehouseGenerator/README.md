# 仓库场景生成与 UE 5.6 使用说明

本目录包含根据参考图生成的模块化仓库场景，可用于 Unreal Engine 5.6 中的场景搭建、机器人仿真、激光雷达测试和仓储布局验证。

参考图不是带有标定信息的测量照片，因此当前尺寸按照常见工业仓库规格推定。生成场景已按原布局统一放大 `1.5 倍`，约为 `36 × 27 × 10.8 米`。货架、货物、通道、墙体和钢结构同步放大，相对布局保持不变。厂房四面均有墙体封闭。如果需要与真实现场严格一致，应补充至少一个已知尺寸和更多无裁剪照片。

## 文件说明

### 可直接使用的文件

- `Output/Warehouse_Reference.blend`：可继续编辑的 Blender 5.0 源文件。
- `Output/Warehouse_Reference.fbx`：包含全部对象的完整场景 FBX。
- `Output/Warehouse_Preview.png`：当前场景预览图。
- `Output/ROS2/warehouse_map.pgm`：直接从仓库几何生成的 Nav2 静态占据栅格地图，不需要 SLAM 扫图。
- `Output/ROS2/warehouse_map.yaml`：静态地图配置。
- `Output/ROS2/warehouse_map_preview.png`：静态地图预览。
- `Output/Modules/SM_Warehouse_Shell.fbx`：地面、墙体、立柱和顶部钢梁，FBX 内为一个合并网格。
- `Output/Modules/SM_Floor_Markings.fbx`：黄色地面安全线。
- `Output/Modules/SM_Rack_Row_A1.fbx`：A1 货架及货物，FBX 内为一个合并网格。
- `Output/Modules/SM_Rack_Row_A2.fbx`：A2 货架及货物，FBX 内为一个合并网格。
- `Output/Modules/SM_Rack_Row_A3.fbx`：A3 货架及货物，FBX 内为一个合并网格。
- `Output/Modules/SM_Loose_Props.fbx`：仓库边缘散放的托盘和货物。

### 工具脚本

- `generate_warehouse.py`：重新生成 Blender、FBX 和预览图。
- `export_static_map.py`：从当前 Blender 仓库直接生成 ROS 2 静态地图。
- `import_to_unreal.py`：自动把六个模块导入 UE 项目的 `/Game/Warehouse`。

## 坐标与单位

- Blender 使用米制单位。
- FBX 已写入单位换算信息，导入 UE 后使用厘米单位。
- 坐标方向为 `Z` 轴向上、`-Y` 轴向前。
- 六个模块共用同一个世界原点。全部放置在 UE 的 `(0, 0, 0)` 后即可恢复完整仓库布局。
- 导入时不要额外修改缩放，Actor Scale 应保持 `(1, 1, 1)`。

## 推荐方式：在 UE 编辑器中手动导入

1. 打开 `VirtuSim.uproject`。
2. 在内容浏览器中创建目录 `/Game/Warehouse`。
3. 将 `Output/Modules` 中的六个 FBX 文件一起拖入该目录。
4. 在 FBX 导入窗口中使用以下设置：
   - **Skeletal Mesh（骨骼网格体）**：关闭。
   - **Import Mesh（导入网格体）**：开启。
   - **Combine Meshes（合并网格体）**：可以开启。模块 FBX 在 Blender 导出前已经真正合并，每个文件只包含一个网格节点。
   - **Import Materials（导入材质）**：开启。
   - **Import Textures（导入纹理）**：关闭，当前资产使用基础颜色材质。
   - **Normal Import Method（法线导入方式）**：选择导入法线和切线。
   - **Generate Lightmap UVs（生成光照贴图 UV）**：开启。
   - **Auto Generate Collision（自动生成碰撞）**：关闭。大型合并网格的自动凸包会跨过货架之间的空隙，把过道错误地变成碰撞区域。
   - **Import Uniform Scale（统一导入缩放）**：保持 `1.0`。
5. 点击 **Import All（全部导入）**。
6. 将六个生成的 Static Mesh 从内容浏览器拖入关卡。
7. 把六个 Actor 的 Location 全部设置为 `(0, 0, 0)`，Rotation 设置为 `(0, 0, 0)`，Scale 设置为 `(1, 1, 1)`。

如果整体朝向与当前关卡坐标约定不同，选中六个 Actor 后统一旋转，不要单独旋转其中一个模块。

## 制作可重复放置的仓库蓝图

模块摆放正确后，可以把它们组合成一个可反复使用的 Actor：

1. 在关卡中同时选中六个仓库模块 Actor。
2. 点击编辑器顶部的 **Blueprint/Add Script（蓝图/添加脚本）**。
3. 选择 **Convert Selection to Blueprint Class（将所选内容转换为蓝图类）**。
4. 保存为 `/Game/Warehouse/BP_Warehouse_Reference`。
5. 删除关卡中的临时模块，然后从内容浏览器拖入 `BP_Warehouse_Reference`。

以后可以整体移动或复制仓库，同时仍能在蓝图内部替换某一排货架或关闭某个模块。

## 自动导入方式

自动导入需要先在 UE 编辑器中启用插件：

1. 打开 **Edit > Plugins（编辑 > 插件）**。
2. 搜索并启用 **Python Editor Script Plugin**。
3. 按编辑器提示重启项目。

然后在项目根目录执行：

```powershell
UnrealEditor-Cmd.exe VirtuSim.uproject -ExecutePythonScript=Tools/WarehouseGenerator/import_to_unreal.py
```

如果系统找不到 `UnrealEditor-Cmd.exe`，使用本机 UE 5.6 的完整路径，例如：

```powershell
& 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'D:\Epic\UEPJ\VirtuSim\VirtuSim.uproject' `
  -ExecutePythonScript='D:\Epic\UEPJ\VirtuSim\Tools\WarehouseGenerator\import_to_unreal.py'
```

脚本会完成以下操作：

- 将六个模块导入 `/Game/Warehouse`。
- 每个模块合并为一个 Static Mesh。
- 生成光照贴图 UV，并关闭会覆盖过道的自动凸包碰撞。
- 仓库外壳、货架和散放货物设置为 **Use Complex Collision As Simple**。
- 地面黄色标线不生成碰撞。
- 保存导入的资产。
- 不会修改或保存任何关卡。

自动导入完成后，仍需在关卡中把六个 Static Mesh 放到 `(0, 0, 0)`，或者按照上一节制作仓库蓝图。

## UE 中的性能与碰撞建议

- 仓库外壳和三排货架可以启用 Nanite。
- 不要对整排货架使用自动生成的凸包碰撞。凸包会跨过货架空隙和过道。
- 当前合并模块推荐使用 **Collision Complexity > Use Complex Collision As Simple**，这样碰撞会贴合墙体、梁柱、货架和箱子的实际表面，过道仍然可通行。
- `SM_Floor_Markings` 应设为 **No Collision**，黄色地面线不应阻挡机器人或角色。
- 如果货架只用于视觉展示，可以把对应 Actor 的 Collision Presets 设置为 **NoCollision**。
- 对需要物理模拟、动态移动或大量复制的货架，建议后续拆成单节模块并制作 UCX 简单碰撞；复杂碰撞适合当前静态场景，不适合作为可模拟刚体。
- 用于激光雷达扫描时，应保留墙体、立柱、货架和货物的碰撞或可追踪几何体。
- 大规模复制货架时，建议进一步拆成单节货架，并通过 ISM/HISM 或 PCG 实例化；当前 A1、A2、A3 是已经装载货物的整排模块。
- 项目当前关闭静态光照并使用 Lumen，导入生成的 Lightmap UV 不会影响动态光照使用。

### 修复已经错误生成的过道碰撞

如果导入时曾开启 **Auto Generate Collision**，UE 可能已经生成跨越过道的凸包。仅修改 Collision Complexity 不一定会删除这些旧凸包，可以运行：

```powershell
& 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' `
  'D:\Epic\UEPJ\VirtuSim\VirtuSim.uproject' `
  -ExecutePythonScript='D:\Epic\UEPJ\VirtuSim\Tools\WarehouseGenerator\fix_collision_in_unreal.py'
```

`fix_collision_in_unreal.py` 会删除六个资产上已有的简单凸包，将仓库外壳、货架和散放货物设置为 **Use Complex Collision As Simple**，并移除地面标线的碰撞。

也可以在 Static Mesh Editor 中手动处理：

1. 打开一个货架 Static Mesh。
2. 点击 **Collision > Remove Collision** 删除已有的凸包。
3. 在 Details 面板把 **Collision Complexity** 设置为 **Use Complex Collision As Simple**。
4. 保存资产，并对仓库外壳、三排货架和散放货物重复操作。
5. `SM_Floor_Markings` 删除碰撞后保持无碰撞。

## 在 Blender 中修改后重新生成

可以直接打开 `Output/Warehouse_Reference.blend` 手动修改并重新导出。也可以在项目根目录运行生成脚本：

```powershell
& 'C:\Program Files\Blender Foundation\Blender 5.0\blender.exe' `
  --background `
  --python Tools\WarehouseGenerator\generate_warehouse.py
```

重新运行会覆盖 `Output` 中同名的 Blender、FBX 和预览图文件。UE 中可以对已导入资产执行 **Reimport（重新导入）** 来更新模型。

重新运行 `generate_warehouse.py` 时也会同步更新 `Output/ROS2` 中的 `.pgm` 和 `.yaml`。静态地图的复制与 Nav2 使用方式见 `静态ROS2地图使用说明.md`。

## 推荐的项目使用顺序

1. 先导入六个模块并在测试关卡中检查尺寸和朝向。
2. 检查机器人、门洞、通道宽度和货架间距是否符合仿真需求。
3. 调整地面和主要结构的碰撞。
4. 制作 `BP_Warehouse_Reference`，作为完整仓库预制体使用。
5. 需要更自由的地编时，再把货架拆分成单节模块并进行实例化摆放。

## 与项目其他目录的关系

- 当前功能与待办：[项目现状](../../Docs/项目现状与下一步.md)。
- Nav2 启动与导航验证：[运行指南](../../Docs/运行指南.md)。
- 本工具 `Output/ROS2/` 保留 Blender 原始地图。UE 中调整过场景后，优先使用 [当前关卡地图导出](UE场景地图导出.md)，核对后同步到 [ROS2 地图部署目录](../ROS2/maps/README.md)，再部署到 Ubuntu。
- 保留 `.blend` 源文件；`__pycache__`、`.blend1` 和运行日志不纳入版本管理。生成器输出没有被批量删除或自动忽略，避免遗漏需要保留的模型源文件。
