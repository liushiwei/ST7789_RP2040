add_executable(st7789_test
        st7789_test.c
        st7789.c
        )

target_link_libraries(st7789_test pico_stdlib hardware_spi hardware_dma)

pico_enable_stdio_usb(st7789_test 1)

pico_add_extra_outputs(st7789_test)
