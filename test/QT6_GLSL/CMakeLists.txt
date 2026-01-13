cmake_minimum_required(VERSION 3.19)
project(LidarPointCloudViewer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

# Enable OpenGL
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Executable
add_executable(${PROJECT_NAME}
    main.cpp
    PointCloudWidget.cpp
    PointCloudWidget.h
    shaders/pointcloud.vert
    shaders/pointcloud.frag
)

# Link Qt6
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
)

# Optional: copy data for testing
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})
