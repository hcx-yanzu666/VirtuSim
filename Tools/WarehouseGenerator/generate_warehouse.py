import bpy
import math
import os
import random
from mathutils import Vector
from export_static_map import export_map


ROOT = os.path.dirname(os.path.abspath(__file__))
OUTPUT = os.path.join(ROOT, "Output")
MODULES = os.path.join(OUTPUT, "Modules")
BLEND_PATH = os.path.join(OUTPUT, "Warehouse_Reference.blend")
SCENE_FBX = os.path.join(OUTPUT, "Warehouse_Reference.fbx")
SCENE_SCALE = 1.5
random.seed(20260916)


def ensure_dir(path):
    os.makedirs(path, exist_ok=True)


def material(name, color, metallic=0.0, roughness=0.55):
    mat = bpy.data.materials.new(name)
    mat.diffuse_color = (*color, 1.0)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*color, 1.0)
    bsdf.inputs["Metallic"].default_value = metallic
    bsdf.inputs["Roughness"].default_value = roughness
    return mat


def collection(name):
    col = bpy.data.collections.new(name)
    bpy.context.scene.collection.children.link(col)
    return col


def move_to_collection(obj, col):
    for source in list(obj.users_collection):
        source.objects.unlink(obj)
    col.objects.link(obj)


def box(name, location, dimensions, mat, col, bevel=0.0):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.dimensions = dimensions
    if bevel > 0.0:
        bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
        modifier = obj.modifiers.new("EdgeBevel", "BEVEL")
        modifier.width = bevel
        modifier.segments = 2
    obj.data.materials.append(mat)
    move_to_collection(obj, col)
    return obj


def beam_between(name, start, end, thickness, mat, col):
    start = Vector(start)
    end = Vector(end)
    delta = end - start
    obj = box(name, (start + end) * 0.5, (thickness, thickness, delta.length), mat, col, 0.01)
    obj.rotation_mode = "QUATERNION"
    obj.rotation_quaternion = Vector((0.0, 0.0, 1.0)).rotation_difference(delta.normalized())
    return obj


def add_text(name, body, location, rotation, size, mat, col):
    bpy.ops.object.text_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.data.body = body
    obj.data.align_x = "CENTER"
    obj.data.align_y = "CENTER"
    obj.data.size = size
    obj.data.extrude = 0.012
    obj.data.bevel_depth = 0.004
    obj.data.materials.append(mat)
    move_to_collection(obj, col)
    return obj


def create_pallet(origin, col, mats, prefix):
    ox, oy, oz = origin
    result = []
    result.append(box(f"{prefix}_PalletDeck", (ox, oy, oz + 0.12), (1.18, 0.96, 0.08), mats["wood"], col))
    for index, x in enumerate((-0.48, 0.0, 0.48)):
        result.append(box(f"{prefix}_PalletRunner_{index}", (ox + x, oy, oz + 0.045), (0.12, 0.92, 0.09), mats["wood_dark"], col))
    return result


def create_load(origin, col, mats, prefix, max_height=1.25):
    ox, oy, oz = origin
    create_pallet((ox, oy, oz), col, mats, prefix)
    rows = 2
    cols = 2
    layers = random.choice((1, 2))
    gap = 0.025
    width = (1.1 - gap * (cols - 1)) / cols
    depth = (0.9 - gap * (rows - 1)) / rows
    height = min(max_height / layers, random.uniform(0.28, 0.48))
    palette = (mats["cardboard"], mats["cardboard_light"], mats["crate"])
    for layer in range(layers):
        for row in range(rows):
            for column in range(cols):
                if random.random() < 0.08:
                    continue
                x = ox - 0.55 + width * 0.5 + column * (width + gap)
                y = oy - 0.45 + depth * 0.5 + row * (depth + gap)
                z = oz + 0.16 + height * 0.5 + layer * (height + gap)
                box(f"{prefix}_Box_{layer}_{row}_{column}", (x, y, z), (width, depth, height), random.choice(palette), col)


