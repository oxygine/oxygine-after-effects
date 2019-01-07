import os
import errno
import sys

import argparse

sha1cache_version = "3"

class PathType(object):
    def __init__(self, type='file', exists=True, dash_ok=True):
        assert exists in (True, False, None)
        assert type in ('file','dir','symlink',None) or hasattr(type,'__call__')

        self._exists = exists
        self._type = type
        self._dash_ok = dash_ok
        pass

    def __call__(self, string):
        if string=='-':
            if self._type == 'dir':
                raise argparse.ArgumentTypeError('standard input/output (-) not allowed as directory path')
            elif not self._dash_ok:
                raise argparse.ArgumentTypeError('standard input/output (-) not allowed')
        else:
            e = os.path.exists(string)
            if self._exists is True:
                if not e:
                    raise argparse.ArgumentTypeError("path does not exist: '%s'" % string)

                if self._type is None:
                    pass
                elif self._type=='file':
                    if not os.path.isfile(string):
                        raise argparse.ArgumentTypeError("path is not a file: '%s'" % string)
                    return os.path.abspath(string)
                elif self._type=='dir':
                    if not os.path.isdir(string):
                        raise argparse.ArgumentTypeError("path is not a directory: '%s'" % string)
                    return os.path.abspath(string)
                elif not self._type(string):
                    raise argparse.ArgumentTypeError("path not valid: '%s'" % string)
            else:
                if self._exists is False and e:
                    raise argparse.ArgumentTypeError("path exists: '%s'" % string)
                return os.path.abspath(string)
                pass
            pass

        return string
        pass
    pass

class BoolType(object):
    def __call__(self, string):
        if string in ['0', 'false', 'False']:
            return False
            pass
        elif string in ['1', 'true', 'True']:
            return True
            pass
        pass
    pass

class IntegerType(object):
    def __call__(self, string):
        return int(string)
        pass
    pass

class FloatType(object):
    def __call__(self, string):
        return float(string)
        pass
    pass

