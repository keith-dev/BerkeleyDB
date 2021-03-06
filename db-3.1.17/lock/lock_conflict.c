/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 */

#include "db_config.h"

#ifndef lint
static const char revid[] = "$Id: lock_conflict.c,v 11.4 2000/04/29 02:26:24 bostic Exp $";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>
#endif

#include "db_int.h"

/*
 * The conflict arrays are set up such that the row is the lock you
 * are holding and the column is the lock that is desired.
 */
const u_int8_t db_rw_conflicts[] = {
	/*		N   R   W */
	/*   N */	0,  0,  0,
	/*   R */	0,  0,  1,
	/*   W */	0,  1,  1
};

const u_int8_t db_riw_conflicts[] = {
	/*		N	S	X	IX	IS	SIX */
	/*   N */	0,	0,	0,	0,	0,	0,
	/*   S */	0,	0,	1,	1,	0,	1,
	/*   X */	1,	1,	1,	1,	1,	1,
	/*  IX */	0,	1,	1,	0,	0,	0,
	/*  IS */	0,	0,	1,	0,	0,	0,
	/* SIX */	0,	1,	1,	0,	0,	0
};
