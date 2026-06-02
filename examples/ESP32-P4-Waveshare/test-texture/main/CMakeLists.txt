idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS
        "../../../Teensy4/3D/test-texture/3Dmodels/spot"
        "../../../Teensy4/3D/test-texture/3Dmodels/bob"
        "../../../Teensy4/3D/test-texture/3Dmodels/blub"
    REQUIRES tgx_idf tgx_p4_display heap
)
