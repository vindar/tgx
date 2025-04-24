#!/usr/bin/env python
# coding: utf-8

# In[ ]:


import sys
from collections import defaultdict
import math
import re


# In[ ]:


def error(msg):
    print("*** ERROR ***"); 
    print(msg)
    print("\n\n")
    input("Press Enter to continue...")
    sys.exit(0)


# In[ ]:


def invertEdge(E):
    return (E[1], E[0])


# In[ ]:


# return the edge after E in the triangle T
def edgeAfter(T , E):    
    E1 = (T[0], T[1])
    E2 = (T[1], T[2])
    E3 = (T[2], T[0])
    if E == E1:
        return E2
    if E == E2:
        return E3
    if E == E3:
        return E1
    error(f"error in edgeAfter({T} , {E}), edge not found")


# In[ ]:


# return edge number i in the triangle
def edge(T, i):
    return (T[0], T[1]) if i == 0 else ((T[1], T[2]) if i ==1 else (T[2], T[0]))


# In[ ]:


# rotate the triangle vertices to start with edge E
def rotateTriangleStartEdge(T,E):
    if (T[0],T[1]) == E:
        return T
    if (T[1],T[2]) == E:
        return (T[1], T[2], T[0])
    if (T[2],T[0]) == E:
        return (T[2], T[0], T[1])
    error(f"error in rotateTriangleStartEdge({T} , {E}), edge not found")    


# In[ ]:


def splitFace(face):
    if (len(face) == 3):        
        return [tuple(face)]
    return [tuple(face[:3])] + splitFace([face[0]] + face[2:])


# In[ ]:


def getint(str, linenb):
    try:
        v = int(str)
        return v
    except:
        error(f"cannot convert [{str}] to int at line {linenb + 1}.gitignore")


# In[ ]:


def getfloat(str, linenb):
    try:
        v = float(str)
        return v
    except:
        error(f"cannot convert [{str}] to int at line {linenb + 1}.gitignore")


# In[ ]:


def parseFaceTag(f, lena, linenb):
    x = f.split("/")
    if len(x) == 0 or len(x) > 3:
        error(f"wrong face index: {f} at line {linenb+1}")
    if len(x) == 1:
        x += ['', '']
    if len(x) == 2:
        x += ['']            
    for i in range(3): 
        if x[i] == '':
            x[i] = -1
        else:
            v = getint(x[i], linenb)        
            if (v == 0):
                error(f"wrong face index: {f} at line {linenb+1} (index 0)")
            x[i] = (v - 1) if v > 0 else lena[i] + v                       
    return tuple(x)


# In[ ]:


