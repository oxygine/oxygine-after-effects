
import sys
import os
import ae_script
import ConfigParser
import sys
import logging

def getValue(confif, section, key, default):
    value = default
    try:
        value = config.get(section, key)
    except ConfigParser.NoOptionError:
        pass

    return value

class FakeSecHead(object):
    def __init__(self, fp):
        self.fp = fp
        self.sechead = '[asection]\n'

    def readline(self):
        if self.sechead:
            try:
                return self.sechead
            finally:
                self.sechead = None
        else:
            return self.fp.readline()


if __name__ == "__main__":

    out = ae_script.Out("settings.out")

    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("--project_path")

    def str2bool(v):
        if v.lower() in ('yes', 'true', 't', 'y', '1'):
            return True
        elif v.lower() in ('no', 'false', 'f', 'n', '0'):
            return False
        else:
            raise argparse.ArgumentTypeError('Boolean value expected.')

    parser.add_argument("--preview", type=str2bool)


    args = parser.parse_known_args()[0]

    dest = ""

    def eprint(arg):
        out.write(arg)


    export_name = ""

    if args.preview:
        import tempfile
        dest = tempfile.mkdtemp(prefix="aep_") + "/"
    else:
        aep = args.project_path
        #aep = """d:\Dropbox (Alisa)\ALISA_SLOTS\ASSETS2\AE\games\eye_of_horus\ui\ui.aep"""

        def get_level_name(left, level):
            for i in xrange(level + 1):
                left, right = os.path.split(left)


            right = os.path.splitext(right)[0]
            return right

        folder = aep
        CONF = ".ae"

        while folder:
            sp = os.path.split(folder)
            if folder == sp[0]:
                break
            folder = sp[0]

            path = os.path.join(folder, CONF)
            if os.path.exists(path):
                break

        import ConfigParser
        config = ConfigParser.SafeConfigParser()


        try:
            config.readfp(FakeSecHead(open(path)))
            section = "asection"

            dest = getValue(config, section, "dest", None)

            dest = dest.replace("${aep}",   get_level_name(aep, 0))
            dest = dest.replace("${aep-0}", get_level_name(aep, 0))
            dest = dest.replace("${aep-1}", get_level_name(aep, 1))
            dest = dest.replace("${aep-2}", get_level_name(aep, 2))
            dest = dest.replace("${aep-3}", get_level_name(aep, 3))
            dest = dest.replace("${aep-4}", get_level_name(aep, 4))

            dest = os.path.normpath(os.path.split(path)[0] + "/" + dest)
            dest = os.path.abspath(dest)

            dest += "\\"

            export_name = getValue(config, section, "name", "")

        except IOError:
            dest = ""


        #open("project_settings.log", "a").write("{} -> {}\n".format(aep, dest))


    eprint("export_path={}".format(dest, ))
    if export_name:
        eprint("export_name={}".format(export_name, ))

    out.write_settings()
    out.close()