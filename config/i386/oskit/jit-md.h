/*
 * Copyright (c) 1998 The University of Utah. All rights reserved.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file.
 *
 * Contributed by the Flux Research Group at the University of Utah.
 * Authors: Godmar Back
 */

#ifndef __i386_freebsd2_jit_md_h
#define __i386_freebsd2_jit_md_h

/**/
/* Include common information. */
/**/
#include "i386/jit.h"

/**/
/* Extra exception handling information. */
/**/

/* Function prototype for signal handlers */
#define	EXCEPTIONPROTO							\
	int sig, int code, struct sigcontext* ctx

/* Get the first exception frame from a signal handler */
#define	EXCEPTIONFRAME(f, c)						\
	(f).retbp = (c)->sc_ebp;					\
	(f).retpc = (c)->sc_eip + 1

#endif
