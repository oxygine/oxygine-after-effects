try:
  import PIL
except ImportError:
  import os
  print("Trying to Install required module: Pillow\n")
  os.system('python get-pip.py')
  os.system('python -m pip install Pillow')
  pass
  
try:
  import pyclipper
except ImportError:
  import os
  print("Trying to Install required module: pyclipper\n")
  os.system('python get-pip.py')
  os.system('python -m pip install pyclipper')
  pass

from PIL import Image,ImageDraw

import earcut
import pyclipper

ERROR_OK=0
ERROR_INVALID_POINTS=1

SCALE_FACTOR=float(2**32) #100000000.0

def scale_up(x):
  return x*SCALE_FACTOR

def scale_down(x):
  return x/SCALE_FACTOR

def parse_points(s):
  try:
    rv=[scale_up(float(v.strip())) for v in s.split()]
    rv=tuple(zip(*[iter(rv)]*2))
    return ERROR_OK,rv
  except:
    return ERROR_INVALID_POINTS,None

def build_bb(x,y,w,h):
  x=scale_up(x)
  y=scale_up(y)
  w=scale_up(w)
  h=scale_up(h)
  return ((x,y),(x+w,y),(x+w,y+h),(x,y+h))
#  return ((0,0),(w-x*2,0),(w-x*2,h-y*2),(0,h-y*2))

def apply_mask(src,mask,subtract):
  clipper=pyclipper.Pyclipper()
  clipper.AddPath(src,pyclipper.PT_SUBJECT,True)
  try: 
    clipper.AddPath(mask,pyclipper.PT_CLIP,True)
  except pyclipper.ClipperException as ex:
    return None
    pass
    
  mode=pyclipper.CT_DIFFERENCE if subtract else pyclipper.CT_INTERSECTION
  rv=clipper.Execute(mode,pyclipper.PFT_EVENODD,pyclipper.PFT_EVENODD)
  return rv

def order_clipped(polygons):
  hulls=[]
  holes=[]
  for poly in polygons:
    if pyclipper.Orientation(poly):
      hulls.append(poly)
    else:
      holes.append(poly)
  return hulls,holes

def triangulate(hulls,holes):
  rv=[]
  for hull in hulls:
    data=earcut.flatten([hull]+holes)
    tris=earcut.earcut(data['vertices'],data['holes'],data['dimensions'])
    rv.append([data['vertices'],tris])
  return rv

def combine_meshes(meshes):
  max_index=0
  rv_vertices={}
  rv_indices=[]
  for vertices,indices in meshes:
    vertices=tuple(zip(*[iter(vertices)]*2))
    for vertex in vertices:
      if vertex not in rv_vertices:
        rv_vertices[vertex]=max_index
        max_index+=1
    for index in indices:
      rv_indices.append(rv_vertices[vertices[index]])
  rv_vertices=[coord for vertex in sorted(rv_vertices,key=rv_vertices.get) for coord in vertex]
  return rv_vertices,rv_indices

def generate_uv(x,y,w,h,vertices):
  rv=[]
  for n in range(0,len(vertices)//2):
    rv.append((vertices[n*2]-x)/w)
    rv.append((vertices[n*2+1]-y)/h)
  return rv

def process(bb,base_width,base_height,trim_width,trim_height,trim_offset_x,trim_offset_y,subtract,points):
  if bb:
    src=build_bb(trim_offset_x,trim_offset_y,trim_width,trim_height)
    error,mask=parse_points(points)
    if error!=ERROR_OK:
      return error,None
    clipped=apply_mask(src,mask,subtract)
  else:
    error,mask=parse_points(points)
    if error!=ERROR_OK:
      return error,None
    clipped=apply_mask(mask,mask,False)
    
  if clipped is None:
    return ERROR_OK, (0, 0, [], [], [])
    pass
    
  hulls,holes=order_clipped(clipped)
  meshes=triangulate(hulls,holes)
  vertices,indices=combine_meshes(meshes)
  vertices=[scale_down(coord) for coord in vertices]
  uv=generate_uv(trim_offset_x,trim_offset_y,trim_width,trim_height,vertices)
  return ERROR_OK,(len(vertices)//2,len(indices),vertices,uv,indices)

#if __name__=='__main__':
#  print(process(True,242.0,284.0,245.0,287.0,-1.0,-1.0,False,'163.39762878418 -22.0001831054688 69.4337463378906 -18.7153472900391 74.0543670654297 319.253051757813 172.017868041992 315.91357421875'))
#  print(process(True,242.0,284.0,245.0,287.0,-1.0,-1.0,True,'163.39762878418 -22.0001831054688 69.4337463378906 -18.7153472900391 74.0543670654297 319.253051757813 172.017868041992 315.91357421875'))
#  print(process(True,242.0,284.0,245.0,287.0,-1.0,-1.0,False,'163.397674560547 -22.0001678466797 -310.49951171875 -157.534454345703 -228.468200683594 429.399017333984 172.017913818359 315.913604736328'))
#  print(process(False,242.0,284.0,245.0,287.0,-1.0,-1.0,False,'163.397674560547 -22.0001678466797 -310.49951171875 -157.534454345703 -228.468200683594 429.399017333984 172.017913818359 315.913604736328'))
