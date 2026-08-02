#ifndef _PROJECT_DEFS_H
#define _PROJECT_DEFS_H

#if defined(BUILD_FOR_STC12C5A60S2_PDIP40)
	#include <STC/12C5AxxS2/PDIP40.h>
#elif !defined(__SDCC)
	#include <uni-STC/uni-STC.h>
#endif

#define SUPPRESS_delay1us_WARNING

#endif // _PROJECT_DEFS_H