def create_rack_module(origin, col, mats, prefix, loaded=True):
    ox, oy, oz = origin
    width, depth, height = 2.7, 1.1, 5.0
    post = 0.10
    beam_h, beam_d = 0.14, 0.12
    levels = (0.55, 1.85, 3.15, 4.45)
    for x in (-width * 0.5, width * 0.5):
        for y in (-depth * 0.5, depth * 0.5):
            box(f"{prefix}_Post", (ox + x, oy + y, oz + height * 0.5), (post, post, height), mats["blue"], col, 0.012)
    for level in levels:
        for y in (-depth * 0.5, depth * 0.5):
            box(f"{prefix}_Beam", (ox, oy + y, oz + level), (width + 0.10, beam_d, beam_h), mats["orange"], col, 0.018)
        for x in (-width * 0.5, width * 0.5):
            box(f"{prefix}_Crossbar", (ox + x, oy, oz + level), (post, depth, 0.07), mats["steel"], col, 0.008)
    for x in (-width * 0.5, width * 0.5):
        for side in (-1, 1):
            beam_between(f"{prefix}_Brace", (ox + x, oy - depth * 0.5, oz + 0.25), (ox + x, oy + depth * 0.5, oz + 1.55), 0.055, mats["blue"], col)
            beam_between(f"{prefix}_Brace", (ox + x, oy + depth * 0.5, oz + 1.55), (ox + x, oy - depth * 0.5, oz + 2.85), 0.055, mats["blue"], col)
            if side == 1:
                break
    if loaded:
        for level_index, level in enumerate(levels[:-1]):
            for slot, x_offset in enumerate((-0.66, 0.66)):
                if random.random() < 0.18:
                    continue
                create_load((ox + x_offset, oy, oz + level + 0.08), col, mats, f"{prefix}_L{level_index}_S{slot}", 1.0)


def create_rack_row(origin, modules, row_name, col, mats):
    ox, oy, oz = origin
    for module_index in range(modules):
        create_rack_module((ox + module_index * 2.72, oy, oz), col, mats, f"{row_name}_M{module_index + 1:02d}")
    sign_x = ox - 1.35
    box(f"{row_name}_SignPlate", (sign_x - 0.065, oy - 0.61, oz + 4.25), (0.12, 0.85, 0.70), mats["blue"], col, 0.04)
    add_text(f"{row_name}_SignText", row_name, (sign_x - 0.13, oy - 0.615, oz + 4.25), (math.radians(90), 0, math.radians(-90)), 0.34, mats["white"], col)


def create_shell(col, mats):
    box("SM_Warehouse_Floor", (0, 0, -0.10), (24.0, 18.0, 0.20), mats["floor"], col, 0.02)
    box("SM_Warehouse_FrontWall", (0, -8.90, 3.5), (24.0, 0.20, 7.0), mats["wall"], col, 0.02)
    box("SM_Warehouse_BackWall", (0, 8.90, 3.5), (24.0, 0.20, 7.0), mats["wall"], col, 0.02)
    box("SM_Warehouse_LeftWall", (-11.90, 0, 3.5), (0.20, 18.0, 7.0), mats["wall"], col, 0.02)
    box("SM_Warehouse_RightWall", (11.90, 0, 3.5), (0.20, 18.0, 7.0), mats["wall"], col, 0.02)
    for x in (-9.0, -3.0, 3.0, 9.0):
        for y in (-8.8, 8.8):
            box("SM_Structure_Column", (x, y, 3.5), (0.32, 0.32, 7.0), mats["steel"], col, 0.02)
    for y in (-8.5, -4.25, 0.0, 4.25, 8.5):
        box("SM_Roof_Beam", (0, y, 6.65), (24.0, 0.24, 0.32), mats["steel_dark"], col, 0.02)
    for x in range(-10, 11, 4):
        box("SM_Roof_Rail", (x, 0, 6.78), (0.16, 18.0, 0.18), mats["steel_dark"], col, 0.015)
    for x in (-7.0, 0.0, 7.0):
        for y in (-5.0, 0.0, 5.0):
            box("SM_Ceiling_Light", (x, y, 6.48), (1.1, 0.35, 0.08), mats["emissive"], col, 0.01)


