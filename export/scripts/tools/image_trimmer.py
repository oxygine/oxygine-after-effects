try:
  import PIL
except ImportError:
  import os
  print("Trying to Install required module: Pillow\n")
  os.system('python get-pip.py')
  os.system('python -m pip install Pillow')
  pass
  
from PIL import Image

ERROR_OK=0
ERROR_LOAD_IMAGE_FAILED=1
ERROR_IMAGE_NOT_RGBA=2
ERROR_IMAGE_EMPTY=3
ERROR_IMAGE_TRANSPARENT=4
ERROR_SAVE_IMAGE_FAILED=5

error_message = {
    ERROR_OK:"ok", 
    ERROR_LOAD_IMAGE_FAILED:"load image failed", 
    ERROR_IMAGE_NOT_RGBA:"image not rgba", 
    ERROR_IMAGE_EMPTY:"image empty", 
    ERROR_IMAGE_TRANSPARENT:"image transparent", 
    ERROR_SAVE_IMAGE_FAILED:"save image failed",
    }

def get_error_message(rv):
    return error_message[rv]
    pass

def image_load(path,mode='RGBA'):
  try:
    img=Image.open(path)
  except:
    return ERROR_LOAD_IMAGE_FAILED,None
  if img.width==0 or img.height==0:
    return ERROR_IMAGE_EMPTY,img
  if img.mode!='RGBA':
    if mode=='RGBA':
      return ERROR_IMAGE_NOT_RGBA,img
    if mode=='ForceRGBA':
      img=img.convert('RGBA')
  return ERROR_OK,img

def image_save(img,path):
  try:
    img.save(path)
  except:
    return ERROR_SAVE_IMAGE_FAILED
  return ERROR_OK

def premultiply_alpha(img):
  img=img.convert('RGBa')
  img.mode='RGBA'
  return img

def process_zero_alpha_pixels(img):
  neighbours=((-1,0),(+1,0),(0,-1),(0,+1))
  for y in range(1,img.height-1):
    for x in range(1,img.width-1):
      px=img.getpixel((x,y))
      if px[3]==0:
        colors=r=g=b=0
        for px in neighbours:
          px=img.getpixel((x+px[0],y+px[1]))
          if px[3]!=0:
            colors+=1
            r+=px[0]
            g+=px[1]
            b+=px[2]
        if colors>0:
          img.putpixel((x,y),(r//colors,g//colors,b//colors,0))
        else:
          img.putpixel((x,y),(0,0,0,0))
  return img      

def image_trimmer_process(src,trim=False,border=0,premultiplied=False,info_only=False):
  src_w=src.width
  src_h=src.height
  if trim:
    trimmed_rect=src.convert('RGBa').getbbox()
    if trimmed_rect is None:
      trimmed_rect=(0,0,0,0)
  else:
    trimmed_rect=(0,0,src_w,src_h)
  src_x=trimmed_rect[0]
  src_y=trimmed_rect[1]
  copy_w=trimmed_rect[2]-src_x
  copy_h=trimmed_rect[3]-src_y
  dest_w=copy_w+border*2
  dest_h=copy_h+border*2
  info=(src_w,src_h,dest_w,dest_h,src_x-border,src_y-border)
  if info_only:
    return ERROR_OK,None,info
  dest=Image.new('RGBA',(dest_w,dest_h),(0,0,0,0))
  trimmed=src.crop((src_x,src_y,src_x+copy_w,src_y+copy_h))
  dest.paste(trimmed,(border,border,border+copy_w,border+copy_h))
  if premultiplied:
    dest=premultiply_alpha(dest)
  else:
    dest=process_zero_alpha_pixels(dest)
  return ERROR_OK,dest,info

def image_trimmer(src_path,dest_path=None,trim=False,border=0,premultiplied=False):
  rv,src=image_load(src_path)
  if rv==ERROR_IMAGE_NOT_RGBA:
    rv=image_save(src,dest_path)
    info=(src.width, src.height, src.width, src.height, 0.0, 0.0)
    return ERROR_OK,info
    
  if rv!=ERROR_OK:
    return rv,None
    
  rv,dest,info=image_trimmer_process(src,trim,border,premultiplied,dest_path is None)
  
  if rv!=ERROR_OK:
    return rv,info
    
  if dest is not None:
    rv=image_save(dest,dest_path)
    
  return rv,info
