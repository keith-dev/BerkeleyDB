/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2000-2004
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: util_cache.c,v 1.8 2004/02/17 16:03:05 bostic Exp $
 */

#include "db_config.h"

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <stdlib.h>

#include <string.h>
#endif

#include "db_int.h"

/*
 * __db_util_cache --
 *	Compute if we have enough cache.
 *
 * PUBLIC: int __db_util_cache __P((DB *, u_int32_t *, int *));
 */
int
__db_util_cache(dbp, cachep, resizep)
	DB *dbp;
	u_int32_t *cachep;
	int *resizep;
{
	u_int32_t pgsize;
	int ret;

	/* Get the current page size. */
	if ((ret = dbp->get_pagesize(dbp, &pgsize)) != 0)
		return (ret);

	/*
	 * The current cache size is in cachep.  If it's insufficient, set the
	 * the memory referenced by resizep to 1 and set cachep to the minimum
	 * size needed.
	 *
	 * Make sure our current cache is big enough.  We want at least
	 * DB_MINPAGECACHE pages in the cache.
	 */
	if ((*cachep / pgsize) < DB_MINPAGECACHE) {
		*resizep = 1;
		*cachep = pgsize * DB_MINPAGECACHE;
	} else
		*resizep = 0;

	return (0);
}
