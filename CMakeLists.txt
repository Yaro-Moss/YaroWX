cmake_minimum_required(VERSION 3.16)

project(myWeChat VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置自动处理Qt的moc、uic、rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 【统一所有模块的输出目录】
# 让可执行文件、库都输出到同一个目录，避免路径分散
set(OUTPUT_BASE_DIR "${CMAKE_BINARY_DIR}/bin")
# Debug/Release区分
foreach(CONFIG DEBUG RELEASE RELWITHDEBINFO MINSIZEREL)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BASE_DIR}/${CONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BASE_DIR}/${CONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${OUTPUT_BASE_DIR}/${CONFIG}")
endforeach()

# 添加cmake模块路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# 查找Qt包
find_package(QT NAMES Qt6 REQUIRED COMPONENTS
    Widgets
    Multimedia
    MultimediaWidgets
    Concurrent
    Network
)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
    Widgets
    Multimedia
    MultimediaWidgets
    Concurrent
    Network
)

# 设置编译定义
if(QT_VERSION_MAJOR EQUAL 6)
    add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050F00)
else()
    add_compile_definitions(QT_DISABLE_DEPRECATED_BEFORE=0x050000)
endif()

# 运行程序前，手动将FFmpeg可执行文件复制到对应输出目录：
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(FFMPEG_EXEC_NAME="ffmpeg.exe")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    add_compile_definitions(FFMPEG_EXEC_NAME="ffmpeg")
endif()

# 添加子目录（保留不变）
add_subdirectory(src/common)
add_subdirectory(src/network)
add_subdirectory(src/storage)
add_subdirectory(src/media)
add_subdirectory(src/controller)
add_subdirectory(test)
add_subdirectory(src/ui)