def create_floor_markings(col, mats):
    z = 0.015
    line = 0.10
    for x in (-9.5, 9.5):
        box("SM_SafetyLine", (x, 0, z), (line, 14.0, 0.012), mats["yellow"], col, 0.0)
    for y in (-7.0, 7.0):
        box("SM_SafetyLine", (0, y, z), (19.0, line, 0.012), mats["yellow"], col, 0.0)
    for y in (-3.0, 3.0):
        box("SM_AisleLine", (0, y, z + 0.002), (18.0, 0.06, 0.012), mats["yellow"], col, 0.0)


def scale_collections(collections, factor):
    for col in collections:
        for obj in col.objects:
            obj.location *= factor
            obj.scale *= factor


def create_camera_and_lighting():
    world = bpy.context.scene.world
    world.color = (0.035, 0.035, 0.035)
    world.use_nodes = True
    world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.05, 0.06, 0.08, 1.0)
    world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.35
    bpy.ops.object.light_add(type="AREA", location=(0, 0, 9.3))
    light = bpy.context.object
    light.name = "Key_Area"
    light.data.energy = 8000
    light.data.shape = "RECTANGLE"
    light.data.size = 24.0
    light.data.size_y = 15.0
    bpy.ops.object.camera_add(location=(15.8, -11.8, 9.4))
    camera = bpy.context.object
    camera.name = "Camera_Overview"
    direction = Vector((0, 1.2, 4.0)) - camera.location
    camera.rotation_euler = direction.to_track_quat("-Z", "Y").to_euler()
    camera.data.lens = 25
    bpy.context.scene.camera = camera


def select_collection(col):
    bpy.ops.object.select_all(action="DESELECT")
    for obj in col.all_objects:
        if obj.type in {"MESH", "FONT"}:
            obj.select_set(True)


def export_collection_as_single_mesh(col, object_name, path):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    temp_collection = bpy.data.collections.new(f"TEMP_{object_name}")
    bpy.context.scene.collection.children.link(temp_collection)
    temporary_objects = []

    for source in col.all_objects:
        if source.type not in {"MESH", "FONT"}:
            continue
        evaluated = source.evaluated_get(depsgraph)
        mesh = bpy.data.meshes.new_from_object(evaluated, depsgraph=depsgraph)
        duplicate = bpy.data.objects.new(source.name, mesh)
        duplicate.matrix_world = source.matrix_world.copy()
        temp_collection.objects.link(duplicate)
        temporary_objects.append(duplicate)

    if not temporary_objects:
        raise RuntimeError(f"Collection {col.name} contains no exportable objects")

    bpy.ops.object.select_all(action="DESELECT")
    for obj in temporary_objects:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = temporary_objects[0]
    bpy.ops.object.join()
    joined = bpy.context.object
    joined.name = object_name
    joined.data.name = object_name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    export_fbx(path, True)

    bpy.data.objects.remove(joined, do_unlink=True)
    bpy.data.collections.remove(temp_collection)


def export_fbx(path, selected_only):
    bpy.ops.export_scene.fbx(
        filepath=path,
        use_selection=selected_only,
        apply_unit_scale=True,
        apply_scale_options="FBX_SCALE_UNITS",
        axis_forward="-Y",
        axis_up="Z",
        object_types={"MESH", "EMPTY", "OTHER"},
        use_mesh_modifiers=True,
        mesh_smooth_type="FACE",
        add_leaf_bones=False,
        bake_anim=False,
        path_mode="AUTO",
    )


