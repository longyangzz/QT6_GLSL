
ADD_DEFINITIONS(-DDCGUI_LIB)
SET(LIB_NAME DCGui)
SET(HEADER_PATH ${CMAKE_SOURCE_DIR}/src/${LIB_NAME})

# =============== 2. 自动收集文件 ===============
# 收集所有 .cpp, .h, .hpp
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    "*.cpp"
    "*.h"
    "*.hpp"
    "*.hxx"
    "*.cxx"
    
)

# 收集 .ui 文件（AUTOUIC 会自动处理）
file(GLOB_RECURSE UIS CONFIGURE_DEPENDS "*.ui")

# 收集 .qrc 文件（AUTORCC 会自动处理）
file(GLOB_RECURSE RESOURCES CONFIGURE_DEPENDS "*.qrc")

# 合并所有文件（CMake 会自动识别类型）
set(ALL_FILES
    ${SOURCES}
    ${UIS}
    ${RESOURCES}
)

add_library(${LIB_NAME} SHARED ${ALL_FILES})

# =============== 5. 设置库属性 ===============
target_include_directories(${LIB_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src>  # 构建时包含当前目录
    $<INSTALL_INTERFACE:include>                    # 安装后头文件在 include/
)

# =============== 4. 链接 Qt 库 ===============
target_link_libraries(${LIB_NAME} PRIVATE
    ${APP_QT_TARGETS}
)

# =============== 5. 设置 C++ 标准（可选） ===============
target_compile_features(${LIB_NAME} PRIVATE cxx_std_17)

# 兼容单配置（Ninja, VS CMake 模式）和多配置（Visual Studio generator）
# === 统一设置输出目录 ===
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config (Visual Studio)
    foreach(CONF Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER "${CONF}" CONF_UPPER)
        set_target_properties(${LIB_NAME} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONF_UPPER} "${PROJECT_BINARY_DIR}/bin/${CONF}"
            LIBRARY_OUTPUT_DIRECTORY_${CONF_UPPER} "${PROJECT_BINARY_DIR}/bin/${CONF}"
            ARCHIVE_OUTPUT_DIRECTORY_${CONF_UPPER} "${PROJECT_BINARY_DIR}/lib/${CONF}"
        )
    endforeach()
else()
    # Single-config (Ninja, VS CMake mode)
    set_target_properties(${LIB_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin"
        ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib"
    )
endif()

install(TARGETS ${LIB_NAME}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}  # .dll on Windows
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}  # .so on Linux
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}  # .lib on Windows
)