def loadObjFile(filename):    
    """
    Load the .obj file
    """

    vertice = []
    texture = []
    normal = []
    obj = []
    tag = []
    
    currentobj = []
    currentname = ""
    currenttagline = 0
    currentnb = 0
    
    print(f"Reading file {filename}.\n")
    try:
        with open(filename, "r") as f:
            lines = list(f)
    except:
        error(f"Cannot open file [{filename}]")
        
    for linenb , line in enumerate(lines): 
        l = line.split()
        if len(l) == 0:
            l = [""] # dummy value
        if l[0] == 'v':
            v = tuple(float(v) for v in l[1:])[:3]
            if len(v) != 3:
                error(f"found wrong vertex [v] (not 3 components) at line {linenb+1}")
            vertice.append(v)
            
        if l[0] == "vt":
            vt = tuple(float(t) for t in l[1:])[:2]
            if len(vt) != 2:
                error(f"found wrong texture coord [vt] (not 2 components) at line {linenb+1}")
            texture.append(vt)
           
        if l[0] == "vn":
            vn = tuple(float(n) for n in l[1:])[:3]
            if len(vn) != 3:
                error(f"found wrong normal [vn] (not 3 components) at line {linenb+1}")
            normal.append(vn)
            
        if l[0] == "o" or l[0] == "g" or l[0] == "usemtl":
            if len(currentobj) > 0:                
                print(f"--- found object {len(obj)+1} starting from tag [{currentname}] at line {currenttagline+1} with {len(currentobj)} triangles.")
                obj.append(currentobj)
                tag.append(currentname)
                currentobj = [] 
                currentname = "" if len(l) == 1 else " ".join(l)
                currenttagline = linenb
            else:
                currenttagline = linenb
                if len(l) >1:
                    currentname += " | " + " ".join(l)
                    
          
        if l[0] == 'f':
            face = [parseFaceTag(x , [len(vertice), len(texture), len(normal)], linenb) for x in l[1:]]
            currentobj += splitFace(face)
            
    if len(currentobj) > 0:                
                print(f"--- found object {len(obj)+1} starting from tag [{currentname}] at line {currenttagline+1} with {len(currentobj)} triangles.")
                obj.append(currentobj)
                tag.append(currentname)              
                
    if (len(obj) == 0):
        error("no faces found in the file !")

    setvi = set()
    setti = set()
    setni = set() 
    for o in obj:
        for T in o:
            for (v,t,n) in T:
                
                if v < 0:
                    error(f"negative vertex index {v} found in face {T}")
                if v >= len(vertice):
                    error(f"vertex index out of bound {v}/{len(vertice)} in face {T}")   
                setvi.add(v)
                
                if (t < -1) or (t >= len(texture)):
                    error(f"texture index out of bound {v}/{len(texture)} in face {T}")  
                setti.add(t)
                
                if (n < -1) or (n >= len(normal)):
                    error(f"normal index out of bound {v}/{len(normal)} in face {T}")  
                setni.add(n)       
           
    if len(setti) > 1 and -1 in setti:
        error(f"Missing texture indexes for some faces (but not all).")  

    if len(setni) > 1 and -1 in setni:
        error(f"Missing normal indexes for some faces (but not all).")  
            
    print(f"\nTOTAL :")
    print(f"- {len(vertice)} vertices [v]")        
    print(f"- {len(texture)} texture coords [vt]")
    print(f"- {len(normal)} normals [vn]\n")

    return vertice, texture, normal, obj, tag


# In[ ]:


def findBoundingBox(vertice):
    xmin , ymin, zmin  = vertice[0]
    xmax , ymax, zmax  = vertice[0]
    for (x,y,z) in vertice:
        xmin = min(xmin,x)
        xmax = max(xmax,x)
        ymin = min(ymin,y)
        ymax = max(ymax,y)
        zmin = min(zmin,z)
        zmax = max(zmax,z)        
    return xmin,xmax,ymin,ymax,zmin,zmax

def recenterAndRescale(vertice):
    pr = 2
    xmin,xmax,ymin,ymax,zmin,zmax = findBoundingBox(vertice)    
    print(f"\nBounding box of the model [{round(xmin,pr)},{round(xmax,pr)}]x[{round(ymin,pr)},{round(ymax,pr)}]x[{round(zmin,pr)},{round(zmax,pr)}]\n")
    ans = input("Do you want to translate/resize the model to fit in [-1,1]x[-1,1]*[-1,1] (y/N) ?")
    if len(ans) > 0 and (ans.lower())[0] == "y":        
        cx = (xmin + xmax)/2
        cy = (ymin + ymax)/2
        cz = (zmin + zmax)/2
        mx = (xmax -xmin)/2
        my = (ymax -ymin)/2
        mz = (zmax -zmin)/2
        s = max(mx,my,mz)
        for i in range(len(vertice)):            
            x,y,z  = vertice[i]
            vertice[i] = ((x - cx)/s , (y-cy)/s, (z-cz)/s)                    
        xmin,xmax,ymin,ymax,zmin,zmax = findBoundingBox(vertice)        
    xmin = round(xmin,pr)
    xmax = round(xmax,pr)
    ymin = round(ymin,pr)
    ymax = round(ymax,pr)
    zmin = round(zmin,pr)
    zmax = round(zmax,pr)
    if len(ans) > 0 and (ans.lower())[0] == "y":        
        print(f"\n- new bounding box: [{xmin},{xmax}]x[{ymin},{ymax}]x[{zmin},{zmax}]")
    return vertice , (xmin,xmax,ymin,ymax,zmin,zmax)


# In[ ]:


def dist(A, B):
    return math.sqrt((A[0]-B[0])*(A[0]-B[0]) + (A[1]-B[1])*(A[1]-B[1]) + (A[2]-B[2])*(A[2]-B[2]))


