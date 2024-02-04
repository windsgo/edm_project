#pragma once

#include "config.h"

#define EDM_FMT          edm
#define EDM_FMT_NS_BEGIN namespace EDM_FMT {
#define EDM_FMT_NS_END   }

#ifdef EDM_USE_FMTLIB

#include <fmt/format.h>

EDM_FMT_NS_BEGIN
using fmt::format;
EDM_FMT_NS_END

#else // EDM_USE_FMTLIB

#include <format>

EDM_FMT_NS_BEGIN
using std::format;
EDM_FMT_NS_END

#endif // EDM_USE_FMTLIB
