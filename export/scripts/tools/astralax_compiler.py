import tempfile
import os
import subprocess
import platform
import struct
import time

from tools import pylz4

from ctypes import *

ERROR_OK=0
ERROR_INVALID_IN_PATH=1
ERROR_ASTRALAX_NOT_FOUND=3
ERROR_ASTRALAX_FAILED=4

def process(in_path,out_path,csa_path,convert,astralax_path, lib_path):
  in_path=os.path.abspath(in_path)
  csa_path=os.path.abspath(csa_path)
  if out_path is None:
    convert=False
    out_path=tempfile.mktemp(suffix=".ptc", dir=tempfile.gettempdir())
  out_path=os.path.abspath(out_path)
  
  cmd_line=[]
  cmd_line.extend([astralax_path])
  cmd_line.extend(['/c'])
  cmd_line.extend([in_path])
  cmd_line.extend([out_path])
  cmd_line.extend([csa_path])

  try:
    startupinfo = None
    if platform.system() == 'Windows':
      startupinfo = subprocess.STARTUPINFO()
      startupinfo.wShowWindow = subprocess.SW_HIDE
      
    p = subprocess.Popen(cmd_line, shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    out, err = p.communicate()
    exitcode = p.returncode
    if exitcode != 0:
        return ERROR_ASTRALAX_NOT_FOUND,None
  except Exception as ex:
    print(ex)
    return ERROR_ASTRALAX_NOT_FOUND,None
    
  MagicLibrary=cdll.LoadLibrary(lib_path)
  
  with open(out_path,'rb') as f:
    data=f.read()
  mf=MagicLibrary.Magic_OpenFileInMemory(data)
  
  if convert is True:
    compressed=pylz4.pylz4_compressHC(data, 12)
  
    class Header(Structure):
      _fields_=[
        ('number',c_int),
        ('version',c_int),
        ('crc32',c_int),
        ('uncompressed_size',c_int),
        ('compressed_size',c_int),
        ]
  
    header=Header()
  
    header.number=struct.unpack('<i', b'PTZ2')[0]
    header.version=2
    header.crc32=0
    header.uncompressed_size=len(data)
    header.compressed_size=len(compressed)
    
    converted_path=out_path.rsplit('.',1)[0]+'.ptz'
    with open(converted_path,'wb') as f:
      f.write(header)
      f.write(compressed)
  
  atlas_count=MagicLibrary.Magic_GetStaticAtlasCount(mf)
  
  class StaticAtlas(Structure):
    _fields_=[
      ('file',c_char_p),
      ('path',c_char_p),
      ('width',c_int),
      ('height',c_int),
      ('ptc_id',c_int),
      ]
  PStaticAtlas=POINTER(StaticAtlas)
  atlas=StaticAtlas()
  
  s=[]
  for n in range(0,atlas_count):
    MagicLibrary.Magic_GetStaticAtlas(mf,n,PStaticAtlas(atlas))
    s.append((atlas.file, 0, 0, atlas.width, atlas.height))
  MagicLibrary.Magic_CloseFile(mf)
  
  return ERROR_OK, s
