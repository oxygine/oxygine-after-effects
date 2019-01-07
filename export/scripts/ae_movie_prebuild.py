import sys
import os

args = ""
for arg in sys.argv[1:]:
    args += "\"" + arg + "\"" + " "

os.chdir(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

with open("temp/prebuild.args", "w") as w:
    w.write(args)

os.system("python prebuild.py " + args)

with open("temp/prebuild.out", "r") as out:
    print(out.read())