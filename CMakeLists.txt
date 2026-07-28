cmake_minimum_required(VERSION 4.0.7)

include(include_sdk.cmake)
project(edbus-rp2040 C CXX ASM)
pico_sdk_init()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_library(${PROJECT_NAME}
  src/edbus.c
)
target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib 
  hardware_pio
  hardware_dma
)
target_include_directories(${PROJECT_NAME} PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/src/edbus.pio)

install(TARGETS ${PROJECT_NAME})
install(DIRECTORY include/ DESTINATION include)

configure_file(
  cmake/edbus-rp2040-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/edbus-rp2040-config.cmake
  @ONLY
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/edbus-rp2040-config.cmake
  DESTINATION lib/cmake/edbus-rp2040
)