# In[ ]:


def boundingBoxes(vertice, R):
    res = []
    for O in R:
        #iterate over the objects
        vert = []
        for C in O:
            for _,T in C:
                for u, _, _ in T:
                    vert.append(vertice[u])  
        xmin,xmax,ymin,ymax,zmin,zmax = findBoundingBox(vert)
        res.append((xmin,xmax,ymin,ymax,zmin,zmax))
    return res


# In[ ]:


def addVec(VA,VB):
    return (VA[0]+VB[0], VA[1]+VB[1], VA[2]+VB[2])

def normVec(V):
    norm = math.sqrt(V[0]*V[0] + V[1]*V[1] + V[2]*V[2])
    return V if (norm == 0) else (V[0]/norm, V[1]/norm, V[2]/norm)

def crossProduct(VA,VB):
    return ( VA[1]*VB[2] -  VA[2]*VB[1], VA[2]*VB[0] -  VA[0]*VB[2], VA[0]*VB[1] -  VA[1]*VB[0])

def Vec(VA, VB):
    return (VB[0] - VA[0], VB[1] - VA[1], VB[2] - VA[2])


# In[ ]:


def fixNormals(vertice, normal, obj):
    # normal = []  # uncomment for forcing normal regeneration
    if len(normal) == 0:        
        print("\nNo normal found in .obj file.", end="")
        ans = input("Do you want to compute smooth normals (y/n) ?")       
        if len(ans) > 0 and ((ans.lower())[0] == "y" or (ans.lower())[0] == "o"):        
            # create the normals
            normal = [(0.0, 0.0, 0.0)] * len(vertice)            
            for o in obj:
                for T0,T1,T2 in o:   
                    i0 = T0[0]
                    i1 = T1[0]
                    i2 = T2[0]
                    V0 = Vec(vertice[i1] , vertice[i0])
                    V1 = Vec(vertice[i2] , vertice[i1])
                    V2 = Vec(vertice[i0] , vertice[i2])
                    N0 = crossProduct(V2, V0)
                    N1 = crossProduct(V0, V1)
                    N2 = crossProduct(V1, V2)
                    N = addVec(addVec(N0, N1),N2);
                    normal[i0] = addVec(normal[i0] , N)
                    normal[i1] = addVec(normal[i1] , N)
                    normal[i2] = addVec(normal[i2] , N)    
                    
            # insert normal indexes in object
            for o in obj:
                for i in range(len(o)):                        
                    T1, T2, T3 = o[i]
                    o[i] = ((T1[0], T1[1], T1[0]),(T2[0], T2[1], T2[0]),(T3[0], T3[1], T3[0]))
                
    # normalize the normals
    for i in range(len(normal)):
        normal[i] = normVec(normal[i])        
    return normal        


# In[ ]:


