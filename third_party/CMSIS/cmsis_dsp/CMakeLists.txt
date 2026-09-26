# 递归查找 cmsis_dsp 下所有 .c 文件
file(GLOB_RECURSE CMSIS_DSP_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

# 添加静态库
add_library(cmsis_dsp STATIC ${CMSIS_DSP_SRC})

# 添加头文件路径
target_include_directories(cmsis_dsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
)
