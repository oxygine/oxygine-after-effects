# -*- coding: utf-8 -*-

from xml.sax.saxutils import quoteattr

import ae_script
import glob


def gen_xml(path, out):
    filelist = glob.glob(path + "/images/*.jpeg")
    filelist.extend(glob.glob(path + "/images/*.jpg"))
    filelist.extend(glob.glob(path + "/images/*.tga"))
    filelist.extend(glob.glob(path + "/images/*.png"))
    print(filelist)
    print(path)

    dest = open(out, "w")
    images = path + "/images"
    write = dest.write

    write("<resources>\n")
    write("\t<set path=\"%s\"/>\n" % ("images", ))

    
    write("\t<atlas>\n")

    for file in filelist:
        name = os.path.split(file)[1]        
        #print ("{}-{}" .format(name, quoteattr(name)))
        name = quoteattr(name)
        write("\t\t<image file=%s/>\n" % (name))

    
    write("\t</atlas>\n")

    write("</resources>\n")
    dest.close()


import os
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("aem")
parser.add_argument("--movie_folder")

args = parser.parse_known_args()[0]

out = ae_script.Out("postbuild.out")
dest = args.movie_folder

try:
    os.makedirs(dest + "/images")
except OSError:
    pass

gen_xml(dest, dest + "/res.xml", )
out.close()