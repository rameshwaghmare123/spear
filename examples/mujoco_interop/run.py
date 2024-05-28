#
# Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
#

import argparse
import mujoco
import mujoco.viewer
import numpy as np
import os
import scipy
import spear
import scipy.spatial


def unreal_rpy_from_mujoco_quaternion(mujoco_quaternion):
    # MuJoCo assumes quaternions are stored in scalar-first (wxyz) order, but scipy.spatial.transform.Rotation assumes scalar-last (xyzw) order
    scipy_quaternion = mujoco_quaternion[[1, 2, 3, 0]]

    # Unreal and scipy.spatial.transform.Rotation have different Euler angle conventions, see python/spear/pipeline.py for details
    scipy_roll, scipy_pitch, scipy_yaw = scipy.spatial.transform.Rotation.from_quat(scipy_quaternion).as_euler("xyz")
    unreal_roll = np.rad2deg(-scipy_roll)
    unreal_pitch = np.rad2deg(-scipy_pitch)
    unreal_yaw = np.rad2deg(scipy_yaw)

    return np.array([unreal_roll, unreal_pitch, unreal_yaw])


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--mjcf_file", default=r"F:\intel\interiorsim\pipeline\apartment_0000\mujoco_scene\main.mjcf")
    args = parser.parse_args()

    # create spear Instance
    config = spear.get_config(user_config_files=[os.path.realpath(os.path.join(os.path.dirname(__file__), "user_config.yaml"))])
    spear.configure_system(config)
    spear_instance = spear.Instance(config)

    # get Unreal actors and functions
    spear_instance.engine_service.begin_tick()

    # get Unreal functions
    unreal_actor_static_class = spear_instance.unreal_service.get_static_class("AActor")
    unreal_set_actor_location_and_rotation_func = spear_instance.unreal_service.find_function_by_name(
        uclass=unreal_actor_static_class, name="K2_SetActorLocationAndRotation")
    unreal_static_mesh_component_static_class = spear_instance.unreal_service.get_static_class("UStaticMeshComponent")
    unreal_set_component_location_and_rotation_func = spear_instance.unreal_service.find_function_by_name(
        uclass=unreal_static_mesh_component_static_class, name="K2_SetRelativeLocationAndRotation")

    # get all actor and componnents
    unreal_actors = spear_instance.unreal_service.find_actors_by_type_as_dict(class_name="AActor")
    unreal_static_mesh_actors = spear_instance.unreal_service.find_actors_by_type_as_dict(class_name="AStaticMeshActor")
    unreal_articulated_actors = {unreal_actor_name: unreal_actor for unreal_actor_name, unreal_actor in unreal_actors.items() if unreal_actor_name not in unreal_static_mesh_actors}

    unreal_articulated_actor_components = {}
    for unreal_actor_name, unreal_actor in unreal_articulated_actors.items():
        unreal_articulated_actor_components[unreal_actor_name] = spear_instance.unreal_service.get_components_by_type_as_dict("UStaticMeshComponent", unreal_actor)

    spear_instance.engine_service.tick()
    spear_instance.engine_service.end_tick()

    # initialize MuJoCo
    mj_model = mujoco.MjModel.from_xml_path(os.path.realpath(args.mjcf_file))
    mj_data = mujoco.MjData(mj_model)
    mujoco.mj_forward(mj_model, mj_data)

    # get MuJoCo bodies
    mj_bodies = {mj_model.body(mj_body).name: mj_body for mj_body in range(mj_model.nbody) if True}

    # launch MuJoCo viewer
    mj_viewer = mujoco.viewer.launch_passive(mj_model, mj_data)

    # initialize MuJoCo camera (not needed when launching the viewer through the command-line, but needed when using launch_passive)
    mj_viewer.cam.distance = 30.0 * 100.0  # 30 meters * 100 Unreal units per meter
    mj_viewer.cam.azimuth = 90.0
    mj_viewer.cam.elevation = -45.0
    mj_viewer.cam.lookat = np.array([0.0, 0.0, 0.0])

    # initialize MuJoCo viewer options
    mj_viewer.opt.label = mujoco.mjtLabel.mjLABEL_SELECTION

    # update MuJoCo viewer state
    mj_viewer.sync()

    while mj_viewer.is_running():

        # perform multiple MuJoCo simulation steps per Unreal frame
        mj_update_steps = 10
        for _ in range(mj_update_steps):
            mujoco.mj_step(mj_model, mj_data)
        mj_viewer.sync()

        # get updated poses from MuJoCo
        mj_bodies_xpos = {mj_body_name: mj_data.body(mj_body).xpos for mj_body_name, mj_body in mj_bodies.items()}
        mj_bodies_xquat = {mj_body_name: mj_data.body(mj_body).xquat for mj_body_name, mj_body in mj_bodies.items()}

        # set updated poses in SPEAR
        spear_instance.engine_service.begin_tick()

        # update static mesh actors
        for unreal_actor_name, unreal_actor in unreal_static_mesh_actors.items():
            # call function for each actor
            args = {
                "NewLocation": dict(zip(["X", "Y", "Z"], mj_bodies_xpos[unreal_actor_name + ":StaticMeshComponent0"])),
                "NewRotation": dict(zip(["Roll", "Pitch", "Yaw"], unreal_rpy_from_mujoco_quaternion(mj_bodies_xquat[unreal_actor_name + ":StaticMeshComponent0"]))),
                "bSweep": False,
                "bTeleport": True}
            spear_instance.unreal_service.call_function(unreal_actor, unreal_set_actor_location_and_rotation_func, args)

        # update articulated actors
        for unreal_actor_name, unreal_actor in unreal_articulated_actors.items():
            # update root component transform
            mujoco_actor_root_component_name = unreal_actor_name + ":DefaultSceneRoot"
            if mujoco_actor_root_component_name in mj_bodies_xpos:
                args = {
                    "NewLocation": dict(zip(["X", "Y", "Z"], mj_bodies_xpos[mujoco_actor_root_component_name])),
                    "NewRotation": dict(zip(["Roll", "Pitch", "Yaw"], unreal_rpy_from_mujoco_quaternion(mj_bodies_xquat[mujoco_actor_root_component_name]))),
                    "bSweep": False,
                    "bTeleport": True}
                spear_instance.unreal_service.call_function(unreal_actor, unreal_set_actor_location_and_rotation_func, args)

            # update component transform
            for unreal_component_name, unreal_component in unreal_articulated_actor_components[unreal_actor_name].items():
                mujoco_component_name = unreal_actor_name + ":DefaultSceneRoot." + unreal_component_name
                if mujoco_component_name in mj_bodies_xpos:
                    location = mj_bodies_xpos[mujoco_component_name]
                    args = {
                        "NewLocation": dict(zip(["X", "Y", "Z"], mj_bodies_xpos[mujoco_component_name])),
                        "NewRotation": dict(zip(["Roll", "Pitch", "Yaw"], unreal_rpy_from_mujoco_quaternion(mj_bodies_xquat[mujoco_component_name]))),
                        "bSweep": False,
                        "bTeleport": True}
                    spear_instance.unreal_service.call_function(unreal_component, unreal_set_component_location_and_rotation_func, args)
                else:
                    # spear.log("invalid_component=",component_full_name)
                    pass
        spear_instance.engine_service.tick()
        spear_instance.engine_service.end_tick()

    mj_viewer.close()
    spear_instance.close()

    spear.log("Done.")