def main():
    ensure_dir(OUTPUT)
    ensure_dir(MODULES)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.meshes, bpy.data.curves, bpy.data.materials, bpy.data.cameras, bpy.data.lights):
        pass
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.display.shading.light = "STUDIO"
    scene.display.shading.color_type = "MATERIAL"
    scene.display.shading.show_shadows = True
    scene.display.shading.show_cavity = True
    scene.display.shading.cavity_type = "WORLD"
    scene.render.resolution_x = 1280
    scene.render.resolution_y = 720
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.filepath = os.path.join(OUTPUT, "Warehouse_Preview.png")

    mats = {
        "floor": material("M_Concrete", (0.34, 0.37, 0.40), 0.0, 0.34),
        "wall": material("M_Wall", (0.72, 0.74, 0.76), 0.0, 0.62),
        "steel": material("M_Steel", (0.42, 0.46, 0.50), 0.65, 0.34),
        "steel_dark": material("M_SteelDark", (0.16, 0.19, 0.23), 0.75, 0.28),
        "blue": material("M_RackBlue", (0.025, 0.16, 0.39), 0.55, 0.30),
        "orange": material("M_RackOrange", (0.92, 0.22, 0.035), 0.45, 0.31),
        "yellow": material("M_SafetyYellow", (0.95, 0.59, 0.025), 0.0, 0.38),
        "cardboard": material("M_Cardboard", (0.52, 0.31, 0.14), 0.0, 0.74),
        "cardboard_light": material("M_CardboardLight", (0.72, 0.51, 0.27), 0.0, 0.76),
        "crate": material("M_PlasticCrate", (0.31, 0.34, 0.34), 0.05, 0.48),
        "wood": material("M_PalletWood", (0.48, 0.30, 0.14), 0.0, 0.71),
        "wood_dark": material("M_PalletWoodDark", (0.29, 0.17, 0.075), 0.0, 0.76),
        "white": material("M_SignWhite", (0.94, 0.95, 0.97), 0.0, 0.4),
        "emissive": material("M_LightPanel", (0.93, 0.95, 1.0), 0.0, 0.2),
    }
    bsdf = mats["emissive"].node_tree.nodes.get("Principled BSDF")
    bsdf.inputs["Emission Color"].default_value = (0.93, 0.96, 1.0, 1.0)
    bsdf.inputs["Emission Strength"].default_value = 5.0

    shell = collection("Warehouse_Shell")
    markings = collection("Floor_Markings")
    racks = [collection(f"Rack_Row_{name}") for name in ("A1", "A2", "A3")]
    props = collection("Loose_Props")
    create_shell(shell, mats)
    create_floor_markings(markings, mats)
    for row_index, (name, y) in enumerate(zip(("A1", "A2", "A3"), (-4.4, 0.0, 4.4))):
        create_rack_row((-6.8, y, 0), 6, name, racks[row_index], mats)
    for index, position in enumerate(((-9.7, -7.2, 0.0), (8.8, 7.2, 0.0), (9.5, -6.9, 0.0))):
        create_load(position, props, mats, f"LooseLoad_{index + 1}", 1.15)
    scale_collections([shell, markings, *racks, props], SCENE_SCALE)
    create_camera_and_lighting()

    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
    bpy.ops.render.render(write_still=True)
    bpy.ops.object.select_all(action="SELECT")
    export_fbx(SCENE_FBX, False)
    exports = [
        (shell, "SM_Warehouse_Shell"),
        (markings, "SM_Floor_Markings"),
        (props, "SM_Loose_Props"),
    ]
    exports.extend((rack, f"SM_Rack_Row_{name}") for rack, name in zip(racks, ("A1", "A2", "A3")))
    for col, object_name in exports:
        export_collection_as_single_mesh(col, object_name, os.path.join(MODULES, f"{object_name}.fbx"))
    bpy.ops.wm.save_as_mainfile(filepath=BLEND_PATH)
    export_map()
    print(f"Generated Blender scene: {BLEND_PATH}")
    print(f"Generated FBX scene: {SCENE_FBX}")


if __name__ == "__main__":
    main()
