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
      [ "Color types", "md_docs_intro_basic.html#autotoc_md3", [
        [ "Blending and opacity parameter.", "md_docs_intro_basic.html#autotoc_md4", null ]
      ] ],
      [ "Image class, memory layout and sub-images", "md_docs_intro_basic.html#autotoc_md6", [
        [ "Creating an image", "md_docs_intro_basic.html#autotoc_md7", null ],
        [ "Memory layout and stride", "md_docs_intro_basic.html#autotoc_md8", null ],
        [ "Sub-images", "md_docs_intro_basic.html#autotoc_md9", null ]
      ] ],
      [ "Coordinates, vectors and boxes", "md_docs_intro_basic.html#autotoc_md11", [
        [ "Vector and box classes", "md_docs_intro_basic.html#autotoc_md12", null ],
        [ "Common beginner mistakes", "md_docs_intro_basic.html#autotoc_md13", null ]
      ] ],
      [ "Storing images in C++ source files", "md_docs_intro_basic.html#autotoc_md15", null ],
      [ "Copying/converting images to different color types", "md_docs_intro_basic.html#subsec_basic_copy_convert", null ],
      [ "Where to go next", "md_docs_intro_basic.html#autotoc_md16", null ]
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
    [ "Using the 3D API", "md_docs_intro_api_3_d.html", [
      [ "Renderer3D Overview", "md_docs_intro_api_3_d.html#sec_3D_overview", null ],
      [ "Minimal Render Loop", "md_docs_intro_api_3_d.html#sec_3D_minimal_loop", null ],
      [ "Destination Image, Viewport and Depth Buffer", "md_docs_intro_api_3_d.html#sec_3D_buffers", [
        [ "Tile Rendering", "md_docs_intro_api_3_d.html#sec_3D_tiling", null ]
      ] ],
      [ "From 3D Coordinates to Pixels", "md_docs_intro_api_3_d.html#sec_3D_coordinates", [
        [ "Math Types, Points and Normals", "md_docs_intro_api_3_d.html#sec_3D_math", null ],
        [ "Model, View and Projection", "md_docs_intro_api_3_d.html#sec_3D_model_view_projection", null ],
        [ "Projection and Viewport Mapping", "md_docs_intro_api_3_d.html#sec_3D_projection_viewport", null ]
      ] ],
      [ "Compile-time and Runtime Rendering Paths", "md_docs_intro_api_3_d.html#sec_3D_render_paths", [
        [ "Renderer Template Parameters", "md_docs_intro_api_3_d.html#sec_3D_template", null ],
        [ "Runtime Shader State", "md_docs_intro_api_3_d.html#sec_3D_shader_state", null ]
      ] ],
      [ "Geometry Sources", "md_docs_intro_api_3_d.html#sec_3D_geometry", [
        [ "Back-face Culling", "md_docs_intro_api_3_d.html#sec_3D_culling", null ],
        [ "Meshes", "md_docs_intro_api_3_d.html#sec_3D_meshes", null ],
        [ "Generated Solid Shapes", "md_docs_intro_api_3_d.html#sec_3D_generated_shapes", null ],
        [ "Low-level Primitives", "md_docs_intro_api_3_d.html#sec_3D_low_level", null ],
        [ "Skyboxes", "md_docs_intro_api_3_d.html#sec_3D_skybox", null ]
      ] ],
      [ "Appearance: Textures, Materials and Lighting", "md_docs_intro_api_3_d.html#sec_3D_appearance", [
        [ "Textures", "md_docs_intro_api_3_d.html#sec_3D_textures", null ],
        [ "Materials", "md_docs_intro_api_3_d.html#sec_3D_materials", null ],
        [ "Lighting Model", "md_docs_intro_api_3_d.html#sec_3D_lighting", null ],
        [ "Directional Lights", "md_docs_intro_api_3_d.html#sec_3D_directional_lights", null ],
        [ "Point and Spot Lights", "md_docs_intro_api_3_d.html#sec_3D_spot_lights", null ]
      ] ],
      [ "Wireframe and Debug Drawing", "md_docs_intro_api_3_d.html#sec_3D_wireframe", null ],
      [ "Embedded Performance Checklist", "md_docs_intro_api_3_d.html#sec_3D_performance", null ],
      [ "Complete Embedded Example", "md_docs_intro_api_3_d.html#sec_3D_complete_example", null ],
      [ "Useful Examples", "md_docs_intro_api_3_d.html#sec_3D_examples", null ],
      [ "Common Pitfalls", "md_docs_intro_api_3_d.html#sec_3D_pitfalls", null ]
    ] ],
    [ "TGX tools", "md_docs_tools.html", [
      [ "Installing the tools", "md_docs_tools.html#tools_install", null ],
      [ "tgx_image", "md_docs_tools.html#tools_image", null ],
      [ "tgx_mesh", "md_docs_tools.html#tools_mesh_gui", null ],
      [ "tgx_font", "md_docs_tools.html#tools_font", null ],
      [ "tgx_info", "md_docs_tools.html#tools_info", null ],
      [ "Command-line tools", "md_docs_tools.html#tools_cli", null ],
      [ "Generated files", "md_docs_tools.html#tools_generated_assets", null ],
      [ "Legacy tools", "md_docs_tools.html#tools_legacy", null ]
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
"_vec2_8h_source.html",
"classtgx_1_1_renderer3_d.html#a4a5ef919c4081d97f25f0b9d8635c82e",
"md_docs_intro_api_2_d.html#sec_2D_naming",
"structtgx_1_1_mat4.html#a6519e9224912a9f636b31891b59d8422",
"structtgx_1_1_r_g_b64.html#a444ed6d962272612ea938dd32ee6113f"
];

var SYNCONMSG = 'click to disable panel synchronisation';
var SYNCOFFMSG = 'click to enable panel synchronisation';