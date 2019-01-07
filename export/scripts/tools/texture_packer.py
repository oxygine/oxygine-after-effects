import tempfile
import os
import subprocess
import glob
import json

ERROR_OK=0
ERROR_INVALID_IN_PATH=1
ERROR_NO_SRC_IMAGES=2
ERROR_TEXTURE_PACKER_NOT_FOUND=3
ERROR_TEXTURE_PACKER_FAILED=4

def load_filelist(path):
  try:
    with open(path,'r') as f:
      s=f.read()
    rv=[line.strip() for line in s.split('\n')]
    return ERROR_OK,rv
  except:
    return ERROR_INVALID_IN_PATH,None

def find_texture_packer():
  return ERROR_OK,'C:\Program Files\CodeAndWeb\TexturePacker\\bin\TexturePacker.exe'
#  return ERROR_TEXTURE_PACKER_NOT_FOUND,None

class Image:
  def to_line(self):
    return '{name};{atlas};{aw};{ah};{ox};{oy};{ow};{oh};{fw};{fh};{x0};{y0};{x1};{y1};{x2};{y2};{x3};{y3};{rotated:d}'.format(**self.__dict__)

def process_json(info):
  images=[]
  atlas_name=info['meta']['image']
  atlas_w=info['meta']['size']['w']
  atlas_h=info['meta']['size']['h']
  for frame in info['frames']:
    x=frame['frame']['x']
    y=frame['frame']['y']
    w=frame['frame']['w']
    h=frame['frame']['h']
    image=Image()
    image.name=frame['filename']
    image.atlas=atlas_name
    image.aw=atlas_w
    image.ah=atlas_h
    image.ox=frame['spriteSourceSize']['x']
    image.oy=frame['spriteSourceSize']['y']
    image.ow=w
    image.oh=h
    image.fw=frame['sourceSize']['w']
    image.fh=frame['sourceSize']['h']
    image.rotated=frame['rotated']
    if image.rotated:
      image.x0=x+h
      image.y0=y
      image.x1=x+h
      image.y1=y+w
      image.x2=x
      image.y2=y+w
      image.x3=x
      image.y3=y
    else:
      image.x0=x
      image.y0=y
      image.x1=x+w
      image.y1=y
      image.x2=x+w
      image.y2=y+h
      image.x3=x
      image.y3=y+h
    images.append(image)
  d=[]
  for image in images:
    l=image.to_line()
    d.append(l)
  return d

def process(in_path,out_path,texture_packer_path=None,log_path=None,premultiply=False):
  error,src_images=load_filelist(in_path) if isinstance(in_path,str) else (ERROR_OK,in_path)
  if error!=ERROR_OK:
    return error,None
  if len(src_images)==0:
    return ERROR_NO_SRC_IMAGES,None

  error,texture_packer_path=find_texture_packer() if texture_packer_path is None else (ERROR_OK,os.path.abspath(texture_packer_path))
  if error!=ERROR_OK:
    return error,None
  if out_path is None:
    out_path='.'
  out_path=os.path.abspath(out_path)
  log_path=os.path.abspath(os.devnull if log_path is None else log_path)
  temp_path=os.path.abspath(tempfile.gettempdir()+'/.libmovie/TexturePacker')
  data_path=os.path.abspath(temp_path+'/data')
  json_path=os.path.abspath(data_path+'/atlas_{n}.json')
  sheet_path=os.path.abspath(out_path+'/atlas_{n}.png')

  for fn in glob.glob(os.path.abspath(data_path+'/*.json')):
    os.remove(fn)

  cmd_line=[]
  cmd_line.extend([texture_packer_path])
  cmd_line.extend(['--multipack'])
  cmd_line.extend(['--enable-rotation'])
  cmd_line.extend(['--trim-mode','Trim'])
  cmd_line.extend(['--size-constraints','POT'])
  cmd_line.extend(['--data',json_path])
  cmd_line.extend(['--sheet',sheet_path])
  cmd_line.extend(['--format','json-array'])
  cmd_line.extend(['--texture-format','png'])
  if premultiply:
    cmd_line.extend(['--alpha-handling','PremultiplyAlpha'])
  cmd_line.extend(['--max-width','2048'])
  cmd_line.extend(['--max-height','2048'])
  cmd_line.extend(['--max-size','2048'])

  for src_image in src_images:
    src_image=src_image.strip()
    if src_image!='':
      cmd_line.extend([os.path.abspath(src_image)])

  with open(log_path,'w') as log_file:
    try:
      exit_code=subprocess.call(cmd_line,stdout=log_file,stderr=log_file,shell=True)
    except:
      return ERROR_TEXTURE_PACKER_NOT_FOUND,None
  if exit_code!=0:
    return ERROR_TEXTURE_PACKER_FAILED,exit_code

  data=[]
  for json_fn in glob.glob(os.path.abspath(data_path+'/*.json')):
    with open(json_fn,'r') as f:
      j = json.load(f)
      d = process_json(j)
      data.extend(d)

  return ERROR_OK, (len(data), data)

#if __name__=='__main__':
#  print(process('test.txt','.','./result.txt','C:\Program Files\CodeAndWeb\TexturePacker\\bin\TexturePacker.exe','texture_packer.log',True))
#  print(process('test.txt','.','./result.txt','C:\Program Files\CodeasdsadasdAndWeb\TexturePacker\\bin\TexturePacker.exe','texture_packer.log',True))
#  print(process(['bishop-icon.png','test with spaces.png'],'.',None,None,None,False))
