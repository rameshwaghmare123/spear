# Before running this file, rename user_config.yaml.example -> user_config.yaml and modify it with appropriate paths for your system.

import argparse
import cv2
import numpy as np
import os
import spear
import time


NUM_STEPS = 100


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--benchmark", action="store_true")
    args = parser.parse_args()

    # load config
    config = spear.get_config(user_config_files=[ os.path.join(os.path.dirname(os.path.realpath(__file__)), "user_config.yaml") ])

    # create Env object
    env = spear.Env(config)

    # reset the simulation to get the first observation    
    obs = env.reset()

    if args.benchmark:
        start_time = time.time()
    else:
        cv2.imshow("camera_final_color", obs["camera_final_color"][:,:,[2,1,0]]) # OpenCV expects BGR instead of RGB
        cv2.waitKey(0)

    # take a few steps
    for i in range(NUM_STEPS):
        if config.SIMULATION_CONTROLLER.AGENT == "SphereAgent":
            obs, reward, done, info = env.step({"apply_force": np.array([1, 1], dtype=np.float32)})
            if not args.benchmark:
                print("SphereAgent: ", obs["compass"], obs["camera_final_color"].shape, obs["camera_final_color"].dtype, reward, done, info)
        elif config.SIMULATION_CONTROLLER.AGENT == "OpenBotAgent":
            obs, reward, done, info = env.step({"apply_voltage": np.array([1, 1], dtype=np.float32)})
            if not args.benchmark:
                print("OpenBotAgent: ", obs["state_data"], obs["control_data"], obs["camera_final_color"].shape, obs["camera_final_color"].dtype, reward, done, info)
        else:
            assert False

        if not args.benchmark:
            cv2.imshow("camera_final_color", obs["camera_final_color"][:,:,[2,1,0]]) # OpenCV expects BGR instead of RGB
            cv2.waitKey(0)

        if done:
            env.reset()

    if args.benchmark:
        end_time = time.time()
        elapsed_time = end_time - start_time
        print("Average frame time: %0.4f ms (%0.4f fps)" % ((elapsed_time / NUM_STEPS)*1000, NUM_STEPS / elapsed_time))
    else:
        cv2.destroyAllWindows()

    # close the environment
    env.close()

    print("Done.")