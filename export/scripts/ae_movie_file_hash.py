from tools import tools
from tools import image_trimmer

class MyTools(tools.Tools):
    def _initialize(self):
        self.add_argument_file('--in_path', 'in_path')
        pass

    def _run(self, args):
        sha1hex = self.get_file_sha1(args.in_path)

        print("hash = {0}\n".format(sha1hex))

        return True
        pass
    pass
    
tools.run(MyTools)