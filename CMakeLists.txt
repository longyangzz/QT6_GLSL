cmake_minimum_required(VERSION 3.30)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(/EHsc)

SET(MAJOR_VERSION 7)
SET(MINOR_VERSION 0)
SET(PATCH_VERSION 0)
SET(SOVERSION 10)

set(LOCAL_PROJECT_NAME LidarPointCloudViewer${MAJOR_VERSION}${MINOR_VERSION}${PATCH_VERSION})
PROJECT(${LOCAL_PROJECT_NAME})

# CMakeLists.txt
add_compile_definitions(NOMINMAX)
# ªÚ’Î∂‘ MSVC
if(MSVC)
    add_compile_options(/D "NOMINMAX")
endif()

# Find Qt6
find_package(QT NAMES Qt6 REQUIRED)
set(QT Qt${QT_VERSION_MAJOR})
MESSAGE(STATUS "Current=${QT}")

# We prepare lists of modules and libraries for different
# versions of Qt
set(APP_QT_MODULES Core Gui Widgets OpenGLWidgets Xml Sql PrintSupport Concurrent)
set(APP_QT_TARGETS ${QT}::Core ${QT}::Gui ${QT}::Widgets ${QT}::OpenGLWidgets ${QT}::Xml ${QT}::Sql ${QT}::PrintSupport ${QT}::Concurrent)

# Here everything is simple - find the modules we need.
find_package(${QT} REQUIRED ${APP_QT_MODULES})

# Enable OpenGL
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(test)