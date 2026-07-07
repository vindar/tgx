idf_component_register(
    SRCS "app_main.cpp"
    INCLUDE_DIRS "." "../../common"
    REQUIRES tgx esp_timer esp_psram
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
