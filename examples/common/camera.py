import mmap

import numpy as np


class CameraSensor:
    def __init__(self, instance, unreal_actor, render_pass_names=["final_color"], width=512, height=512, fov=90):
        assert unreal_actor
        self._instance = instance
        self._render_pass_names = render_pass_names
        self._width = width
        self._height = height
        self._fov = fov

        self._camera_sensor_component = instance.unreal_service.get_component_by_type_v2("/Script/CoreUObject.Class'/Script/SpComponents.CameraSensorComponent'", unreal_actor)
        assert self._camera_sensor_component
        self._camera_sensor_component_class = instance.unreal_service.get_class(self._camera_sensor_component)

        # setup CameraSensorComponent
        camera_component = instance.unreal_service.get_component_by_type("UCameraComponent", unreal_actor)
        assert camera_component
        setup_args = {
            "camera_component": instance.unreal_service.to_ptr(camera_component),
            "render_pass_names": self._render_pass_names,
            "width": self._width,
            "height": self._height,
            "fov": self._fov,
        }
        unreal_camera_sensor_setup_func = instance.unreal_service.find_function_by_name(uclass=self._camera_sensor_component_class, name="setup")
        instance.unreal_service.call_function(self._camera_sensor_component, unreal_camera_sensor_setup_func, setup_args)

    def get_images(self):
        # do rendering
        self._instance.rpc_client.call("cpp_func_service.call_func", self._camera_sensor_component, "camera_sensor.camera", {})

        # get rendered image from shared memory
        images = {}
        shared_memory_desc = self._instance.rpc_client.call("cpp_func_service.get_shared_memory_views", self._camera_sensor_component)
        for render_pass in self._render_pass_names:
            shared_memory_object = mmap.mmap(-1, shared_memory_desc['final_color']['num_bytes_'], shared_memory_desc['final_color']['id_'])
            shared_memory_array = np.ndarray(shape=(-1,), dtype=np.uint8, buffer=shared_memory_object)

            img = shared_memory_array.reshape([self._width, self._height, 4])
            img[:, [0, 1, 2]] = img[:, [2, 1, 0]]
            images[render_pass] = img[:, :, :3]

        return images
