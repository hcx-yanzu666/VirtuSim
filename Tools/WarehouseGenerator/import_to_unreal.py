import os
import unreal


SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SOURCE_DIR = os.path.join(SCRIPT_DIR, "Output", "Modules")
DESTINATION = "/Game/Warehouse"
NO_COLLISION_ASSETS = {"SM_Floor_Markings"}


def import_fbx(filename):
    source = os.path.join(SOURCE_DIR, filename)
    task = unreal.AssetImportTask()
    task.filename = source
    task.destination_path = DESTINATION
    task.automated = True
    task.replace_existing = True
    task.save = True

    options = unreal.FbxImportUI()
    options.import_mesh = True
    options.import_as_skeletal = False
    options.import_materials = True
    options.import_textures = False
    options.static_mesh_import_data.combine_meshes = True
    options.static_mesh_import_data.generate_lightmap_u_vs = True
    # A generated convex hull bridges rack aisles and other open spaces.
    # Collision is configured explicitly after import instead.
    options.static_mesh_import_data.auto_generate_collision = False
    task.options = options

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not task.imported_object_paths:
        raise RuntimeError(f"Failed to import {source}")
    for object_path in task.imported_object_paths:
        asset = unreal.EditorAssetLibrary.load_asset(object_path)
        if not isinstance(asset, unreal.StaticMesh):
            continue
        body_setup = asset.get_editor_property("body_setup")
        collision_mode = (
            unreal.CollisionTraceFlag.CTF_USE_DEFAULT
            if asset.get_name() in NO_COLLISION_ASSETS
            else unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        )
        body_setup.set_editor_property("collision_trace_flag", collision_mode)
        body_setup.modify()
        asset.modify()
        asset.post_edit_change()
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
    unreal.log(f"Imported {filename}: {task.imported_object_paths}")


def main():
    filenames = [
        "SM_Warehouse_Shell.fbx",
        "SM_Floor_Markings.fbx",
        "SM_Rack_Row_A1.fbx",
        "SM_Rack_Row_A2.fbx",
        "SM_Rack_Row_A3.fbx",
        "SM_Loose_Props.fbx",
    ]
    for filename in filenames:
        import_fbx(filename)
    unreal.EditorAssetLibrary.save_directory(DESTINATION, only_if_is_dirty=False, recursive=True)
    unreal.log(f"Warehouse assets are ready in {DESTINATION}")


if __name__ == "__main__":
    main()
