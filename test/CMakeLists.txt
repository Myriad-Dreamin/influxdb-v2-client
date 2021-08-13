
add_executable(influxTest test_write_main.cpp)
target_link_libraries(influxTest PRIVATE influxMClient)
target_compile_options(influxTest PRIVATE "-fsanitize=address")
target_link_libraries(influxTest PUBLIC "-fsanitize=address")
