/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 1998, 1999
 *	Sleepycat Software.  All rights reserved.
 */

#include "db_config.h"

#ifndef lint
static const char sccsid[] = "@(#)os_abs.c	11.1 (Sleepycat) 7/25/99";
#endif /* not lint */

#include "db_int.h"

/*
 * __os_abspath --
 *	Return if a path is an absolute path.
 */
int
__os_abspath(path)
	const char *path;
{
	/*
	 * !!!
	 * Check for drive specifications, e.g., "C:".  In addition, the path
	 * separator used by the win32 DB (PATH_SEPARATOR) is \; look for both
	 * / and \ since these are user-input paths.
	 */
	if (isalpha(path[0]) && path[1] == ':')
		path += 2;
	return (path[0] == '/' || path[0] == '\\');
}
