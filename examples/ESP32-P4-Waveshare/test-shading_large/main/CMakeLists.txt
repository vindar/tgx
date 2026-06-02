idf_component_register(
    SRCS
        "main.cpp"
    INCLUDE_DIRS
        "../../../Teensy4/3D/test-shading/3Dmodels/teapot"
        "../../../Teensy4/3D/test-shading/3Dmodels/bunny"
        "../../../Teensy4/3D/test-shading/3Dmodels/skull"
        "../../../Teensy4/3D/test-shading/3Dmodels/suzanne"
        "../../../Teensy4/3D/test-shading/3Dmodels/dragon"
    REQUIRES tgx_idf tgx_p4_display heap
)
