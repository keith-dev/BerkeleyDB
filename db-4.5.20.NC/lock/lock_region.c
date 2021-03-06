/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: lock_region.c,v 12.11 2006/08/24 14:46:11 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/lock.h"

static int  __lock_region_init __P((DB_ENV *, DB_LOCKTAB *));
static size_t
	    __lock_region_size __P((DB_ENV *));

/*
 * The conflict arrays are set up such that the row is the lock you are
 * holding and the column is the lock that is desired.
 */
#define	DB_LOCK_RIW_N	9
static const u_int8_t db_riw_conflicts[] = {
/*         N   R   W   WT  IW  IR  RIW DR  WW */
/*   N */  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*   R */  0,  0,  1,  0,  1,  0,  1,  0,  1,
/*   W */  0,  1,  1,  1,  1,  1,  1,  1,  1,
/*  WT */  0,  0,  0,  0,  0,  0,  0,  0,  0,
/*  IW */  0,  1,  1,  0,  0,  0,  0,  1,  1,
/*  IR */  0,  0,  1,  0,  0,  0,  0,  0,  1,
/* RIW */  0,  1,  1,  0,  0,  0,  0,  1,  1,
/*  DR */  0,  0,  1,  0,  1,  0,  1,  0,  0,
/*  WW */  0,  1,  1,  0,  1,  1,  1,  0,  1
};

/*
 * This conflict array is used for concurrent db access (CDB).  It uses
 * the same locks as the db_riw_conflicts array, but adds an IW mode to
 * be used for write cursors.
 */
#define	DB_LOCK_CDB_N	5
static const u_int8_t db_cdb_conflicts[] = {
	/*		N	R	W	WT	IW */
	/*   N */	0,	0,	0,	0,	0,
	/*   R */	0,	0,	1,	0,	0,
	/*   W */	0,	1,	1,	1,	1,
	/*  WT */	0,	0,	0,	0,	0,
	/*  IW */	0,	0,	1,	0,	1
};

/*
 * __lock_open --
 *	Internal version of lock_open: only called from DB_ENV->open.
 *
 * PUBLIC: int __lock_open __P((DB_ENV *));
 */
int
__lock_open(dbenv)
	DB_ENV *dbenv;
{
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	size_t size;
	int region_locked, ret;

	region_locked = 0;

	/* Create the lock table structure. */
	if ((ret = __os_calloc(dbenv, 1, sizeof(DB_LOCKTAB), &lt)) != 0)
		return (ret);
	lt->dbenv = dbenv;

	/* Join/create the lock region. */
	lt->reginfo.dbenv = dbenv;
	lt->reginfo.type = REGION_TYPE_LOCK;
	lt->reginfo.id = INVALID_REGION_ID;
	lt->reginfo.flags = REGION_JOIN_OK;
	if (F_ISSET(dbenv, DB_ENV_CREATE))
		F_SET(&lt->reginfo, REGION_CREATE_OK);
	size = __lock_region_size(dbenv);
	if ((ret = __db_r_attach(dbenv, &lt->reginfo, size)) != 0)
		goto err;

	/* If we created the region, initialize it. */
	if (F_ISSET(&lt->reginfo, REGION_CREATE))
		if ((ret = __lock_region_init(dbenv, lt)) != 0)
			goto err;

	/* Set the local addresses. */
	region = lt->reginfo.primary =
	    R_ADDR(&lt->reginfo, lt->reginfo.rp->primary);

	/* Set remaining pointers into region. */
	lt->conflicts = R_ADDR(&lt->reginfo, region->conf_off);
	lt->obj_tab = R_ADDR(&lt->reginfo, region->obj_off);
	lt->locker_tab = R_ADDR(&lt->reginfo, region->locker_off);

	dbenv->lk_handle = lt;

	LOCK_SYSTEM_LOCK(dbenv);
	region_locked = 1;

	if (dbenv->lk_detect != DB_LOCK_NORUN) {
		/*
		 * Check for incompatible automatic deadlock detection requests.
		 * There are scenarios where changing the detector configuration
		 * is reasonable, but we disallow them guessing it is likely to
		 * be an application error.
		 *
		 * We allow applications to turn on the lock detector, and we
		 * ignore attempts to set it to the default or current value.
		 */
		if (region->detect != DB_LOCK_NORUN &&
		    dbenv->lk_detect != DB_LOCK_DEFAULT &&
		    region->detect != dbenv->lk_detect) {
			__db_errx(dbenv,
		    "lock_open: incompatible deadlock detector mode");
			ret = EINVAL;
			goto err;
		}
		if (region->detect == DB_LOCK_NORUN)
			region->detect = dbenv->lk_detect;
	}

	/*
	 * A process joining the region may have reset the lock and transaction
	 * timeouts.
	 */
	if (dbenv->lk_timeout != 0)
		region->lk_timeout = dbenv->lk_timeout;
	if (dbenv->tx_timeout != 0)
		region->tx_timeout = dbenv->tx_timeout;

	LOCK_SYSTEM_UNLOCK(dbenv);
	region_locked = 0;

	return (0);

err:	dbenv->lk_handle = NULL;
	if (lt->reginfo.addr != NULL) {
		if (region_locked)
			LOCK_SYSTEM_UNLOCK(dbenv);
		(void)__db_r_detach(dbenv, &lt->reginfo, 0);
	}

	__os_free(dbenv, lt);
	return (ret);
}

/*
 * __lock_region_init --
 *	Initialize the lock region.
 */
static int
__lock_region_init(dbenv, lt)
	DB_ENV *dbenv;
	DB_LOCKTAB *lt;
{
	const u_int8_t *lk_conflicts;
	struct __db_lock *lp;
	DB_LOCKER *lidp;
	DB_LOCKOBJ *op;
	DB_LOCKREGION *region;
	u_int32_t i;
	u_int8_t *addr;
	int lk_modes, ret;

	if ((ret = __db_shalloc(&lt->reginfo,
	    sizeof(DB_LOCKREGION), 0, &lt->reginfo.primary)) != 0)
		goto mem_err;
	lt->reginfo.rp->primary = R_OFFSET(&lt->reginfo, lt->reginfo.primary);
	region = lt->reginfo.primary;
	memset(region, 0, sizeof(*region));

	if ((ret = __mutex_alloc(
	    dbenv, MTX_LOCK_REGION, 0, &region->mtx_region)) != 0)
		return (ret);

	/* Select a conflict matrix if none specified. */
	if (dbenv->lk_modes == 0)
		if (CDB_LOCKING(dbenv)) {
			lk_modes = DB_LOCK_CDB_N;
			lk_conflicts = db_cdb_conflicts;
		} else {
			lk_modes = DB_LOCK_RIW_N;
			lk_conflicts = db_riw_conflicts;
		}
	else {
		lk_modes = dbenv->lk_modes;
		lk_conflicts = dbenv->lk_conflicts;
	}

	region->need_dd = 0;
	LOCK_SET_TIME_INVALID(&region->next_timeout);
	region->detect = DB_LOCK_NORUN;
	region->lk_timeout = dbenv->lk_timeout;
	region->tx_timeout = dbenv->tx_timeout;
	region->locker_t_size = __db_tablesize(dbenv->lk_max_lockers);
	region->object_t_size = __db_tablesize(dbenv->lk_max_objects);
	memset(&region->stat, 0, sizeof(region->stat));
	region->stat.st_id = 0;
	region->stat.st_cur_maxid = DB_LOCK_MAXID;
	region->stat.st_maxlocks = dbenv->lk_max;
	region->stat.st_maxlockers = dbenv->lk_max_lockers;
	region->stat.st_maxobjects = dbenv->lk_max_objects;
	region->stat.st_nmodes = lk_modes;

	/* Allocate room for the conflict matrix and initialize it. */
	if ((ret = __db_shalloc(
	    &lt->reginfo, (size_t)(lk_modes * lk_modes), 0, &addr)) != 0)
		goto mem_err;
	memcpy(addr, lk_conflicts, (size_t)(lk_modes * lk_modes));
	region->conf_off = R_OFFSET(&lt->reginfo, addr);

	/* Allocate room for the object hash table and initialize it. */
	if ((ret = __db_shalloc(&lt->reginfo,
	    region->object_t_size * sizeof(DB_HASHTAB), 0, &addr)) != 0)
		goto mem_err;
	__db_hashinit(addr, region->object_t_size);
	region->obj_off = R_OFFSET(&lt->reginfo, addr);

	/* Allocate room for the locker hash table and initialize it. */
	if ((ret = __db_shalloc(&lt->reginfo,
	    region->locker_t_size * sizeof(DB_HASHTAB), 0, &addr)) != 0)
		goto mem_err;
	__db_hashinit(addr, region->locker_t_size);
	region->locker_off = R_OFFSET(&lt->reginfo, addr);

	/* Initialize locks onto a free list. */
	SH_TAILQ_INIT(&region->free_locks);
	for (i = 0; i < region->stat.st_maxlocks; ++i) {
		if ((ret = __db_shalloc(&lt->reginfo,
		    sizeof(struct __db_lock), 0, &lp)) != 0)
			goto mem_err;
		lp->mtx_lock = MUTEX_INVALID;
		lp->gen = 0;
		lp->status = DB_LSTAT_FREE;
		SH_TAILQ_INSERT_HEAD(&region->free_locks, lp, links, __db_lock);
	}

	/* Initialize objects onto a free list.  */
	SH_TAILQ_INIT(&region->dd_objs);
	SH_TAILQ_INIT(&region->free_objs);
	for (i = 0; i < region->stat.st_maxobjects; ++i) {
		if ((ret = __db_shalloc(&lt->reginfo,
		    sizeof(DB_LOCKOBJ), 0, &op)) != 0)
			goto mem_err;
		SH_TAILQ_INSERT_HEAD(
		    &region->free_objs, op, links, __db_lockobj);
	}

	/* Initialize lockers onto a free list.  */
	SH_TAILQ_INIT(&region->lockers);
	SH_TAILQ_INIT(&region->free_lockers);
	for (i = 0; i < region->stat.st_maxlockers; ++i) {
		if ((ret = __db_shalloc(&lt->reginfo,
		    sizeof(DB_LOCKER), 0, &lidp)) != 0) {
mem_err:		__db_errx(dbenv,
			    "Unable to allocate memory for the lock table");
			return (ret);
		}
		SH_TAILQ_INSERT_HEAD(
		    &region->free_lockers, lidp, links, __db_locker);
	}

	return (0);
}

/*
 * __lock_dbenv_refresh --
 *	Clean up after the lock system on a close or failed open.
 *
 * PUBLIC: int __lock_dbenv_refresh __P((DB_ENV *));
 */
int
__lock_dbenv_refresh(dbenv)
	DB_ENV *dbenv;
{
	struct __db_lock *lp;
	DB_LOCKER *locker;
	DB_LOCKOBJ *lockobj;
	DB_LOCKREGION *lr;
	DB_LOCKTAB *lt;
	REGINFO *reginfo;
	int ret;

	lt = dbenv->lk_handle;
	reginfo = &lt->reginfo;
	lr = reginfo->primary;

	/*
	 * If a private region, return the memory to the heap.  Not needed for
	 * filesystem-backed or system shared memory regions, that memory isn't
	 * owned by any particular process.
	 */
	if (F_ISSET(dbenv, DB_ENV_PRIVATE)) {
		/* Discard the conflict matrix. */
		__db_shalloc_free(reginfo, R_ADDR(reginfo, lr->conf_off));

		/* Discard the object hash table. */
		__db_shalloc_free(reginfo, R_ADDR(reginfo, lr->obj_off));

		/* Discard the locker hash table. */
		__db_shalloc_free(reginfo, R_ADDR(reginfo, lr->locker_off));

		/* Discard locks. */
		while ((lp =
		    SH_TAILQ_FIRST(&lr->free_locks, __db_lock)) != NULL) {
			SH_TAILQ_REMOVE(&lr->free_locks, lp, links, __db_lock);
			__db_shalloc_free(reginfo, lp);
		}

		/* Discard objects. */
		while ((lockobj =
		    SH_TAILQ_FIRST(&lr->free_objs, __db_lockobj)) != NULL) {
			SH_TAILQ_REMOVE(
			    &lr->free_objs, lockobj, links, __db_lockobj);
			__db_shalloc_free(reginfo, lockobj);
		}

		/* Discard lockers. */
		while ((locker =
		    SH_TAILQ_FIRST(&lr->free_lockers, __db_locker)) != NULL) {
			SH_TAILQ_REMOVE(
			    &lr->free_lockers, locker, links, __db_locker);
			__db_shalloc_free(reginfo, locker);
		}
	}

	/* Detach from the region. */
	ret = __db_r_detach(dbenv, reginfo, 0);

	/* Discard DB_LOCKTAB. */
	__os_free(dbenv, lt);
	dbenv->lk_handle = NULL;

	return (ret);
}

/*
 * __lock_region_mutex_count --
 *	Return the number of mutexes the lock region will need.
 *
 * PUBLIC: u_int32_t __lock_region_mutex_count __P((DB_ENV *));
 */
u_int32_t
__lock_region_mutex_count(dbenv)
	DB_ENV *dbenv;
{
	return (dbenv->lk_max);
}

/*
 * __lock_region_size --
 *	Return the region size.
 */
static size_t
__lock_region_size(dbenv)
	DB_ENV *dbenv;
{
	size_t retval;

	/*
	 * Figure out how much space we're going to need.  This list should
	 * map one-to-one with the __db_shalloc calls in __lock_region_init.
	 */
	retval = 0;
	retval += __db_shalloc_size(sizeof(DB_LOCKREGION), 0);
	retval += __db_shalloc_size(
	    (size_t)(dbenv->lk_modes * dbenv->lk_modes), 0);
	retval += __db_shalloc_size(
	    __db_tablesize(dbenv->lk_max_objects) * (sizeof(DB_HASHTAB)), 0);
	retval += __db_shalloc_size(
	    __db_tablesize(dbenv->lk_max_lockers) * (sizeof(DB_HASHTAB)), 0);
	retval +=
	    __db_shalloc_size(sizeof(struct __db_lock), 0) * dbenv->lk_max;
	retval +=
	    __db_shalloc_size(sizeof(DB_LOCKOBJ), 0) * dbenv->lk_max_objects;
	retval +=
	    __db_shalloc_size(sizeof(DB_LOCKER), 0) * dbenv->lk_max_lockers;

	/*
	 * Include 16 bytes of string space per lock.  DB doesn't use it
	 * because we pre-allocate lock space for DBTs in the structure.
	 */
	retval += __db_shalloc_size(dbenv->lk_max * 16, sizeof(size_t));

	/* And we keep getting this wrong, let's be generous. */
	retval += retval / 4;

	return (retval);
}
