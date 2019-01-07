import os
import sys

import ae_script
import logging
l = os.path.abspath(__file__)
t = os.path.split(l)[0]
loc = t + "/../"
loc = os.path.normpath(loc)
if loc not in sys.path:
    print loc
    sys.path.append(loc)


if __name__ == "__main__":
    import argparse

    out = ae_script.Out("preview.out")


    parser = argparse.ArgumentParser()
    parser.add_argument("--project_path")
    parser.add_argument("--composition_name")
    parser.add_argument("--work_area")

    args = parser.parse_known_args()[0]

   
    project = os.path.split(args.project_path)[0]
    
    os.chdir("..\\examples\\HelloViewerAE\\data\\")
    cmd = "HelloViewerAE.exe \"{}\" \"{}\" \"{}\"> viewer.log".format(project, args.composition_name, args.work_area)

    out.log.write("runing AEViewer...\n")
    out.log.write(cmd + "\n")
    out.log.flush()
    print cmd
    print("building zip... 4")
    os.system(cmd)
    out.close()