def reorderObjectTriangles(obj):
    availT = obj[:] # list of triangles. Set to none once used. 
    dicedge = defaultdict(lambda: []) # mapping from edge to triangles indexes
    firstTavail = 0 # first triangle available in availT
    nbTavail = len(obj) # number of triangle available in availT
    for i in range(len(obj)):                
        dicedge[edge(obj[i],0)].append(i)
        dicedge[edge(obj[i],1)].append(i)
        dicedge[edge(obj[i],2)].append(i)   
    
    # this method return a triangle index a given edge and mark it as used
    # or return none if no triangle available. 
    def findTriangleWithEdge(E):
        nonlocal availT
        nonlocal dicedge
        nonlocal firstTavail
        nonlocal nbTavail
        L = dicedge[E]
        while(len(L) > 0):
            i = L.pop()
            if availT[i] != None:
                TT = availT[i]
                availT[i] = None
                nbTavail -= 1
                return TT
        return None

    # return the next available triangle or none if all triangles have been used
    def getNextTriangle():
        nonlocal availT
        nonlocal firstTavail
        nonlocal nbTavail        
        if nbTavail == 0:
            return None
        nbTavail -= 1
        while(availT[firstTavail] == None):
            firstTavail += 1            
        T = availT[firstTavail]
        availT[firstTavail] = None              
        return T
    R = []    
    while True:        
        # start a new chain 
        T = getNextTriangle()
        if (T == None):
            return R        
        C = [ (None, T) ]        
        E = edge(T, 2) 
        
        while True:
            MAXCHAINLEN = 65535
            if len(C) == MAXCHAINLEN:                
                break                 
            E = edgeAfter(T , E)            
            T2 = findTriangleWithEdge(invertEdge(E))
            if T2 == None:
                E = edgeAfter(T , E)
                T2 = findTriangleWithEdge(invertEdge(E))
                if T2 == None:
                    if len(C) > 1:                        
                        break 
                    else:                    
                        E = edgeAfter(T , E)
                        T2 = findTriangleWithEdge(invertEdge(E))
                        if T2 == None:                            
                            break 
            # got the next link in T2 with shared edge E
            pT = C[-1][1]
            n = None
            if (E == (pT[0], pT[1])):
                if len(C) > 1:
                    error("wrong next edge...")
                n = 2
            if (E == (pT[2], pT[0])):
                n = 0
            if (E == (pT[1], pT[2])):
                n = 1
            if n == None:
                    error("edge not found...")        
            E = invertEdge(E)            
            T = T2
            C.append((n, rotateTriangleStartEdge(T,E)))
            
        if len(C)>1:
            if C[1][0] == 2:
                C[0] = (None , ( C[0][1][1], C[0][1][2], C[0][1][0] )) 
                C[1] = (0, C[1][1])
        R.append(C)


# In[ ]:


def reorderVNTarrays(vertice, texture, normal, R):
    
    def orderByFirstUse(ar, R, index):
        vi = []
        setv = set()
        for O in R:
            for C in O:
                _ , T = C[0]
                for i in range(3):
                    if T[i][index] not in setv:
                        setv.add(T[i][index])
                        vi.append(T[i][index])                
                for (_ , T) in C[1:]:
                    if T[2][index] not in setv:
                        setv.add(T[2][index])
                        vi.append(T[2][index])  
        if ar == []:
            if vi != [-1]:
                error("reorderVNTarrays, vi should be [-1]")                
            return [] # nothing to do
        else:
            if (len(vi) > len(ar)):
                error("reorderVNTarrays, wrong size for vi")
            if (len(vi) < len(ar)):
                for j in range(len(ar)):
                    if j not in setv:
                        vi.append(j)                    
            setc = set()
            for i in vi:
                if i < 0:
                    error("reorderVNTarrays, negative index")
                if i >= len(ar):
                    error("reorderVNTarrays, index too large")
                if i in setc:
                    error("reorderVNTarrays, index found twice")
                setc.add(i)
            if len(setc) != len(ar):
                    error("reorderVNTarrays, hum.. impossible !")
        # ok, vi defines a permutation
        return vi
    
    def invperm(vi):
        iv = vi[:]
        for i, a in enumerate(vi):
            iv[a] = i
        return iv
        
    def chI(T, index, iv):
        if index == 0: 
            return (iv[T[0]], T[1], T[2])
        if index == 1:
            return (T[0], iv[T[1]], T[2])
        if index == 2:
            return (T[0], T[1], iv[T[2]])
        error("chTriangleIndex() wrong index")
        
    def reorderArray(vi, ar, R, index): 
        if vi == []:
            return
        # permute ar
        newar = ar[:]
        for i , v in enumerate(vi):
            newar[i] = ar[v] 
        for i in range(len(ar)):
            ar[i] = newar[i]            
        # replace each index j by iv[j] in R
        iv = invperm(vi)                
        for O in R:            
            for C in O:
                for k in range(len(C)):                
                    C[k] = (C[k][0] , (chI(C[k][1][0], index, iv), chI(C[k][1][1], index, iv), chI(C[k][1][2], index, iv)))                
        return 
    
    def trim(ar, R, index, arname):
        next = 0
        for O in R:
            for C in O:
                _ , T = C[0]
                for i in range(3):
                    z = T[i][index] 
                    if z > next:
                        error("array reordering was incorrect (1)!")
                    if z == next:
                         next += 1                
                for (_ , T) in C[1:]:
                    z = T[2][index] 
                    if z > next:
                        error("array reordering was incorrect (2)!")
                    if z == next:
                         next += 1
        if next > len(ar):
            error("index out of bounds (should not be possible)")
        if next < len(ar):
            print(f"\n- deleting {len(ar) - next} unused entry from the {arname} array. {next} entries remaining.")
            del ar[next:]            
                                
    vi_v = orderByFirstUse(vertice, R, 0)
    reorderArray(vi_v, vertice, R, 0)
    trim(vertice, R, 0, "vertex")

    vi_t = orderByFirstUse(texture, R, 1)
    reorderArray(vi_t, texture, R, 1)                        
    trim(texture, R, 1, "texture")

    vi_n = orderByFirstUse(normal, R, 2)
    reorderArray(vi_n, normal, R, 2)
    trim(normal, R, 2, "normal")
    return


