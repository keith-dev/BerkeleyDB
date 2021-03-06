/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999
 *	Sleepycat Software.  All rights reserved.
 */

#include "db_config.h"

#ifndef lint
static const char sccsid[] = "@(#)os_errno.c	11.1 (Sleepycat) 7/25/99";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <errno.h>
#endif

#include "db_int.h"

/*
 * __os_get_errno --
 *	Return the value of errno.
 *
 * PUBLIC: int __os_get_errno __P((void));
 */
int
__os_get_errno()
{
	/* This routine must be able to return the same value repeatedly. */
	return (errno);
}

/*
 * __os_set_errno --
 *	Set the value of errno.
 *
 * PUBLIC: void __os_set_errno __P((int));
 */
void
__os_set_errno(evalue)
	int evalue;
{
	errno = evalue;
}
