#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import PIL
from PIL import Image
import numpy as np
import sys


# In[ ]:


# display an error message and quit
def error(msg):
    print("*** ERROR ***"); 
    print(msg)
    print("\n\n")
    input("Press Enter to continue...")
    sys.exit(0)


# In[ ]:


# add the alpha channel, the image must not have it already
def addAlphaChannel(ar):
    if ar.shape[2] != 3:
        error(f"*** Unsupported image type (this image has {nbchannels} color channels when it should be 3 or 4) ***")
    B = np.full((ar.shape[0], ar.shape[1] ,1),255)
    ar = np.concatenate((ar,B),axis=2)
    return ar

# Return the minimum alpha value of all pixels in the image
def minAlpha(ar):
    ma = 255
    for i,j in np.ndindex(ar.shape[0:2]):
        ma = min(ar[i,j,3], ma)
    return ma

# convert the image array from plain alpha to premultiply alpha
# the image must have 4 channels
def premultiply(ar):
    print("Converting to pre-multiplied alpha format.")
    for i,j in np.ndindex(ar.shape[0:2]):
        alpha = float(ar[i,j,3]) / 255.0      
        ar[i,j] = [int(ar[i,j,0]*alpha) ,int(ar[i,j,1]*alpha), int(ar[i,j,2]*alpha),ar[i,j,3]]
    return ar

# convert the first 3 channels to 5/6/5 bits
def convertRGB565(ar):
    for i,j in np.ndindex(ar.shape[0:2]):
        R = int((ar[i,j,0] + 4)*31.0/255.0)
        G = int((ar[i,j,1] + 2)*63.0/255.0)
        B = int((ar[i,j,2] + 4)*31.0/255.0)
        ar[i,j] = [R,G, B , ar[i,j,3]]
    return ar
        
# Set a given color to all images with transparency strictly below a given threshold
# alpha_threshold in [1,255]
# rgb_transparent the transparent color to use
def setTransparentColor(alpha_threshold , rgb_transparent, alt, ar):
    m = 0
    for i,j in np.ndindex(ar.shape[0:2]):        
        if tuple(ar[i,j,0:3]) == tuple(rgb_transparent):
            ar[i,j,0:3] = alt        
        if ar[i,j,3] < alpha_threshold:
            ar[i,j,0:3] = rgb_transparent
            m += 1        
    return ar , m
        


# In[ ]:


def RGB565(col):
    return f"C({col[0]},{col[1]},{col[2]})"

def RGB24(col):
    return f"C({col[0]},{col[1]},{col[2]})"

def RGB32(col):
    return f"C({col[0]},{col[1]},{col[2]},{col[3]})"

def RGBf(col):
    return f"C({float(col[0])/255.0}f,{float(col[1])/255.0}f,{float(col[2])/255.0}f)"

def color(col , color_type):
    if color_type == "RGB565":
        return RGB565(col);
    elif  color_type == "RGB24":
        return RGB24(col);
    elif  color_type == "RGB32":
        return RGB32(col);
    elif  color_type == "RGBf":
        return RGBf(col);
    else:
        raise(f"Error, unsupported color type [type]")
        
def colorTypeSize(color_type):
    if color_type == "RGB565":
        return 2;
    elif color_type == "RGB24":
        return 3;
    elif color_type == "RGB32":
        return 4;
    elif color_type == "RGBf":
        return 12;
    else:
        raise(f"Error, unsupported color type [type]")

def defineC(color_type):
    if color_type == "RGB565":
        return "C(R,G,B) tgx::RGB565(R,G,B)";
    elif color_type == "RGB24":
        return "C(R,G,B) tgx::RGB24(R,G,B)";
    elif color_type == "RGB32":
        return "C(R,G,B,A) tgx::RGB32(R,G,B,A)";
    elif color_type == "RGBf":
        return "C(R,G,B) tgx::RGBf(R,G,B)";
    else:
        raise(f"Error, unsupported color type [type]")
    
        
def colorName(color_type):
    color_type = color_type.lower()
    if (color_type == "rgb565") or (color_type.lower() == "rgb16"):
        return "RGB565";
    elif  color_type == "rgb24":
        return "RGB24";
    elif  color_type == "rgb32":
        return "RGB32";
    elif  color_type == "rgbf":
        return "RGBf";
    else:
        return ""
        


# In[ ]:


