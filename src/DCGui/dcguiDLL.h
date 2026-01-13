#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(DCGUI_LIB)
#  define DCGUI_EXPORT Q_DECL_EXPORT
# else
#  define DCGUI_EXPORT Q_DECL_IMPORT
# endif
#else
# define DCGUI_EXPORT
#endif
