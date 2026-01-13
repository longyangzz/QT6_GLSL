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
# 或针对 MSVC
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

# =============== 4. 自动部署 Qt 运行时依赖（关键！）===============
# 需要 Qt6 的部署模块
include(cmake/Qt56CoreMacros.cmake)

qt56_generate_deploy_app_script(
    TARGET BCGP
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
	DEPLOY_TOOL_OPTIONS ${_QT56_DEPLOY_TOOL_OPTIONS}
)

install(SCRIPT ${deploy_script})
message("deploy_script=$ENV{WINDIR}/SysWOW64")
install(FILES "C:/Windows/SysWOW64/opengl32.dll" DESTINATION bin)

# ─── CPack packaging setup ───────────────────────────────

# Basic CPack settings
set(CPACK_PACKAGE_NAME "${LOCAL_PROJECT_NAME}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CONTACT "you@example.com")
set(CPACK_PACKAGE_VENDOR "Your Name or Company")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Example Qt app packaged via CPack")

# 假设资源在项目根目录的 res/ 文件夹
install(DIRECTORY ${CMAKE_SOURCE_DIR}/res/
        DESTINATION bin/
        FILES_MATCHING PATTERN "*")



set(QT_PLUGIN_SUBDIRS
    iconengines
    imageformats
    platforms
    sqldrivers
    styles
)
message("QT_PLUGIN_SUBDIRS-${QT_PLUGIN_SUBDIRS}")

# Qt6 6.3+ 可能提供这些变量（但非标准，不保证）
if(DEFINED Qt6_INSTALL_PREFIX)
    set(QT_PLUGIN_DIR "${Qt6_INSTALL_PREFIX}/plugins")
else()
    # 回退方案
    get_filename_component(QT_PLUGIN_DIR "${Qt6_DIR}/../../../plugins" ABSOLUTE)
endif()

message("QT_PLUGIN_DIR${QT_PLUGIN_DIR}")
foreach(subdir ${QT_PLUGIN_SUBDIRS})
    if(EXISTS "${QT_PLUGIN_DIR}/${subdir}")
        install(DIRECTORY "${QT_PLUGIN_DIR}/${subdir}/"
                DESTINATION plugins/${subdir}
                FILES_MATCHING PATTERN "*.dll")
    endif()
endforeach()


if(WIN32)
  # NSIS installer on Windows
  set(CPACK_GENERATOR "NSIS")
  set(CPACK_NSIS_MODIFY_PATH ON)
  set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME} Installer")
  set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
  set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")
  # 关键：禁止 CPack 自动包含系统 DLL
  set(CPACK_COMPONENTS_ALL_IN_ONE_PACKAGE ON)
  set(CPACK_NSIS_INCLUDE_STANDARD_MODULES OFF)  # 示例：NSIS
  #set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\myapp.exe")
elseif(UNIX AND EXISTS "/etc/debian_version")
  # DEB package
  set(CMAKE_INSTALL_PREFIX /usr/local/${PROJECT_NAME})
  set(CPACK_PACKAGING_INSTALL_PREFIX /usr/local/${PROJECT_NAME})
  set(CPACK_GENERATOR "DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "you@example.com")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.29)")
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
  set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
elseif(UNIX AND EXISTS "/etc/redhat-release")
  # RPM package
  set(CPACK_GENERATOR "RPM")
  set(CPACK_RPM_PACKAGE_LICENSE "MIT")
  set(CPACK_RPM_PACKAGE_RELEASE 1)
  set(CPACK_RPM_PACKAGE_GROUP "Applications/Utilities")
endif()

include(CPack)