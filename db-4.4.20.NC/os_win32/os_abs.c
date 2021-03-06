/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: os_abs.c,v 12.1 2005/06/16 20:23:28 bostic Exp $
 */

#include "db_config.h"

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
