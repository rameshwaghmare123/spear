import argparse
import numpy as np
import os
import pandas as pd
import ray
from ray.tune.registry import register_env
from ray.tune import run, sample_from
from ray.tune.schedulers import PopulationBasedTraining
from ray.tune.schedulers.pb2 import PB2
from ray.rllib.agents.ppo import PPOTrainer
import time

from envs import SphereAgentEnv
from models import get_model_config_conv, get_model_config_fc
import spear


# PPO specific settings:
# lr_schedule = None
# use_critic = True
# use_gae = True
# lambda_ = 1.0
# kl_coeff = 0.2
# sgd_minibatch_size = 128
# num_sgd_iter = 30
# shuffle_sequences = True
# vf_loss_coeff = 1.0
# entropy_coeff = 0.0
# entropy_coeff_schedule = None
# clip_param = 0.3
# vf_clip_param = 10.0
# grad_clip = None
# kl_target = 0.01

# Override some of AlgorithmConfig's default values with PPO-specific values.
# rollout_fragment_length = 200
# train_batch_size = 4000
# lr = 5e-5
# model["vf_share_layers"] = False

if __name__ == "__main__":

    # Parse input script arguments
    parser = argparse.ArgumentParser()
    parser.add_argument("--num_epochs", type=int, help="number of iterations through the environment", required=True)
    parser.add_argument("--num_workers", required=True, type=int)
    parser.add_argument("--run_name")
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--seed", type=int, default=0)
    parser.add_argument("--filename", type=str, default="")
    parser.add_argument("--method", type=str, default="pb2")  # ['pbt', 'pb2']
    parser.add_argument("--save_csv", action="store_true") # "training_iteration", "time_total_s"
    args = parser.parse_args()

    # load config
    config = spear.get_config(user_config_files=[ os.path.join(os.path.dirname(os.path.realpath(__file__)), "user_config.yaml") ])

    if args.resume:
        assert args.run_name is not None
        experiment_analysis = ray.tune.ExperimentAnalysis(os.path.join(config.TRAIN.RAY_RESULTS_DIR, args.name))
        assert experiment_analysis.get_last_checkpoint() is not None
        print("\n\n\nResuming " + args.name + " at checkpoint: " + experiment_analysis.get_last_checkpoint() + "\n\n\n")

    # RLlib overwrites this environment variable, so we copy it into env_config before invoking RLlib.
    # See https://docs.ray.io/en/master/ray-core/using-ray-with-gpus.html
    config.defrost()
    if "CUDA_VISIBLE_DEVICES" in os.environ:
        config.TRAIN.CUDA_VISIBLE_DEVICES = os.environ["CUDA_VISIBLE_DEVICES"]
    else:
        config.TRAIN.CUDA_VISIBLE_DEVICES = ""
    config.freeze()

    model_config = get_model_config_conv(config.SIMULATION_CONTROLLER.SPHERE_AGENT.CAMERA.IMAGE_HEIGHT, config.SIMULATION_CONTROLLER.SPHERE_AGENT.CAMERA.IMAGE_WIDTH)
    # model_config = get_model_config_fc(512)

    env_config = {"config": config}

    ray_config = {
        "env": SphereAgentEnv,
        "env_config": env_config,
        "num_workers": args.num_workers,
        # "num_gpus": 1,
        "framework": "torch",
        "model": model_config,
        "log_level": "DEBUG",
        "seed": args.seed,
        # "num_sgd_iter": 10,
        # "sgd_minibatch_size": 128,
        # "lambda": sample_from(lambda spec: np.random.uniform(0.9, 1.0)),
        # "clip_param": sample_from(lambda spec: np.random.uniform(0.1, 0.5)),
        "lr": sample_from(lambda spec: np.random.uniform(1e-3, 1e-5)),
        # "train_batch_size": sample_from(lambda spec: np.random.randint(1000, 4000)),
    }

    experiment_analysis = run(
        PPOTrainer,
        stop={"training_iteration": args.num_epochs},
        config=ray_config,
        mode="max",  # Maximize the metric
        checkpoint_freq=1,
        checkpoint_at_end=True,
        log_to_file=True,
        local_dir="./Models",
        resume=args.resume,
        name="{}_{}_seed-{}_{}".format(time.perf_counter(), args.method, str(args.seed), args.filename),
        verbose=1,
        max_failures=1
    )

    assert experiment_analysis.get_last_checkpoint() is not None

    print("\n\n\nLast checkpoint: ")
    print(experiment_analysis.get_last_checkpoint())
    print("\n\n\n")

    all_dfs = experiment_analysis.trial_dataframes
    names = list(all_dfs.keys())

    results = pd.DataFrame()

    for i in range(names):
        df = all_dfs[names[i]]
        df = df[
            [
                "timesteps_total",
                "episodes_total",
                "episode_reward_mean",
                "info/learner/default_policy/cur_kl_coeff",
            ]
        ]
        df["Agent"] = i
        results = pd.concat([results, df]).reset_index(drop=True)

    if args.save_csv:
        csv_dir = os.path.join(os.path.dirname(os.path.realpath(__file__)), "data")
        if not os.path.exists(csv_dir):
            os.makedirs(csv_dir)

        results.to_csv(csv_dir + "seed-{}.csv".format(str(args.seed)))