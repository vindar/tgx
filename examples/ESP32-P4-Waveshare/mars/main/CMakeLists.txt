set(MARS_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../Teensy4/3D/mars")

idf_component_register(
    SRCS
        "main.cpp"
        "${MARS_DIR}/marble.cpp"
        "${MARS_DIR}/mars_back.cpp"
        "${MARS_DIR}/mars_bottom.cpp"
        "${MARS_DIR}/mars_front.cpp"
        "${MARS_DIR}/mars_left.cpp"
        "${MARS_DIR}/mars_right.cpp"
        "${MARS_DIR}/mars_top_neb.cpp"
    INCLUDE_DIRS
        "${MARS_DIR}"
    REQUIRES tgx_idf tgx_p4_display heap
)
