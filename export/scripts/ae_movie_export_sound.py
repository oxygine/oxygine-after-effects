from tools import tools

class MyTools(tools.Tools):
    def _initialize(self):
        self.add_argument_file('--in_path', 'in_path')
        self.add_argument_file('--out_path', 'out_path', exists=None)
        self.add_argument_dir('--export_path', 'export_path', exists=None)
        self.add_argument('--export_name', 'export_name')
        self.add_argument('--project_name', 'project_name')
        self.add_argument('--write_name', 'write_name')
        self.add_argument('--write_path', 'write_path')
        self.add_argument_bool('--use_sha1', 'use_sha1')
        pass

    def _run(self, args):
        ffmpeg_path = self.get_tools_path("ffmpeg.exe")

        if self.exist_file(ffmpeg_path) is False:
            self.error_result("Not found ffmpeg.exe")

            return False
            pass

        sha1path = None
        if args.use_sha1 is True:
            sha1hex, sha1path = self.get_file_sha1_new_path(args.in_path, ".ogg")            
            sha1tag = "export_sound"
            temp_path = self.get_sha1_cach_path(sha1tag, sha1hex + ".export_sound")
            
            out_path = args.export_path + ".store_sounds" + "/" + sha1path
            pass
        else:
            temp_path = self.get_temp_path("export_sound.info")
            out_path = self.change_ext(args.out_path, "ogg")
            pass

        if args.use_sha1 is False or ((args.out_path is not None and self.exist_file(out_path) is False) or self.exist_file(temp_path) is False):
            if out_path is not None:
                self.make_dir_for_file(out_path)
                pass
            
            command_args = ["-loglevel", "error"
                , "-y"
                , "-threads", "4"
                , "-i", "{0}".format(args.in_path)
                , "-map_metadata", "-1"
                , "-ac", "2"
                , "-ar", "44100"
                , "-aq", "100"
                , "-acodec", "libvorbis"
                , "{0}".format(out_path)
                ]
        
            if self.command_call("ffmpeg sound", ffmpeg_path, command_args) is False:
                return False
                pass
                
            with open(temp_path, "w") as f:
                pass
            pass

        if args.use_sha1 is True:
            print("write_name = {0}\n".format(sha1path))
            pass
        else:
            print("write_name = Movies2_{0}_{1}_{2}\n".format(args.project_name, "Sound", args.write_name))
            pass

        if args.use_sha1 is True:
            ogg_write_path = self.change_ext(sha1path, "ogg")
            pass
        else:
            ogg_write_path = self.change_ext(args.write_path, "ogg")
            pass

        print("write_path = {0}\n".format(ogg_write_path))

        print("codec = 1\n")

        return True
        pass
    pass

tools.run(MyTools)
