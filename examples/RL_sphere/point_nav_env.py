import gym
import numpy as np
import os

import spear
from scipy.spatial.transform import Rotation

common_dir = os.path.realpath(os.path.join(os.path.dirname(__file__), "..", "common"))
import sys

sys.path.append(common_dir)
from agent import OpenBotAgent, SimpleAgent, HabitatNavAgent


def random_position(range=550):
    pos = np.random.rand(3)
    pos *= range
    pos[2] = 50
    return pos.astype(np.float64)


class SpPointNavEnv(gym.Env):
    def __init__(self, env_config):
        spear.log("__init__ Start.")
        self._config = env_config['config']
        self._dummy = env_config["dummy"]
        if env_config['test']:
            self._dummy = True
        if self._dummy:
            pass
        else:

            self._config = spear.get_config(
                user_config_files=[
                    os.path.realpath(os.path.join(os.path.dirname(__file__), "user_config.yaml")),
                    os.path.realpath(os.path.join(common_dir, "default_config.common.yaml"))])

            self._instance = spear.Instance(self._config)

            self._instance.engine_service.begin_tick()
            self._gameplay_statics_class = self._instance.unreal_service.get_static_class(class_name="UGameplayStatics")
            self._gameplay_statics_default_object = self._instance.unreal_service.get_default_object(uclass=self._gameplay_statics_class, create_if_needed=False)
            self._set_game_paused_func = self._instance.unreal_service.find_function_by_name(uclass=self._gameplay_statics_class, name="SetGamePaused")
            self._instance.engine_service.tick()
            self._instance.engine_service.end_tick()

            self._instance.engine_service.begin_tick()

            # spawn agent
            self._agent = SimpleAgent(self._instance)
            # self.action_space = self._agent.get_action_space()
            # self.observation_space = self._agent.get_observation_space()

            self._instance.engine_service.tick()

            # self._goal = np.array([position['x'], position['y'], position['z'] + 50])

            self._instance.engine_service.end_tick()

        self.observation_space = gym.spaces.Dict({
            # "camera.final_color": gym.spaces.Box(0, 255, (480, 640, 3,), np.uint8),
            # "location": gym.spaces.Box(-1000, 1000, (3,), np.float64),
            # "rotation": gym.spaces.Box(-1000, 1000, (3,), np.float64),
            "goal_in_agent_frame": gym.spaces.Box(-2000, 2000, (3,), np.float64),
        })
        self.action_space = gym.spaces.Dict({
            "add_to_location": gym.spaces.Box(-10, 10, (2,), np.float64),
            # "move_left": gym.spaces.Box(-360, 360, (1,), np.float64),
            # "move_right": gym.spaces.Box(-10, 10, (1,), np.float64),
            # "stop": gym.spaces.Box(0, 1, (1,), np.int),
        })
        self._obs = {
            "location": np.array([0, 0, 50]),
            "rotation": np.array([0, 0, 0]),
            "goal_in_agent_frame": np.array([0, 0, 0]),
        }
        self._goal = random_position(range=500)  # + np.array([-200, -200,0])
        self._step = 0

        spear.log("self.action_space", self.action_space)
        spear.log("self.observation_space", self.observation_space)
        spear.log("__init__ Done.")

    def reset(self):
        self._goal = random_position(range=500)  # + np.array([-200, -200,0])
        if self._dummy:
            new_location = random_position()
            new_rotation = np.array([0, 0, 0])

        else:
            self._instance.engine_service.begin_tick()
            self._instance.unreal_service.call_function(uobject=self._gameplay_statics_default_object, ufunction=self._set_game_paused_func, args={"bPaused": False})

            agent_obs = self._agent.reset()

            self._instance.engine_service.tick()

            agent_obs = self._agent.get_observation()
            new_location = agent_obs['location']
            new_rotation = agent_obs['rotation']

            # position = self._agent.get_random_points(1)[0]
            # self._goal = np.array([position['x'], position['y'], position['z'] + 50])

            self._instance.unreal_service.call_function(uobject=self._gameplay_statics_default_object, ufunction=self._set_game_paused_func, args={"bPaused": True})
            self._instance.engine_service.end_tick()

        quat = Rotation.from_euler("xyz", new_rotation, degrees=True)
        new_rotation = np.array(quat.as_euler("xyz", degrees=True))

        self._obs['location'] = new_location
        self._obs['rotation'] = new_rotation
        self._obs["goal_in_agent_frame"] = quat.inv().as_matrix().dot(self._goal - self._obs['location'])

        spear.log("reset Done.", self._step)

        self._step = 0

        return {"goal_in_agent_frame": self._obs["goal_in_agent_frame"]}

    def step(self, action):
        if self._dummy:
            new_rotation = self._obs['rotation']
            quat = Rotation.from_euler("xyz", new_rotation, degrees=True)
            new_rotation = np.array(quat.as_euler("xyz", degrees=True))

            new_location = self._obs['location'] + np.array([action['add_to_location'][0], action['add_to_location'][1], 0])

            collision = abs(new_location[0]) > 550 or abs(new_location[1]) > 550
        else:
            self._instance.engine_service.begin_tick()
            self._instance.unreal_service.call_function(uobject=self._gameplay_statics_default_object, ufunction=self._set_game_paused_func, args={"bPaused": False})

            old_obs = self._agent.get_observation()
            self._agent.apply_action({"add_to_location": np.array([action['add_to_location'][0], action['add_to_location'][1], 0])})

            self._instance.engine_service.tick()

            hit_actors = self._agent.get_hit_actors()
            obs = self._agent.get_observation()
            new_location = obs['location']
            new_rotation = obs['rotation']
            quat = Rotation.from_euler("xyz", new_rotation, degrees=True)
            new_rotation = np.array(quat.as_euler("xyz", degrees=True))

            diff = np.array([action['add_to_location'][0], action['add_to_location'][1], 0]) + old_obs['location'] - new_location
            if np.linalg.norm(diff) > 10:
                print("DIFF?", diff, action, obs['location'], new_location)
            self._instance.unreal_service.call_function(uobject=self._gameplay_statics_default_object, ufunction=self._set_game_paused_func, args={"bPaused": True})
            self._instance.engine_service.end_tick()

            collision = len(hit_actors) > 1

        self._obs['location'] = new_location
        self._obs['rotation'] = new_rotation
        self._obs["goal_in_agent_frame"] = quat.inv().as_matrix().dot(self._goal - self._obs['location'])

        distance = np.linalg.norm(self._goal - self._obs['location'])
        succ = distance < 50

        reward = -distance * 0.001 + (100 if succ else 0) + (-10 if collision else 0)
        done = succ or collision

        info = {}

        if done:
            spear.log("SUCC " if succ else "COLL ", self._step, self._goal, self._obs['location'], self._obs['goal_in_agent_frame'], action, reward, done)
        elif self._step % 100 == 0 or done:
            # spear.log("step ", self._step, self._goal, self._obs['location'], self._obs['goal_in_agent_frame'], action, reward, done)
            pass
        self._step += 1
        return {"goal_in_agent_frame": self._obs["goal_in_agent_frame"]}, reward, done, info

    def close(self):
        if self._dummy:
            pass
        else:
            # close the unreal instance and rpc connection
            self._instance.close()

        spear.log("close Done.")
