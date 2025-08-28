# 递归查找 cmsis_nn 下所有 .c 文件
file(GLOB_RECURSE CMSIS_NN_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/*.c"
)

# 添加静态库
add_library(cmsis_nn STATIC ${CMSIS_NN_SRC})

# 添加头文件路径
target_include_directories(cmsis_nn PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../cmsis_dsp/Include
)
