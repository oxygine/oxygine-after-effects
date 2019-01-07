from tools import tools
from tools import image_polygonize

class MyTools(tools.Tools):
    def _initialize(self):
        self.add_argument_file('--in_path', 'in_path')
        self.add_argument_file('--source_path', 'source_path')
        self.add_argument_float('--offset_x', 'offset_x')
        self.add_argument_float('--offset_y', 'offset_y')
        self.add_argument_float('--width', 'width')
        self.add_argument_float('--height', 'height')
        self.add_argument_integer('--tolerance', 'tolerance')
        self.add_argument_bool('--use_sha1', 'use_sha1')
        pass

    def _run(self, args):
        if args.use_sha1 is True:
            sha1hex = self.get_file_sha1(args.source_path)
            sha1tag = "image_polygonize {0}:{1} {2}:{3} {4}".format(args.offset_x, args.offset_y, args.width, args.height, args.tolerance)
            temp_path = self.get_sha1_cach_path(sha1tag, sha1hex + ".image_polygonize")
            pass
        else:
            temp_path = self.get_temp_path("image_polygonize.info")
            pass
            
        if args.use_sha1 is False or self.exist_file(temp_path) is False:
            temp_dir = self.get_temp_dir()
            log_path = self.get_temp_path("ae_export_atlas.log")
            rv, info = image_polygonize.process(args.in_path, temp_dir=temp_dir, log_path=log_path, offset_x=args.offset_x, offset_y=args.offset_y, width=args.width, height=args.height, tolerance=args.tolerance)
            
            with open(temp_path,'w') as f:
                f.write(str(info[0])+'\n')
                f.write(str(info[1])+'\n')
                f.write(', '.join(map(str, info[2]))+'\n')
                f.write(', '.join(map(str, info[3]))+'\n')
                f.write(', '.join(map(str, info[4]))+'\n')
                pass
            pass
            
        with open(temp_path, "r") as f:
            vertex_count = f.readline().rstrip('\n')
            index_count = f.readline().rstrip('\n')
            positions = f.readline().rstrip('\n')
            uvs = f.readline().rstrip('\n')
            indices = f.readline().rstrip('\n')
            pass
            
        print("vertex_count = {0}\n".format(vertex_count))
        print("index_count = {0}\n".format(index_count))
        print("positions = {0}\n".format(positions))
        print("uvs = {0}\n".format(uvs))
        print("indices = {0}\n".format(indices))

        return True
        pass
    pass

tools.run(MyTools)