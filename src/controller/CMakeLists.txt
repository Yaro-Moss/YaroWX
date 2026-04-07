file(GLOB_RECURSE  CONTROLLER_SOURCES
    "src/*.cpp"
)

file(GLOB_RECURSE  CONTROLLER_HEADERS
    "include/*.h"
)

# 创建控制层库
add_library(controller STATIC
    ${CONTROLLER_SOURCES}
    ${CONTROLLER_HEADERS}
)

# 设置包含目录
target_include_directories(controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/conversationController
    ${CMAKE_CURRENT_SOURCE_DIR}/include/messageController
    ${CMAKE_CURRENT_SOURCE_DIR}/include/databaseInitializationController
    ${CMAKE_CURRENT_SOURCE_DIR}/include/userController
    ${CMAKE_CURRENT_SOURCE_DIR}/include/contactController
    ${CMAKE_CURRENT_SOURCE_DIR}/include/groupController
    ${CMAKE_CURRENT_SOURCE_DIR}/include/localMomentController
)


# 链接依赖
target_link_libraries(controller PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Concurrent
    storage
    common
    media
    network
)

