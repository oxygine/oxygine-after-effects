import tempfile
import os
import subprocess
import glob
import xml.etree.ElementTree as ET

ERROR_OK=0
ERROR_INVALID_IN_PATH=1
ERROR_TEXTURE_PACKER_NOT_FOUND=3
ERROR_TEXTURE_PACKER_FAILED=4

def find_texture_packer():
  return ERROR_OK,'c:\Program Files\CodeAndWeb\TexturePacker\\bin\TexturePacker.exe'
#  return ERROR_TEXTURE_PACKER_NOT_FOUND,None

def split1(f, text):
  return map(f, text.split(' '))
  pass

def split2(f, text):
  vv = [v for v in map(f, text.split(' '))]
  return zip(vv[::2], vv[1::2])
  pass
  
def getf(f, v, a, d):    
    s=v.get(a, d)
    return d if s is None else f(s)

def process_sprite_xml(sprite,offset_x,offset_y,width,height):
  x=getf(int,sprite,'x', 0)
  y=getf(int,sprite,'y', 0)
  w=getf(float,sprite,'w', 0)
  h=getf(float,sprite,'h', 0)
  ox=getf(int,sprite,'oX', x)
  oy=getf(int,sprite,'oY', y)
  ow=getf(float,sprite,'oW', w)
  oh=getf(float,sprite,'oH', h)
  
  if width<0.0:width=w
  if height<0.0:height=h

  positions=[]
  for vx,vy in split2(int, sprite.find('vertices').text):
    px=vx-x+offset_x
    py=vy-y+offset_y
    positions.extend((px,py))

  uvs=[]
  for uvx,uvy in split2(int, sprite.find('verticesUV').text):
    pu=uvx-x+ox
    pv=uvy-y+oy
    uvs.extend((float(pu)/float(width),float(pv)/float(height)))

  indices=[]
  for tri in split1(int, sprite.find('triangles').text):
    indices.append(tri)

  vertex_count=len(positions)//2
  index_count=len(indices)

  return [vertex_count,index_count,positions,uvs,indices]

def process(in_path,temp_dir=None,texture_packer_path=None,log_path=None,offset_x=0,offset_y=0,width=-1.0,height=-1.0,tolerance=200):
  error,texture_packer_path=find_texture_packer() if texture_packer_path is None else (ERROR_OK,os.path.abspath(texture_packer_path))
  if error!=ERROR_OK:
    return error,None
  in_path=os.path.abspath(in_path)
  log_path=os.path.abspath(os.devnull if log_path is None else log_path)
  
  if temp_dir is None:
    temp_dir=tempfile.gettempdir()
    pass
  data_path=os.path.join(temp_dir, 'ae_image_polygonize_data.xml')

  cmd_line=[]
  cmd_line.extend([texture_packer_path])
  cmd_line.extend(['--shape-padding','0'])
  cmd_line.extend(['--border-padding','0'])
  cmd_line.extend(['--padding','0'])
  cmd_line.extend(['--disable-rotation'])
  cmd_line.extend(['--extrude','0'])
  cmd_line.extend(['--trim-mode','Polygon'])
  cmd_line.extend(['--trim-threshold','0'])
  cmd_line.extend(['--tracer-tolerance',str(tolerance)])
  cmd_line.extend(['--max-width','8192'])
  cmd_line.extend(['--max-height','8192'])
  cmd_line.extend(['--max-size','8192'])
  cmd_line.extend(['--data',data_path])
#  cmd_line.extend(['--format','json-array'])
  cmd_line.extend(['--format','xml'])
  cmd_line.extend([in_path])

  with open(log_path,'w') as log_file:
    try:
      exit_code=subprocess.call(cmd_line,stdout=log_file,stderr=log_file,shell=True)
    except:
      return ERROR_TEXTURE_PACKER_NOT_FOUND,None
  if exit_code!=0:
    return ERROR_TEXTURE_PACKER_FAILED,exit_code
    
  tree=ET.parse(data_path)
  root=tree.getroot()
  for sprite in root.findall('.//sprite'):
    info=process_sprite_xml(sprite,offset_x,offset_y,width,height)

  return ERROR_OK,info

#if __name__=='__main__':
#  process('bishop-icon.png','test.txt',log_path='./test.log')
#  print(process('bishop-icon.png','test.txt',log_path='./test.log'))
