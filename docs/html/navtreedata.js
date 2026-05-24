/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "TGX", "index.html", [
    [ "About the library", "index.html#about", null ],
    [ "Getting started", "index.html#getting_started", null ],
    [ "References", "index.html#References", null ],
    [ "Installation.", "md_docs_intro_install.html", null ],
    [ "Basic concepts.", "md_docs_intro_basic.html", [
      [ "Color types", "md_docs_intro_basic.html#autotoc_md2", [
        [ "Blending and opacity parameter.", "md_docs_intro_basic.html#autotoc_md3", null ]
      ] ],
      [ "Image class, memory layout and sub-images", "md_docs_intro_basic.html#autotoc_md5", [
        [ "Creating an image", "md_docs_intro_basic.html#autotoc_md6", null ],
        [ "Memory layout and stride", "md_docs_intro_basic.html#autotoc_md7", null ],
        [ "Sub-images", "md_docs_intro_basic.html#autotoc_md8", null ]
      ] ],
      [ "Coordinates, vectors and boxes", "md_docs_intro_basic.html#autotoc_md10", [
        [ "Vector and box classes", "md_docs_intro_basic.html#autotoc_md11", null ],
        [ "Common beginner mistakes", "md_docs_intro_basic.html#autotoc_md12", null ]
      ] ],
      [ "Storing images in C++ source files", "md_docs_intro_basic.html#autotoc_md14", null ],
      [ "Copying/converting images to different color types", "md_docs_intro_basic.html#subsec_basic_copy_convert", null ],
      [ "Where to go next", "md_docs_intro_basic.html#autotoc_md15", null ]
    ] ],
    [ "Using the 2D API.", "md_docs_intro_api_2_d.html", [
      [ "Overview of the 2D API", "md_docs_intro_api_2_d.html#sec_2D_overview", null ],
      [ "Naming conventions", "md_docs_intro_api_2_d.html#sec_2D_naming", null ],
      [ "Performance and clipping", "md_docs_intro_api_2_d.html#sec_2D_performance", null ],
      [ "2D drawing methods", "md_docs_intro_api_2_d.html#sec_2Dprimitives", [
        [ "Filling the screen", "md_docs_intro_api_2_d.html#subsec_filling", null ],
        [ "Reading/writing pixels", "md_docs_intro_api_2_d.html#subsec_rwpixels", null ],
        [ "Flood-filling a region", "md_docs_intro_api_2_d.html#subsec_floodfill", null ],
        [ "Blitting sprites", "md_docs_intro_api_2_d.html#subsec_blitting", null ],
        [ "Drawing horizontal and vertical lines", "md_docs_intro_api_2_d.html#subsec_vhlines", null ],
        [ "Drawing lines", "md_docs_intro_api_2_d.html#subsec_lines", null ],
        [ "Drawing rectangles", "md_docs_intro_api_2_d.html#subsec_rect", null ],
        [ "Drawing rounded rectangles", "md_docs_intro_api_2_d.html#subsec_roundrect", null ],
        [ "Drawing triangles", "md_docs_intro_api_2_d.html#subsec_triangles", null ],
        [ "Drawing triangles (advanced)", "md_docs_intro_api_2_d.html#subsec_triangles_advanced", null ],
        [ "Drawing quads", "md_docs_intro_api_2_d.html#subsec_quads", null ],
        [ "Drawing quads (advanced)", "md_docs_intro_api_2_d.html#subsec_quads_advanced", null ],
        [ "Drawing polylines", "md_docs_intro_api_2_d.html#subsec_polylines", null ],
        [ "Drawing polygons", "md_docs_intro_api_2_d.html#subsec_polygons", null ],
        [ "Drawing circles", "md_docs_intro_api_2_d.html#subsec_circles", null ],
        [ "Drawing ellipses", "md_docs_intro_api_2_d.html#subsec_ellipses", null ],
        [ "Drawing circle arcs and pies", "md_docs_intro_api_2_d.html#subsec_arcpies", null ],
        [ "Drawing Bezier curves", "md_docs_intro_api_2_d.html#subsec_bezier", null ],
        [ "Drawing splines", "md_docs_intro_api_2_d.html#subsec_splines", null ],
        [ "Drawing text", "md_docs_intro_api_2_d.html#subsec_text", null ]
      ] ],
      [ "TGX extensions via external libraries", "md_docs_intro_api_2_d.html#sec_extensions", [
        [ "Drawing text with TrueType fonts", "md_docs_intro_api_2_d.html#subsec_openfontrender", null ],
        [ "Drawing PNG images", "md_docs_intro_api_2_d.html#subsec_PNGdec", null ],
        [ "Drawing JPEG images", "md_docs_intro_api_2_d.html#subsec_JPEGDEC", null ],
        [ "Drawing GIF images", "md_docs_intro_api_2_d.html#subsec_AnimatedGIF", null ]
      ] ],
      [ "Where to go next", "md_docs_intro_api_2_d.html#sec_2D_next", null ]
    ] ],
    [ "Using the 3D API.", "md_docs_intro_api_3_d.html", [
      [ "Rendering model", "md_docs_intro_api_3_d.html#sec_3D_overview", null ],
      [ "Coordinate spaces and matrices", "md_docs_intro_api_3_d.html#sec_3D_pipeline", [
        [ "Short math reminder", "md_docs_intro_api_3_d.html#sec_3D_vectors_matrices", null ]
      ] ],
      [ "Renderer template parameters", "md_docs_intro_api_3_d.html#sec_3D_renderer_template", null ],
      [ "Shader state", "md_docs_intro_api_3_d.html#sec_3D_shaders", null ],
      [ "Projection, camera and model transform", "md_docs_intro_api_3_d.html#sec_3D_projection", null ],
      [ "Z-buffer", "md_docs_intro_api_3_d.html#sec_3D_zbuffer", null ],
      [ "Mesh3Dv2, Mesh3D and generated models", "md_docs_intro_api_3_d.html#sec_3D_meshes", null ],
      [ "Drawing primitives directly", "md_docs_intro_api_3_d.html#sec_3D_primitives", null ],
      [ "Textures", "md_docs_intro_api_3_d.html#sec_3D_textures", null ],
      [ "Light and material", "md_docs_intro_api_3_d.html#sec_3D_lighting", null ],
      [ "Back-face culling", "md_docs_intro_api_3_d.html#sec_3D_culling", null ],
      [ "Tile rendering", "md_docs_intro_api_3_d.html#sec_3D_tiling", null ],
      [ "Wireframe and debug drawing", "md_docs_intro_api_3_d.html#sec_3D_wireframe", null ],
      [ "Embedded performance checklist", "md_docs_intro_api_3_d.html#sec_3D_performance", null ],
      [ "Complete embedded example", "md_docs_intro_api_3_d.html#sec_3D_complete_example", null ],
      [ "Useful examples", "md_docs_intro_api_3_d.html#sec_3D_examples", null ],
      [ "Common pitfalls", "md_docs_intro_api_3_d.html#sec_3D_pitfalls", null ]
    ] ],
    [ "Mesh and validation tools", "md_docs_tools.html", [
      [ "Installing the tool environment", "md_docs_tools.html#tools_setup", null ],
      [ "Optional stripifier helpers", "md_docs_tools.html#tools_stripifiers", null ],
      [ "Mesh formats in practice", "md_docs_tools.html#tools_formats", null ],
      [ "Converting an OBJ model to Mesh3Dv2", "md_docs_tools.html#tools_obj_to_mesh3dv2", null ],
      [ "Migrating an existing Mesh3D header", "md_docs_tools.html#tools_legacy_to_mesh3dv2", null ],
      [ "Optimizing legacy Mesh3D output", "md_docs_tools.html#tools_legacy_mesh3d", null ],
      [ "Inspecting and visualizing meshes", "md_docs_tools.html#tools_inspect_view", null ],
      [ "Preprocessing and cleanup", "md_docs_tools.html#tools_cleanup", null ],
      [ "Validation tools", "md_docs_tools.html#tools_validation", null ],
      [ "Suggested workflow", "md_docs_tools.html#tools_workflow", null ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", null ],
        [ "Variables", "globals_vars.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_box2_8h.html",
"_vec3_8h.html#ac1caa239be43f9aa85c0016e491b726a",
"classtgx_1_1_renderer3_d.html#a845bbd628cfd96f379f3f23d880aa60a",
"md_docs_tools.html#tools_stripifiers",
"structtgx_1_1_r_g_b24.html#a4ab2a21ebf091abe62f3861ef0a51eeb",
"structtgx_1_1_vec2.html#a0dd3317719c780a7614027f9b50edb17"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';