# In[ ]:


def reverseTriangleDirection(obj,normal):
    """ Reverse the direction of all the triangle in all the objects"""
    for o in obj:
        for i in range(len(o)):
            T = o[i]
            o[i] = (T[0],T[2],T[1])
    for i in range(len(normal)):
        N = normal[i]
        normal[i] = (-N[0], -N[1], -N[2])


# In[ ]:


def arraytoString(array):
    return "{\n" + ("\n".join( [ "{" + ",".join([str(i) for i in u]) + "}," for u in array] )).rstrip(",") + "\n};\n"


# In[ ]:


def savemodel(vertice, texture, normal, R, modelname, texturenames, tag, color, lightning, BB, BBS):    
    
    NAMESPACE = "tgx" 
    
    MAXVERTICE = 32767
    MAXNORMAL  = 65535
    MAXTEXTURE = 65535
    
    if (len(vertice) > MAXVERTICE):
        error(f"Model has too many vertices: {len(vertice)} found (max allowed {MAXVERTICE})")    
    if (len(texture) > MAXTEXTURE):
        error(f"Model has too many texture coords: {len(texture)} found (max allowed {MAXTEXTURE})")
    if (len(normal) > MAXNORMAL):
        error(f"Model has too many normals: {len(normal)} found (max allowed {MAXNORMAL})")
        
    def nbT(O):
        tot = 0
        for C in O:            
            tot += len(C)
        return tot
    
    totKB = len(vertice)*12 + len(normal)*12 + len(texture)*8 + len(R)*68
    for O in R:            
        for C in O: 
            elem = 1
            if len(texture) > 0:
                elem +=1
            if len(normal) > 0:
                elem +=1
            totKB += 2*(1 + (2 + len(C))*elem)   
    totKB //= 1024
    
    with open(modelname + ".h", "w") as f:
        f.write(f'// 3D model [{modelname}]\n')
        f.write(f'//\n')
        f.write(f'// - vertices   : {len(vertice)}\n')
        f.write(f'// - textures   : {len(texture)}\n')
        f.write(f'// - normals    : {len(normal)}\n')    
        
        nttot = 0
        for O in R:
            nttot += nbT(O)
        f.write(f'// - triangles  : {nttot}\n')                    
        f.write(f'//\n')        
        f.write(f'// - memory size: {totKB}kb\n')    
        f.write(f'//\n')    
        f.write(f'// - model bounding box: [{BB[0]},{BB[1]}]x[{BB[2]},{BB[3]}]x[{BB[4]},{BB[5]}]\n')    
        f.write(f'//\n')    
        
        if len(R) == 1:        
            f.write(f'// object [{modelname}] (tagged [{tag[0]}]) with {nbT(R[0])} triangles ({len(R[0])} chains)\n')
        else:
            for i, O in enumerate(R):
                f.write(f'// object [{modelname}_{i+1}] (tagged [{tag[i]}]) with {nbT(O)} triangles ({len(O)} chains)\n')
        f.write('\n#pragma once\n')
        f.write('\n#include <tgx.h>\n')
    
        for i, tname in enumerate(texturenames):
            if (tname != None):
                if len(R) == 1:                    
                    f.write(f'\n#include "{tname}_texture.h" // texture for object [{modelname}]\n')
                else:
                    f.write(f'\n#include "{tname}_texture.h" // texture for object [{modelname}_{i+1}]\n')
                                
        name_vertice = modelname + "_vert_array"
        name_texture = modelname + "_tex_array" if len(texture) > 0 else "nullptr"
        name_normal = modelname + "_norm_array" if len(normal) > 0 else "nullptr"
 
        f.write(f"\n\n// vertex array: {(len(vertice)*12)//1024}kb.\n")
        f.write(f"const {NAMESPACE}::fVec3 {name_vertice}[{len(vertice)}] PROGMEM = ")
        f.write(arraytoString(vertice))
    
        if len(texture) > 0:
            f.write(f"\n\n// texture array: {(len(texture)*8)//1024}kb.\n")
            f.write(f"const {NAMESPACE}::fVec2 {name_texture}[{len(texture)}] PROGMEM = ")
            f.write(arraytoString(texture))
            
        if len(normal) > 0:
            f.write(f"\n\n// normal array: {(len(normal)*12)//1024}kb.\n")
            f.write(f"const {NAMESPACE}::fVec3 {name_normal}[{len(normal)}] PROGMEM = ")
            f.write(arraytoString(normal))
            
        f.write("\n");
            
        elem = 1
        if len(texture) > 0:
            elem +=1
        if len(normal) > 0:
            elem +=1
           
        nbw = 0
        def writeelem(el):
                nonlocal f
                nonlocal nbw
                nbw+=1
                f.write(f"{el[0]},")
                if el[1] >= 0:
                    f.write(f"{el[1]},")
                    nbw+=1
                if el[2] >= 0:
                    f.write(f"{el[2]},")
                    nbw+=1                    
                f.write(" ")
            
        for mnb, O in enumerate(R):            
            name = modelname
            if len(R) > 1:
                name += "_" + str(mnb + 1)
            name += "_face"
            nbw = 0
            tl = 1            
            for C in O:                                
                tl +=  1 + (2 + len(C))*elem   
            f.write(f"\n// face array: {(tl*2)//1024}kb.\n")
            f.write(f"const uint16_t {name}[{tl}] PROGMEM = {{\n")
            for nc, C in enumerate(O):                
                f.write(f"{len(C)}, // chain {nc}\n")
                nbw+=1
                writeelem(C[0][1][0])
                writeelem(C[0][1][1])
                writeelem(C[0][1][2])
                f.write("\n")
                
                nl = 0
                for L in C[1:]:
                    writeelem( (L[1][2][0] + (32768*L[0]) , L[1][2][1] , L[1][2][2]) )
                    nl +=1
                    if (nl == 16):
                        f.write("\n")
                        nl = 0  
                if nl!=0:
                    f.write("\n")
                    
            f.write("\n 0};\n\n")
            nbw +=1
            if nbw != tl:
                error("savemodel() wrong count !")
        
                        
        for bnm in range(len(R)):
            
            mnb = len(R) - bnm - 1; 
            O = R[mnb]
            
            tl = 1            
            for C in O:                                
                tl +=  1 + (2 + len(C))*elem 
            
            name = modelname
            nextname = "nullptr"
            if len(R) > 1:                
                if bnm > 0:
                    nextname = "&" + name + "_" + str(mnb + 2)
                name += "_" + str(mnb + 1)
                
            name_triangle = name + "_face"            
            tname = texturenames[mnb]
            if (tname == None):
                tname = "nullptr"
            else:
                tname = "&" + tname + "_texture"
            
            f.write(f"""
// mesh info for object {name} (with tag [{tag[mnb]}])
const {NAMESPACE}::Mesh3D<{NAMESPACE}::RGB565> {name} PROGMEM = 
    {{
    1, // version/id
    
    {len(vertice)}, // number of vertices
    {len(texture)}, // number of texture coords
    {len(normal)}, // number of normal vectors
    {nbT(O)}, // number of triangles
    {tl}, // size of the face array. 

    {name_vertice}, // array of vertices
    {name_texture}, // array of texture coords
    {name_normal}, // array of normal vectors        
    {name_triangle}, // array of face vertex indexes   
    
    {tname}, // pointer to texture image 
    
    {{ {color[mnb][0]}f , {color[mnb][1]}f, {color[mnb][2]}f }}, // default color
    
    {lightning[mnb][0]}f, // ambiant light strength 
    {lightning[mnb][1]}f, // diffuse light strength
    {lightning[mnb][2]}f, // specular light strength
    {lightning[mnb][3]}, // specular exponent
    
    {nextname}, // next mesh to draw after this one    
    
    {{ // mesh bounding box
    {BBS[mnb][0]}f, {BBS[mnb][1]}f, 
    {BBS[mnb][2]}f, {BBS[mnb][3]}f, 
    {BBS[mnb][4]}f, {BBS[mnb][5]}f
    }},
    
    "{modelname}" // model name    
    }};
    
""")                                   
        f.write(f"""                
/** end of {modelname}.h */
    
    
    """)
      


