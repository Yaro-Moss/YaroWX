file(GLOB_RECURSE STORAGE_SOURCES "src/*.cpp" "include/*.h")

# 创建库
add_library(storage STATIC ${STORAGE_SOURCES})

# 包含目录
target_include_directories(storage PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include 
)

# 查找必要的Qt模块
find_package(Qt6 REQUIRED COMPONENTS Core Sql)

# 链接Qt库
target_link_libraries(storage PRIVATE
    Qt6::Core
    Qt6::Sql
    common
)



