#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(GLSLVIEWER_LIB)
#  define GLSLVIEWER_EXPORT Q_DECL_EXPORT
# else
#  define GLSLVIEWER_EXPORT Q_DECL_IMPORT
# endif
#else
# define GLSLVIEWER_EXPORT
#endif
