import unreal


ASSET_FOLDER = "/Game/Warehouse"
COLLIDING_ASSETS = (
    "SM_Warehouse_Shell",
    "SM_Rack_Row_A1",
    "SM_Rack_Row_A2",
    "SM_Rack_Row_A3",
    "SM_Loose_Props",
)
NO_COLLISION_ASSETS = ("SM_Floor_Markings",)


def load_static_mesh(asset_name):
    asset_path = f"{ASSET_FOLDER}/{asset_name}.{asset_name}"
    asset = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(asset, unreal.StaticMesh):
        raise RuntimeError(f"Static Mesh not found: {asset_path}")
    return asset


def set_collision_trace(asset, trace_flag):
    body_setup = asset.get_editor_property("body_setup")
    body_setup.set_editor_property("collision_trace_flag", trace_flag)
    body_setup.modify()
    asset.modify()
    asset.post_edit_change()
    unreal.EditorAssetLibrary.save_loaded_asset(asset)


def main():
    subsystem = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

    for asset_name in COLLIDING_ASSETS:
        asset = load_static_mesh(asset_name)
        subsystem.remove_collisions(asset)
        set_collision_trace(asset, unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE)
        unreal.log(f"Complex-as-simple collision enabled: {asset_name}")

    for asset_name in NO_COLLISION_ASSETS:
        asset = load_static_mesh(asset_name)
        subsystem.remove_collisions(asset)
        set_collision_trace(asset, unreal.CollisionTraceFlag.CTF_USE_DEFAULT)
        unreal.log(f"Collision removed: {asset_name}")

    unreal.EditorAssetLibrary.save_directory(ASSET_FOLDER, only_if_is_dirty=False, recursive=True)
    unreal.log("Warehouse collision repair completed")


if __name__ == "__main__":
    main()
