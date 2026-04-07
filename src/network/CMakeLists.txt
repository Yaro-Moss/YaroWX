# 自动 MOC（必须，因为你用了 Q_OBJECT）
set(CMAKE_AUTOMOC ON)

# 自动扫描所有源文件
file(GLOB_RECURSE NET_SOURCES
    src/*.cpp
    include/*.h
)

# 创建静态库
add_library(network STATIC ${NET_SOURCES})

# 公开头文件路径
target_include_directories(network PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 正确链接 Qt 库（修复关键！）
target_link_libraries(network
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::WebSockets
    common
)