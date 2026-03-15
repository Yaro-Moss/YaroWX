file(GLOB_RECURSE NET_SOURCES "src/*.cpp" "include/*.h")
add_library(network ${NET_SOURCES})
target_include_directories(network PUBLIC "include")
# 链接库
target_link_libraries(network PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::WebSockets
    common
)
