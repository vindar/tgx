idf_component_register(
    SRCS "tgx_p4_display.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_lcd esp_driver_gpio esp_driver_ppa esp_mm esp_timer freertos waveshare__esp_lcd_st7703
)
