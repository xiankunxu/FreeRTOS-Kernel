cmake_minimum_required(VERSION 3.10)

add_library(cmsis_rtos_v2 OBJECT
    ${CMAKE_CURRENT_SOURCE_DIR}/cmsis_os2.c
)

target_include_directories(cmsis_rtos_v2 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cmsis_rtos_v2
    PUBLIC
    freertos_kernel_include
    freertos_kernel_port_headers
)