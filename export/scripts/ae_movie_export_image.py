from tools import tools
from tools import image_trimmer

version = 1

class MyTools(tools.Tools):
    def _initialize(self):
        self.add_argument_file('--in_path', 'in_path')
        self.add_argument_file('--out_path', 'out_path', exists=None)
        self.add_argument_dir('--export_path', 'export_path', exists=None)
        self.add_argument('--export_name', 'export_name')
        self.add_argument('--project_name', 'project_name')
        self.add_argument('--write_name', 'write_name')
        self.add_argument('--write_path', 'write_path')
        self.add_argument_integer('--image_border', 'image_border')
        self.add_argument_bool('--image_bezmesh', 'image_bezmesh')
        self.add_argument_bool('--image_trackmatte', 'image_trackmatte')
        self.add_argument_bool('--image_premultiplied', 'image_premultiplied')
        self.add_argument_bool('--out_premultiplied', 'out_premultiplied')
        self.add_argument_bool('--use_sha1', 'use_sha1')
        pass

    def _run(self, args):
        sha1path = None
        if args.use_sha1 is True:
            image_ext = self.get_ext(args.in_path)
            sha1hex, sha1path = self.get_file_sha1_new_path(args.in_path, image_ext)
            
            sha1params = dict(version=version
            , image_bezmesh=args.image_bezmesh
            , image_trackmatte=args.image_trackmatte
            , image_premultiplied=args.image_premultiplied
            , out_premultiplied=args.out_premultiplied
            , image_border=args.image_border)
            
            sha1tag = "export_image {}".format(sha1params)
            temp_path = self.get_sha1_cach_path(sha1tag, sha1hex + ".export_image")
            
            if args.out_path is not None:
                out_path = args.export_path + ".store_images" + "/" + sha1path
                pass
            else:
                out_path = None
                pass
            pass
        else:
            temp_path = self.get_temp_path("export_image.info")
            out_path = args.out_path
            pass

        if args.use_sha1 is False or ((args.out_path is not None and self.exist_file(out_path) is False) or self.exist_file(temp_path) is False):
            if out_path is not None:
                self.make_dir_for_file(out_path)
                pass
                
            if args.image_trackmatte is True and args.image_border == 0:
                args.image_border = 1
                pass
        
            premultiplied = args.image_premultiplied is False and args.out_premultiplied is True
            rv, info = image_trimmer.image_trimmer(args.in_path, dest_path = out_path, trim = (args.image_bezmesh is False), border = args.image_border, premultiplied = premultiplied)
            
            if rv != image_trimmer.ERROR_OK:
                error_message = image_trimmer.get_error_message(rv)
                print(error_message)
            
                return False
                pass
        
            base_width = info[0]
            base_height = info[1]
            trim_width = info[2]
            trim_height = info[3]
            offset_x = info[4]
            offset_y = info[5]
        
            with open(temp_path, "w") as f:
                f.write("{0}\n".format(base_width))
                f.write("{0}\n".format(base_height))
                f.write("{0}\n".format(trim_width))
                f.write("{0}\n".format(trim_height))
                f.write("{0}\n".format(offset_x))
                f.write("{0}\n".format(offset_y))
                pass
            pass
        else:            
            with open(temp_path, "r") as f:
                base_width = f.readline().rstrip('\n')
                base_height = f.readline().rstrip('\n')
                trim_width = f.readline().rstrip('\n')
                trim_height = f.readline().rstrip('\n')
                offset_x = f.readline().rstrip('\n')
                offset_y = f.readline().rstrip('\n')            
                pass
            pass

        if out_path is not None:
            print("out_path = {0}\n".format(out_path))
            pass

        print("base_width = {0}\n".format(base_width))
        print("base_height = {0}\n".format(base_height))
        print("trim_width = {0}\n".format(trim_width))
        print("trim_height = {0}\n".format(trim_height))
        print("offset_x = {0}\n".format(offset_x))
        print("offset_y = {0}\n".format(offset_y))

        if args.write_name is not None:
            if args.use_sha1 is True:
                print("write_name = {0}\n".format(sha1path))
            else:
                print("write_name = Movies2_{0}_{1}_{2}\n".format(args.export_name, "Image", args.write_name))
                pass
            pass

        if args.use_sha1 is True:
            image_write_path = sha1path
            pass
        else:
            image_write_path = args.write_path
            pass

        if image_write_path is not None:
            print("write_path = {0}\n".format(image_write_path))
            pass

        if args.image_premultiplied is False and args.out_premultiplied is True:
            print("premultiplied = 1\n")
            pass

        return True
        pass
    pass
    
tools.run(MyTools)