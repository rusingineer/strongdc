#ifdef _MSC_VER

#ifdef _WIN64
#include "opensslconf-msvc-x64.h"
#else
#include "opensslconf-msvc-x86.h"
#endif

#else
#include "opensslconf-mingw.h"
#endif
