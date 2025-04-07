#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import PIL
from PIL import Image
import numpy as np
import sys


# In[ ]:


def RGB565(rgb):
    r = int(rgb[0])
    g = int(rgb[1])
    b = int(rgb[2])

    R = int((r + 4) * 31.0 / 255.0) << 11
    G = int((g + 2) * 63.0 / 255.0) << 5
    B = int((b + 4) * 31.0 / 255.0)
    return hex(R + G + B)


# In[ ]:


def createTexture(im, name):
    NAMESPACE = "tgx"
    im = im.convert("RGB")  # Ensure RGB format
    ar = np.asarray(im)
    with open(name + "_texture.h", "w") as f:   
        f.write('//\n')
        f.write(f'// texture [{name}]\n')
        f.write('//\n')
        f.write('#pragma once\n\n')
        f.write('#include <tgx.h>\n\n')
        f.write(f'const uint16_t {name}_texture_data[{im.width}*{im.height}] PROGMEM = {{\n')
        i = 0
        for y in range(im.height):
            for x in range(im.width):
                rgb = ar[im.height - 1 - y, x]
                f.write(RGB565(rgb))
                if y * x != ((im.width - 1) * (im.height - 1)):
                    f.write(", ")
                i += 1
                if i == 16:
                    f.write("\n")
                    i = 0
        f.write('};\n\n')
        f.write(f'const {NAMESPACE}::Image<{NAMESPACE}::RGB565> {name}_texture((void*){name}_texture_data, {im.width}, {im.height});')                    
        f.write(f'\n\n/** end of file {name}_texture.h */\n\n')
    print(f"\nTexture file [{name}_texture.h] created.\n\n")


# In[ ]:


def testPow2(n):
    return (n & (n-1) == 0) and n != 0


# In[ ]:


print("""
*** Python script to generate a .h texture file from an image ***

""")

# get the file name
while True:
    try:
        filename = input("Texture image filename ? ")
        print()
        image = Image.open(filename)
        break
    except:
        print(f"\n*** cannot open image file [{filename}]... ***\n\n")
    
print(f"\nImage [{filename}] of size : {image.width}x{image.height}\n")

w = input(f"Texture width (enter to keep {image.width}) ? ")
if len(w) == 0:
    w = image.width
else:
    w = int(w)    
    
if not testPow2(w):
    print("!!!! the image width is not a power of two: image cannot be used for texture mapping  (but it can still be used as an image)!!!!")
    
h = input(f"Texture height (enter to keep {image.height}) ? ")
if len(h) == 0:
    h = image.height
else:
    h = int(h)    

if not testPow2(h):
    print("!!!! the image height is not a power of two: image cannot be used for texture mapping (but it can still be used as an image) !!!!")

if (w != image.width) or (h != image.height):
    image = image.resize((w,h),Image.BICUBIC)


name = input(f"Name of the texture ? ")

createTexture(image, name)


# In[ ]:




