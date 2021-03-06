/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: mp_trickle.c,v 12.9 2006/08/24 14:46:15 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"

static int __memp_trickle __P((DB_ENV *, int, int *));

/*
 * __memp_trickle_pp --
 *	DB_ENV->memp_trickle pre/post processing.
 *
 * PUBLIC: int __memp_trickle_pp __P((DB_ENV *, int, int *));
 */
int
__memp_trickle_pp(dbenv, pct, nwrotep)
	DB_ENV *dbenv;
	int pct, *nwrotep;
{
	DB_THREAD_INFO *ip;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->mp_handle, "memp_trickle", DB_INIT_MPOOL);

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__memp_trickle(dbenv, pct, nwrotep)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __memp_trickle --
 *	DB_ENV->memp_trickle.
 */
static int
__memp_trickle(dbenv, pct, nwrotep)
	DB_ENV *dbenv;
	int pct, *nwrotep;
{
	DB_MPOOL *dbmp;
	MPOOL *c_mp, *mp;
	u_int32_t clean, dirty, i, need_clean, total, dtmp, wrote;
	int ret;

	dbmp = dbenv->mp_handle;
	mp = dbmp->reginfo[0].primary;

	if (nwrotep != NULL)
		*nwrotep = 0;

	if (pct < 1 || pct > 100) {
		__db_errx(dbenv,
	    "DB_ENV->memp_trickle: %d: percent must be between 1 and 100",
		    pct);
		return (EINVAL);
	}

	/*
	 * Loop through the caches counting total/dirty buffers.
	 *
	 * XXX
	 * Using hash_page_dirty is our only choice at the moment, but it's not
	 * as correct as we might like in the presence of pools having more
	 * than one page size, as a free 512B buffer may not be equivalent to
	 * having a free 8KB buffer.
	 */
	for (ret = 0, i = dirty = total = 0; i < mp->nreg; ++i) {
		c_mp = dbmp->reginfo[i].primary;
		total += c_mp->stat.st_pages;
		__memp_stat_hash(&dbmp->reginfo[i], c_mp, &dtmp);
		dirty += dtmp;
	}

	/*
	 * If there are sufficient clean buffers, no buffers or no dirty
	 * buffers, we're done.
	 */
	if (total == 0 || dirty == 0)
		return (0);

	clean = total - dirty;
	need_clean = (total * (u_int)pct) / 100;
	if (clean >= need_clean)
		return (0);

	need_clean -= clean;
	ret = __memp_sync_int(
	    dbenv, NULL, need_clean, DB_SYNC_TRICKLE, &wrote);
	mp->stat.st_page_trickle += wrote;
	if (nwrotep != NULL)
		*nwrotep = (int)wrote;

	return (ret);
}
