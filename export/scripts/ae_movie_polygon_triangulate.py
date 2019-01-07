from tools import tools
from tools import polygon_triangulator

class MyTools(tools.Tools):
    def _initialize(self):
        self.add_argument_bool('--bb', 'bb')
        self.add_argument_float('--base_width', 'base_width')
        self.add_argument_float('--base_height', 'base_height')
        self.add_argument_float('--trim_width', 'trim_width')
        self.add_argument_float('--trim_height', 'trim_height')
        self.add_argument_float('--trim_offset_x', 'trim_offset_x')
        self.add_argument_float('--trim_offset_y', 'trim_offset_y')
        self.add_argument_bool('--subtract', 'subtract')
        self.add_arguments('--points', help = 'points')
        pass

    def _run(self, args):
        rv, data = polygon_triangulator.process(args.bb, args.base_width, args.base_height, args.trim_width, args.trim_height, args.trim_offset_x, args.trim_offset_y, args.subtract, args.points[0])
        
        vertex_count = data[0]
        index_count = data[1]
        positions = data[2]
        uvs = data[3]
        indices = data[4]
    
        print("vertex_count = {0}\n".format(vertex_count))
        print("index_count = {0}\n".format(index_count))
        print("positions = {0}\n".format(", ".join(map(str, positions))))
        print("uvs = {0}\n".format(", ".join(map(str, uvs))))
        print("indices = {0}\n".format(", ".join(map(str, indices))))

        return True
        pass
    pass

tools.run(MyTools)