class Tools(object):
    def __init__(self):
        self.parser = argparse.ArgumentParser()
        
        self.add_argument('--tools_path', 'tools_path', type=PathType(type='dir'))
        
        temp_dir = self.get_temp_dir()
        self.make_dir(temp_dir)
        
        self._initialize()
        pass
    
    def _initialize(self):
        pass
    
    def add_argument(self, argument, help, **kwds):
        self.parser.add_argument(argument, help = help, **kwds)
        pass

    def add_argument_bool(self, argument, help):
        self.parser.add_argument(argument, help = help, type=BoolType())
        pass

    def add_argument_integer(self, argument, help):
        self.parser.add_argument(argument, help = help, type=IntegerType())
        pass

    def add_argument_float(self, argument, help):
        self.parser.add_argument(argument, help = help, type=FloatType())
        pass

    def add_argument_file(self, argument, help, **kwds):
        self.parser.add_argument(argument, help = help, type=PathType(type='file', **kwds))
        pass 
        
    def add_argument_dir(self, argument, help, **kwds):
        self.parser.add_argument(argument, help = help, type=PathType(type='dir', **kwds))
        pass             

    def add_arguments(self, argument, help, **kwds):
        self.parser.add_argument(argument, nargs = '*', help = help)
        pass

    def command_call(self, msg, process, args):
        import shlex
        import subprocess
        
        process_path = os.path.abspath(process)

        if sys.version_info > (3,0):
            proc = subprocess.Popen([process_path] + args, shell=True, encoding='utf-8', stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        else:
            proc = subprocess.Popen([process_path] + args, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            pass

        out, err = proc.communicate()
        exitcode = proc.returncode

        if exitcode != 0:
            if sys.version_info > (3, 0):
                err = err.encode("utf-8")
                pass

            self.error_result("{0} code error {1}:\n{2}\n{3}".format(msg, exitcode, out, err))
            
            return False
            pass

        return True
        pass

    def error_result(self, msg):
        print("# {0}".format(msg))
        pass
        
    def successful_result(self):
        print("@@@\n")
        pass
        
    def get_reg_value(self, KeyPath, ValueName, Error):
        if sys.version_info > (3, 0):
            import winreg as winreg
            pass
        else:
            import _winreg as winreg
            pass

        try:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, KeyPath, 0, winreg.KEY_QUERY_VALUE) as k:
                Value = winreg.QueryValueEx(k, ValueName)
                
                return str(Value[0])
                pass
        except Exception as ex:
            Error.append(type(ex))
            Error.append(str(ex))
            pass
            
        return None 
        pass

    def make_dir(self, dirname):
        if not os.path.exists(dirname):
            try:
                os.makedirs(dirname)
                pass
            except OSError as exc: # Guard against race condition
                if exc.errno != errno.EEXIST:
                    raise
                    pass
                pass
            pass
        pass
        
    def make_dir_for_file(self, filepath):
        result_dirname = os.path.dirname(filepath)
        self.make_dir(result_dirname)
        pass

    def exist_file(self, filepath):
        if filepath is None:
            return False
            pass
    
        return os.path.isfile(filepath)
        pass
        
    def get_file_sha1(self, filepath):
        import hashlib

        f = open(filepath, "rb")

        sha1 = hashlib.sha1()

        while True:
            data = f.read(65536)
            if not data:
                break
                pass

            sha1.update(data)
            pass

        sha1hex = sha1.hexdigest()
        
        return sha1hex
        pass
        
    def get_text_sha1(self, text):
        import hashlib
        sha1 = hashlib.sha1(bytes(text, 'utf-8'))
        sha1hex = sha1.hexdigest()
        return sha1hex
        pass
        
    def get_file_sha1_new_path(self, filepath, ext):
        sha1hex = self.get_file_sha1(filepath)
        sha1path = sha1hex[:2] + "/" + sha1hex[2:] + ext
        
        return sha1hex, sha1path
        pass        
        
    def run(self):
        try:
            self._pre_run()
        except Exception as e:
            print("pre run except: {0}".format(e))
            pass
        
        try:
            if self._run(self.args) is True:
                self.successful_result()
                pass
        except Exception as e:
            print("run except: {0}".format(e))
            pass

        try:
            self._post_run(self.args)
        except Exception as e:
            print("post run except: {0}".format(e))
            pass
        pass
        
    def _pre_run(self):
        self.args = self.parser.parse_args()        
        pass
        
    def _run(self, args):
        return True
        pass

    def _post_run(self, args):
        pass
        
    def get_temp_dir(self):
        import tempfile
        temp_dir = tempfile.gettempdir()
        return os.path.join(temp_dir, ".libmovie")
        pass        

    def get_temp_path(self, filename):
        temp_dir = self.get_temp_dir()
        return os.path.join(temp_dir, filename)
        pass
        
    def get_sha1_cach_path(self, tag, filename):
        temp_dir = self.get_temp_dir()
        tag_sha1 = self.get_text_sha1(tag)
        path = os.path.join(temp_dir, '.sha1cache{0}'.format(sha1cache_version), tag_sha1, filename[:2], filename[2:])
        self.make_dir_for_file(path)
        return path
        pass        
        
    def get_tools_path(self, filename):
        return os.path.join(self.args.tools_path, "tools", filename)
        pass

    def get_ext(self, filepath):
        return os.path.splitext(filepath)[1]
        pass

    def change_ext(self, filepath, newext):
        return os.path.splitext(filepath)[0] + '.' + newext
        pass
        
    def get_prefix_sha1path(self, sha1path):
        return os.path.split(sha1path)[0]
        pass

    def copy_file(self, src, dst):
        from shutil import copyfile
        copyfile(src, dst)
        pass
    pass

def run(tools):
    t = tools()
    t.run()
    pass