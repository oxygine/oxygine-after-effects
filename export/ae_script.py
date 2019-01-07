
import sys
import os


import sys

class Out:
    def __init__(self, out_name):
        self.log = open("temp/log", "a")
        
        import  traceback
        def exception_hook(exc_type, exc_value, exc_traceback):
            self.log.write("exception!!!\n")
            res = traceback.format_exception(exc_type, exc_value, exc_traceback)
            self.log.writelines(res)
            self.log.write("\n\n")
        sys.excepthook = exception_hook

        out_name = "temp/{}".format(out_name, )

        self.fh = open(out_name, "w")        
        import datetime
        self.log.write("<<<<<<<<<<<< {}\n".format(datetime.datetime.now(), ))
        self.log.write(str(sys.argv) + "\n")
        self.log.write("-----------\n")

    def close(self):
        self.fh.write("@@@\n")
        self.fh.close()
        self.fh = None
        self.log.write(">>>>>>>>>>>\n")
        self.log = None

    def write(self, arg):
        print (arg)
        self.fh.write(arg + "\n")
        self.log.write("{}\n".format(arg))

    def write_settings(self):
        self.write("pure_video=true")
        self.write("trim_image=true")
        
        self.write("image_polygonize=true")
        self.write("image_polygonize_minimum_square=10000")
        self.write("image_polygonize_tolerance=1500")

        self.write("allow_image_premultiplied=false")
        
        
        self.write("use_sha1=false")        
        self.write("allow_sha1=false")
        self.write("allow_atlas=false")