# In[ ]:


def getColorLightning(use_default_cl, nb):
    
    DEFAULT_COLOR = (0.75,0.75,0.75) # silver
    DEFAULT_LIGHTNING = (0.1, 0.7, 0.6, 32) # glossy aspect (but not too much).  
    
    if use_default_cl:
        return DEFAULT_COLOR, DEFAULT_LIGHTNING

    coltxt = input(f"- color for object {nb}. [ENTER] for default: {DEFAULT_COLOR}")
    try:
        col = [ float(l) for l in re.split(r',|\(|\)| ', coltxt) if len(l)>0] 
    except:
        col = []
    if (len(col) != 3):
        if len(coltxt) > 0:
            print("Incorrect color, using default values.")
        col = DEFAULT_COLOR

    lighttxt = input(f"- lightning for object {nb}. [ENTER] for default: {DEFAULT_LIGHTNING}")
    try:
        light = [ float(l) for l in re.split(r',|\(|\)| ', lighttxt) if len(l)>0] 
        light[3] = int(light[3])
    except:
        light = []    
    if (len(light) != 4):
        if len(lighttxt) > 0:
            print("Incorrect lightning, using default values.")
        light = DEFAULT_LIGHTNING
    
    return col,light    


# In[ ]:


print("""
*** Python script to generate a .h from a .obj 3D model ***

""")