def createCPP(ar, color_type, name, tc):
    
    width = ar.shape[0]
    height = ar.shape[1]
    color_size  = colorTypeSize(color_type)
    
    with open(name + ".cpp", "w") as f:           
        f.write('//\n');
        f.write(f'// Image: {name}\n');
        f.write(f'// dimension: {width}x{height}\n');
        f.write(f'// Size: {int(round(width*height*color_size / 1024))}kb\n');        
        f.write(f'//\n\n');
        f.write(f'#include "{name}.h"\n\n');
        f.write(f'#define {defineC(color_type)}\n\n');
        f.write(f'// image data\n');
        f.write(f'static const tgx::{color_type} {name}_data[{width}*{height}] PROGMEM = {{\n');
        i = 0
        for y in range(height):
            for x in range(width):
                f.write(color(ar[x, y], color_type))
                if y*x != ((width-1)*(height-1)):
                    f.write(", ")
                i += 1
                if i == 16:
                    f.write("\n")
                    i = 0;
        f.write('};\n\n')
        f.write(f'// image object\n');        
        f.write(f'const tgx::Image<tgx::{color_type}> {name}({name}_data, {width}, {height});\n\n');             
        f.write(f'#undef C\n')
        f.write(f'// end of file {name}.cpp\n\n')
    
    with open(name + ".h", "w") as f:           
        f.write('//\n');
        f.write(f'// Image: {name}\n');
        f.write(f'// dimension: {width}x{height}\n');
        f.write(f'// Size: {int(round(width*height*color_size / 1024))}kb\n');        
        f.write(f'//\n\n');
        f.write(f'#pragma once\n\n');        
        f.write(f'#include <tgx.h>\n\n'); 
        if (tc):            
            f.write(f'// the image uses this color as transparent.\n')
            f.write(f'static const tgx::{color_type} {name}_transparent_color({tc[0]},{tc[1]},{tc[2]});\n\n'); 

        f.write(f'// the image object\n')
        f.write(f'extern const tgx::Image<tgx::{color_type}> {name};\n\n')                
        f.write(f'// end of file {name}.h\n\n')
    
    print(f"file [{name}.h] and [{name}.cpp] created.\n\n")
    
    


# In[ ]:


# get the file name
while True:
    try:
        filename = input("Image filename ? ")
        image = Image.open(filename)        
        break
    except:
        print(f"\n*** cannot open image file [{filename}]... ***\n\n")

arim = np.asarray(image,dtype=np.int)        
if len(arim.shape) != 3:
    error("Unsupported image type !")

arim = np.copy(arim.transpose((1, 0, 2))) # reorder width/height
width = arim.shape[0]
height = arim.shape[1]
nbchannels = arim.shape[2]
print(f"\nImage size : {width}x{height} with {nbchannels} color channels\n")

# deal with transparency in the source image
if nbchannels == 4:
    minalpha = minAlpha(arim)
    if (minalpha == 255):
        print("- this image has an alpha channel but all pixels are fully opaque.\n")
    else:   
        print("- this image uses transparency.\n")
        ans = input("Do you want to convert colors to pre-multiplied alpha (Y/n)?")
        if not (ans.lower() =='n'):
            arim = premultiply(arim)
else:
    arim = addAlphaChannel(arim)
    minalpha = 255
    
# Choose output color type
while True:    
    color_type = input("\nChoose the image color type: RGB565, RGB24, RGB32 or RGBf ? ")
    color_type = colorName(color_type)
    if (color_type != ""):
        break

# change color value range when using 5/6/5 bits images
if (color_type == "RGB565"):
    arim  = convertRGB565(arim)        
     
# choose transparent color (if needed)
use_tc = False
tc = [1,1,1]# transparent color
alt_tc = [1,0,1] # alternate color for pixel with inital color tc

if (minalpha < 255):
    if (color_type == "RGB565") or (color_type == "RGB24"):
        ans = input("The output color format does not contain an alpha channel\nDo you want to specify an alpha threshold to use a transparent color?").lower()
        if (ans == 'y'):
            use_tc = True
            alpha_threshold = int(input("- alpha threshold (in [1,255]) ? "))
            print(f"- color (R={tc[0]},G={tc[1]},B={tc[2]}) is chosen as the transparent color")
            print(f"  and replaces all pixels with alpha < {alpha_threshold}...")
            arim , m = setTransparentColor(alpha_threshold, tc, alt_tc, arim)
            print(f"  Found {m} transparent pixels.")

filename = input("Name of the image ? ")
createCPP(arim, color_type, filename, tc if use_tc else None)
            


# In[ ]:




