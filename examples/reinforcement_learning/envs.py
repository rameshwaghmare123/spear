import gym
import numpy as np
import os
import psutil
from yacs.config import CfgNode

import spear


class RLlibEnv(gym.Env):

    def __init__(self, env_config):

        # We need to perform some modifications to config from within this constructor instead of
        # earlier in our code, because we need access to env_config.worker_index, and this property
        # is automatically added to env_config by RLLib just before invoking this constructor.
        self._worker_index = env_config.worker_index

        # Get the top-level YACS CfgNode from env_config
        config = env_config["config"]

        # If we're restoring from a previous checkpoint, then config will be a dict instead of a CfgNode
        if isinstance(config, dict):
            config = CfgNode(init_dict=config)

        config.defrost()

        config.SPEAR.UNREAL_INTERNAL_LOG_FILE = os.path.join("worker_index_%02d" % self._worker_index, "log.txt")
        config.SPEAR.TEMP_DIR = os.path.join(config.SPEAR.TEMP_DIR, "worker_index_%02d" % self._worker_index)
        config.SPEAR.RANDOM_SEED = self._worker_index
        config.SIMULATION_CONTROLLER.PORT = config.SIMULATION_CONTROLLER.PORT + self._worker_index

        # If there are CUDA devices, then assign gpu_id based on worker_index and available CUDA devices.
        # We don't use the CUDA_VISIBLE_DEVICES environment variable directly because RLlib overwrites
        # it. See https://docs.ray.io/en/master/ray-core/using-ray-with-gpus.html
        if len(config.TRAIN.CUDA_VISIBLE_DEVICES) > 0:
            device_ids = [int(i) for i in config.TRAIN.CUDA_VISIBLE_DEVICES.split(",")]
            config.SPEAR.GPU_ID = device_ids[self._worker_index % len(device_ids)]

        config.freeze()

        # Verify that the desired port is not in use. Note that this test is overly conservative, because
        # it isn't a problem if a truly remote address is using our desired port. But it's hard to check
        # if an apparently remote address is truly a remote address, so we implement our overly conservative
        # test.
        for pid in psutil.pids():
            try:
                process = psutil.Process(pid)
                for connection in process.connections(kind="tcp"):
                    assert connection.laddr.port != config.SIMULATION_CONTROLLER.PORT
                    if connection.raddr != ():
                        assert connection.raddr.port != config.SIMULATION_CONTROLLER.PORT
            except psutil.AccessDenied:
                pass
            except psutil.NoSuchProcess:
                pass

        # Environment creation
        self._env = spear.Env(config=config)
        self._set_spaces()

    def reset(self):
        obs = self._env.reset()
        return self._transform_observation(obs)

    def step(self, action):
        obs, reward, done, info = self._env.step(action)
        return self._transform_observation(obs), reward, done, info

    def close(self):
        print("self._worker_index = " + str(self._worker_index) + ", Closing environment...")
        self._env.close()

    def _set_spaces(self):
        assert False

    def _transform_observation(self, obs):
        assert False


class SphereAgentEnv(RLlibEnv):
    def _set_spaces(self):
        self.action_space = self._env.action_space
        self.observation_space = gym.spaces.Dict({
            "compass": self._env.observation_space["compass"],
            "camera": gym.spaces.Box(low=0.0, high=1.0, shape=self._env.observation_space["camera_final_color"].shape, dtype=np.float32)})

    def _transform_observation(self, obs):
        return {
            "compass": obs["compass"],
            "camera": (obs["camera_final_color"] / 255.0).astype(np.float32)}