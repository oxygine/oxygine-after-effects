import sys
import os

args = ""
for arg in sys.argv[1:]:
    args += "\"" + arg + "\"" + " "

os.chdir(os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

with open("temp/settings.args", "w") as w:
    w.write(args)

os.system("python settings.py " + args)

with open("temp/settings.out", "r") as out:
    print(out.read())