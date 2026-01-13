#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(DCLCMG_LIB)
#  define DCLCMG_EXPORT Q_DECL_EXPORT
# else
#  define DCLCMG_EXPORT Q_DECL_IMPORT
# endif
#else
# define DCLCMG_EXPORT
#endif
