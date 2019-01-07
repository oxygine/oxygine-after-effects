import sys
import os
import shutil
import ae_script
import argparse

if __name__ == "__main__":
    out = ae_script.Out("prebuild.out")

    parser = argparse.ArgumentParser()
    parser.add_argument("aem")
    parser.add_argument("--movie_folder")

    args = parser.parse_known_args()[0]

    try:
        shutil.rmtree(args.movie_folder)
    except OSError:
        pass

    os.makedirs(args.movie_folder)

    out.close()