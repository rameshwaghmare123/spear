#
# Copyright(c) 2022 Intel. Licensed under the MIT License <http://opensource.org/licenses/MIT>.
#

import argparse
import os 
import shutil
import spear
import sys


if __name__ == "__main__":

    parser = argparse.ArgumentParser()
    parser.add_argument("--project_dir", default=os.path.realpath(os.path.join(os.path.dirname(__file__), "..", "cpp", "unreal_projects", "SpearSim")))
    args = parser.parse_args()

    # entire directories to be removed
    dirs = [
        os.path.realpath(os.path.join(args.project_dir, "Binaries")),
        os.path.realpath(os.path.join(args.project_dir, "Build")),
        os.path.realpath(os.path.join(args.project_dir, "DerivedDataCache")),
        os.path.realpath(os.path.join(args.project_dir, "Intermediate")),
        os.path.realpath(os.path.join(args.project_dir, "Saved"))]

    # individual files to be removed
    files = []

    # based on the platform, add project generated files or directories
    if sys.platform == "win32":
        dirs.append(os.path.realpath(os.path.join(args.project_dir, ".vs")))
        files.extend([
                os.path.realpath(os.path.join(args.project_dir, ".vsconfig")),
                os.path.realpath(os.path.join(args.project_dir, "SpearSim.sln"))])

    # add plugin specific directories to the list of dirs to be removed
    plugins_dir = os.path.realpath(os.path.join(args.project_dir, "Plugins"))
    plugins = os.listdir(plugins_dir)
    for plugin in plugins:
        plugin_dir = os.path.realpath(os.path.join(plugins_dir, plugin))
        dirs.extend([
                os.path.realpath(os.path.join(plugin_dir, "Binaries")),
                os.path.realpath(os.path.join(plugin_dir, "Intermediate"))])
    
    # remove dirs
    for dir in dirs:
        if os.path.exists(dir):
            spear.log("Removing: ", dir)
            shutil.rmtree(dir, ignore_errors=True)

    # remove files
    for file in files:
        if os.path.exists(file):
            spear.log("Removing: ", file)
            os.remove(file)

    spear.log("Done.")