# get the file name
filename = input("name of the .obj file ? ")
if not filename.lower().endswith(".obj"):
    filename += '.obj'
print()

#load the file
vertice, texture, normal, obj, tag = loadObjFile(filename)

#invert triangle direction (only for non-standard oriented meshes)
#reverseTriangleDirection(obj,normal)

# create normals if needed and normalize them.
normal = fixNormals(vertice, normal, obj)

for i,N in enumerate(normal):
    n2 = N[0]*N[0] + N[1]*N[1] + N[2]*N[2]
    if (n2 < 0.9999) or (n2 > 1.0001):
        print(f"error normal {N} (index {i}) with norm2 ={n2}")

# reorder triangles to maximize chaining
print("")
R = []
for i,x in enumerate(obj):
    print(f"Reordering object {i+1} with {len(x)} triangles... ", end="")
    U = reorderObjectTriangles(x)
    print(f"Done. {len(U)} chains created.")            
    R.append(U)

# renumber vertices/texture/normal optimize cache acess
print("\nrenumbering vertices/texture/normals...", end="")
reorderVNTarrays(vertice, texture, normal, R)
print("Done.\n")

# recenter and rescale the model if needed
vertice, BB = recenterAndRescale(vertice)

#compute the bounding sphere for each object
BBS = boundingBoxes(vertice, R)

# create the header file
modelname = input("\n\nname of this model ? ")


ans = input("\nuse default color/lightning parameters (Y/n) ?")
use_default_cl = True if len(ans) == 0 or (ans.lower())[0] == "y" else False

# get the texture names
color = [None] * len(obj)
lightning = [None] * len(obj)
texturenames = [None] * len(obj)
for i in range(len(obj)):
    if (len(texture)>0):    
        tname = input(f"\n\n- name of texture for object {i+1} [{tag[i]}] (press [ENTER] if none) ? ")        
        if (len(tname) > 0):
            texturenames[i] = tname            
    color[i] , lightning[i] = getColorLightning(use_default_cl, i+1)
        
savemodel(vertice, texture, normal, R,
          modelname, texturenames, tag, color, lightning, BB, BBS)



print(f"\n*** conversion complete: model saved in [{modelname + '.h'}] ***\n\n")


# In[ ]:




