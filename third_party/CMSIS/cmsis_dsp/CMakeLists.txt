# Recursively find all .c files under cmsis_dsp.
file(GLOB_RECURSE CMSIS_DSP_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

# Add the static library.
add_library(cmsis_dsp STATIC ${CMSIS_DSP_SRC})

# Add include paths.
target_include_directories(cmsis_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)
