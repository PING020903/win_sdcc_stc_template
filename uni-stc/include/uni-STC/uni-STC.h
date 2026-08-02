/**
 * @file uni-STC/uni-STC.h
 * 
 * This file contains the definitions expected when compiling firmware
 * as well as unit tests (with GCC or clang).
 */

#ifndef _UNISTC_UNISTC_H
#define _UNISTC_UNISTC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __SDCC
	#define CRITICAL __critical
	#define REENTRANT __reentrant
	#define USING(registerBank) __using(registerBank)
	// With SDCC, we always optimise (generally for size), so no problem.
	#define INLINE inline
	// Remove the stupid warning "function declarator with no prototype"
	#pragma disable_warning 283
#else
	#define CRITICAL
	#define REENTRANT
	#define USING(registerBank)
	#define INTERRUPT(name, vector) void name()
	#define INTERRUPT_USING(name, vector, regnum) void name()
	// GCC & CLANG never inline functions unless optimisation is requested, 
	// which is not the case by default, so we need to insist a little bit.
	#define INLINE inline __attribute__((always_inline))
	#define __data
	#define __idata
	#define __pdata
	#define __xdata
	#define __code
#endif // __SDCC

#define elementsof(array) (sizeof(array) / sizeof(array[0]))
#define byte_mask(nbits, spos) ((0xff >> (8 - nbits)) << (spos))

#endif // _UNISTC_UNISTC_H
