# Recursively find all .c files under cmsis_nn.
file(GLOB_RECURSE CMSIS_NN_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

# Add the static library.
add_library(cmsis_nn STATIC ${CMSIS_NN_SRC})

# Add include paths.
target_include_directories(cmsis_nn PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmsis_dsp/Include
)
