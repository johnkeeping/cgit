#ifndef CGIT_COMPAT_UTIL_H
#define CGIT_COMPAT_UTIL_H

#include "git/git-compat-util.h"

#ifdef NO_TIMEGM
#define timegm cgittimegm
extern time_t cgittimegm(struct tm *tm);
#endif

#endif
