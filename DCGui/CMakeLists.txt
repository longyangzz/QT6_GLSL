
IF(DYNAMIC_DCGp)
    ADD_DEFINITIONS(-DDCGUI_LIB)
ENDIF()

SET(LIB_NAME DCGui)
SET(HEADER_PATH ${DCGp_SOURCE_DIR}/src/BaseLibrarys/${LIB_NAME})

#Q_Object headerList  with Q_Object#
SET(SOURCES_H
    
)

#QT5_WRAP_CPP( SOURCES_H_MOC ${SOURCES_H} OPTIONS "-f" )

file( GLOB ui_list *.ui Impl/*.ui)
#qt5_wrap_ui( generated_ui_list ${ui_list} )
#qt5_wrap_ui( generated_ui_list ${ui_list} )
#qt5_add_resources( generated_qrc_list ${qrc_list} )
source_group( "UI Files" FILES ${ui_list}  )
source_group( "Generated Files" FILES ${MOC_SRCS} ${generated_ui_list} )
source_group( "Resource Files" FILES ${generated_qrc_list} )
## method 2
file( GLOB header_list *.h  impl/*.hxx)
SET(TARGET_H ${header_list} ${generated_ui_list})


file( GLOB cpp_list *.cpp  impl/*.cpp impl/*.cxx)
SET(TARGET_SRC
    ${cpp_list}
	${ui_list}
    ${SOURCES_H_MOC}
    ${DCGp_VERSIONINFO_RC}
)


IF(CMAKE_COMPILER_IS_GNUCXX)
    # Remove -pedantic flag as it barfs on Qt headers
   STRING(REGEX REPLACE "-pedantic" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
ENDIF()

SET(TARGET_LIBRARIES
    ${TARGET_LIBRARIES}
		DCCore
)

INCLUDE_DIRECTORIES(
		${QT_INCLUDE_DIR} 
		${QT_QTCORE_INCLUDE_DIR}
		${QT_QTGUI_INCLUDE_DIR}
		${QT_QTSQL_INCLUDE_DIR} 
		${QT_QTXML_INCLUDE_DIR} 
		${DCGp_BINARY_DIR}/src/BaseLibrarys/${LIB_NAME}
		${DCGp_SOURCE_DIR}/src/BaseLibrarys

	)

SETUP_BASELIBRARY(${LIB_NAME})

