set(TGX_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../../..")

idf_component_register(
    SRCS
        "${TGX_ROOT}/src/Color.cpp"
        "${TGX_ROOT}/src/Fonts.cpp"
        "${TGX_ROOT}/src/Renderer3D.cpp"
        "${TGX_ROOT}/src/font_tgx_OpenSans_Bold.cpp"
    INCLUDE_DIRS
        "${TGX_ROOT}/src"
)
