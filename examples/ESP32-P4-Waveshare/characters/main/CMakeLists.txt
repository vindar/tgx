idf_component_register(
    SRCS "main.cpp"
    INCLUDE_DIRS
        "../../../Teensy4/3D/characters/3Dmodels/nanosuit"
        "../../../Teensy4/3D/characters/3Dmodels/R2D2"
        "../../../Teensy4/3D/characters/3Dmodels/elementalist"
        "../../../Teensy4/3D/characters/3Dmodels/sinbad"
        "../../../Teensy4/3D/characters/3Dmodels/cyborg"
        "../../../Teensy4/3D/characters/3Dmodels/dennis"
        "../../../Teensy4/3D/characters/3Dmodels/manga3"
        "../../../Teensy4/3D/characters/3Dmodels/naruto"
        "../../../Teensy4/3D/characters/3Dmodels/stormtrooper"
    REQUIRES tgx_idf tgx_p4_display heap
)
