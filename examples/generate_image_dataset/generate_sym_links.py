import argparse
import os
import sys


if sys.platform == "linux":
    PLATFORM = "Linux"
elif sys.platform == "darwin":
    PLATFORM = "MacOS"
elif sys.platform == "win32":
    PLATFORM = "Windows"


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--paks_dir", type=str, required=True)
    parser.add_argument("--executable_content_dir", type=str, required=True)
    args = parser.parse_args()

    # get list of scenes from pak_dir
    scenes = os.listdir(args.paks_dir)

    # copy pak to the executable dir as this is required for launching the appropriate pak file
    assert os.path.exists(f"{args.executable_content_dir}/Paks")

    for scene in scenes:
        if not os.path.exists(f"{args.executable_content_dir}/Paks/{scene}_{PLATFORM}.pak"):
            os.symlink(os.path.join(args.paks_dir, f"{scene}/paks/{PLATFORM}/{scene}/{scene}_{PLATFORM}.pak"), f"{args.executable_content_dir}/Paks/{scene}_{PLATFORM}.pak")