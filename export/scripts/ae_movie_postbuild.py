import sys
import os

args = ""
for arg in sys.argv[1:]:
    args += "\"" + arg + "\"" + " "

os.chdir(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

with open("temp/postbuild.args", "w") as w:
    w.write(args)

os.system("python postbuild.py " + args)

with open("temp/postbuild.out", "r") as out:
    print(out.read())