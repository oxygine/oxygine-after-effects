from tools import tools

class MyTools(tools.Tools):
    def _initialize(self):
        self.add_argument_file('--in_path', 'in_path')
        self.add_argument_file('--out_path', 'out_path', exists=None)
        self.add_argument_dir('--export_path', 'export_path', exists=None)
        self.add_argument('--export_name', 'export_name')
        self.add_argument('--project_name', 'project_name')
        self.add_argument('--movie_name', 'movie_name')
        self.add_argument_bool('--use_sha1', 'use_sha1')
        pass
        
    def __compiler(self, args):
        command_path = self.get_tools_path("MovieCompiler.exe")

        if self.exist_file(command_path) is False:
            self.error_result("Not found MovieCompiler.exe")

            return False
            pass

        command_args = []
        
        in_path = args.in_path
        out_path = self.change_ext(args.out_path, "aez")

        command_args.extend(["--in_path", in_path])
        command_args.extend(["--out_path", out_path])

        if self.command_call("postbuild", command_path, command_args) is False:
            return False
            pass

        return True
        pass
        
    def __resource(self, args):
        command_path = self.get_tools_path("MovieResource.exe")
        
        if self.exist_file(command_path) is False:
            self.error_result("Not found MovieResource.exe")

            return False
            pass
            
        command_args = []

        in_path = args.in_path
        out_path = self.change_ext(args.out_path, "xml")

        command_args.extend(["--in_path", in_path])
        command_args.extend(["--out_path", out_path])
        command_args.extend(["--movie_name", args.movie_name])

        if self.command_call("postbuild", command_path, command_args) is False:
            return False
            pass

        return True
        pass

    def _run(self, args):
        if args.project_name == "None":
            self.copy_file(args.in_path, args.out_path)
        
            return True
            pass
        
        if self.__resource(args) is False:
            return False
            pass            
        
        if self.__compiler(args) is False:
            return False
            pass
            
        return True
        pass
    pass
    
tools.run(MyTools)