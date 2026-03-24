file(GLOB_RECURSE COMMON_SOURCES
    "src/*.cpp"
)

file(GLOB_RECURSE COMMON_HEADERS
    "include/*.h"
)

# 创建库
add_library(common STATIC
    ${COMMON_SOURCES}
    ${COMMON_HEADERS}
)

# 设置包含目录（
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/models
    ${CMAKE_CURRENT_SOURCE_DIR}/../storage/include
)

# 查找 Qt Core 模块
find_package(Qt6 REQUIRED COMPONENTS Core Sql Gui)

# 链接 Qt Core 模块
target_link_libraries(common PRIVATE
    Qt6::Core
    Qt6::Sql
    Qt6::Gui
)


