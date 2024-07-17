#
# Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
#

# Example command that can be called in the Unreal Editor
# py C:\github\spear\pipeline\unreal\import_fbx.py --fbx_file "C:\Users\Rachith\Downloads\Shoved_Reaction_With_Spin.fbx" --destination_dir "/Game/ImportedFBXContent" --destination_name CoolKid --actor_label CoolKid

import argparse
import posixpath
import unreal

parser = argparse.ArgumentParser()
parser.add_argument("--fbx_file", required=True)
parser.add_argument("--destination_dir", required=True)
parser.add_argument("--destination_name", required=True)
parser.add_argument("--actor_label", required=True)
args = parser.parse_args()


def import_assets():

    # create asset tools object
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    ####################################
    #   Automated way of importing data (Not helpful as we cannot name our imported asset)
    ####################################

    # # create asset import data object        
    # assetImportData = unreal.AutomatedAssetImportData()

    # # set assetImportData attributes
    # assetImportData.destination_path = args.destination_dir
    # assetImportData.filenames = [args.fbx_file]
    # assetImportData.replace_existing = True
    # asset_tools.import_assets_automated(assetImportData)

    ######################################
    #   Customizable way of importing data
    ######################################

    # Set up the FBX import options
    fbx_import_options = unreal.FbxImportUI()

    # General settings
    fbx_import_options.set_editor_property("automated_import_should_detect_type", True)
    fbx_import_options.set_editor_property("import_animations", True)

    # Skeletal Mesh settings
    fbx_import_options.skeletal_mesh_import_data = unreal.FbxSkeletalMeshImportData()
    fbx_import_options.skeletal_mesh_import_data.set_editor_property("compute_weighted_normals", False)
    fbx_import_options.skeletal_mesh_import_data.set_editor_property("import_morph_targets", True)
    fbx_import_options.skeletal_mesh_import_data.set_editor_property("normal_import_method", unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)

    # set assetImportTask
    fbx_task = unreal.AssetImportTask()
    fbx_task.set_editor_property("destination_path", args.destination_dir)
    fbx_task.set_editor_property("destination_name", args.destination_name)
    fbx_task.set_editor_property("filename", args.fbx_file)
    fbx_task.set_editor_property("save", True)
    fbx_task.set_editor_property("automated", True)
    fbx_task.set_editor_property("replace_existing", True)
    fbx_task.set_editor_property("options", fbx_import_options)

    # Execute the import task
    asset_tools.import_asset_tasks([fbx_task])


def configure_actor_with_imported_asset():

    # Get the current level
    unreal_editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    actor_editor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    asset_editor_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)

    if not unreal_editor_subsystem.get_editor_world():
        unreal.log_error("Failed to get the current level.")
        return

    actor_class = unreal.Actor
    location = unreal.Vector(-60.0, 230.0, 50.0)
    rotation = unreal.Rotator(0.0, 0.0, 0.0)

    # Spawn the actor
    actor = actor_editor_subsystem.spawn_actor_from_class(actor_class, location, rotation)
    actor.set_actor_label(args.actor_label)

    if actor:
        unreal.log(f"Spawned actor {actor.get_name()} at {location}")
    else:
        unreal.log_error(f"Failed to spawn actor {actor.get_name()}")

    # Load the skeletal mesh asset
    skeletal_mesh_path = posixpath.join(args.destination_dir, args.destination_name)
    skeletal_mesh = asset_editor_subsystem.load_asset(skeletal_mesh_path)

    # create a PoseableMeshComponent and attach it to the spawned actor
    unreal_engine_subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    actor_root_sub_object = unreal_engine_subsystem.k2_gather_subobject_data_for_instance(actor)[0]
    poseablemesh_sub_object = unreal_engine_subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=actor_root_sub_object,
        new_class=unreal.PoseableMeshComponent
    ))
    poseable_mesh_components = actor.get_components_by_class(unreal.PoseableMeshComponent)
    assert len(poseable_mesh_components) == 1
    poseable_mesh_component = poseable_mesh_components[0]
    poseable_mesh_component.set_skinned_asset_and_update(skeletal_mesh)

    # create a StableNameComponent and attach it to the spawned actor
    stablename_sub_object = unreal_engine_subsystem.add_new_subobject(unreal.AddNewSubobjectParams(
        parent_handle=actor_root_sub_object,
        new_class=unreal.StableNameComponent
    ))
    stablename_mesh_components = actor.get_components_by_class(unreal.StableNameComponent)
    assert len(stablename_mesh_components) == 1
    stablename_mesh_component = stablename_mesh_components[0]


if __name__ == "__main__":

    import_assets()

    configure_actor_with_imported_asset()
