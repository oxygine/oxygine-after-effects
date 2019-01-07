import sys
import os
import ae_script

if __name__ == "__main__":

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--cfg_path")

    args = parser.parse_known_args()[0]


    out = ae_script.Out("multiexport.out")
    dest = ""

    def eprint(arg):
        out.write(arg)

    eprint("[settings]")
    out.write_settings()
    
    eprint("[data]")

    def go(path):
        items = os.listdir(path)
        for item in items:
            folder = os.path.join(path, item)
            if os.path.isdir(folder):
                v = folder
                r = os.path.split(v)[1]
                aep = "{}/{}.aep".format(folder, r)
                if os.path.exists(aep):
                    eprint("in={}".format(aep))
                go(folder)

    conf = args.cfg_path
    root = os.path.split(conf)[0]
    go(root)

    out.close()