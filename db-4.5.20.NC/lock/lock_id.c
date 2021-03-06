/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: lock_id.c,v 12.16 2006/08/24 14:46:11 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"

/*
 * __lock_id_pp --
 *	DB_ENV->lock_id pre/post processing.
 *
 * PUBLIC: int __lock_id_pp __P((DB_ENV *, u_int32_t *));
 */
int
__lock_id_pp(dbenv, idp)
	DB_ENV *dbenv;
	u_int32_t *idp;
{
	DB_THREAD_INFO *ip;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->lk_handle, "DB_ENV->lock_id", DB_INIT_LOCK);

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__lock_id(dbenv, idp, NULL)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __lock_id --
 *	DB_ENV->lock_id.
 *
 * PUBLIC: int  __lock_id __P((DB_ENV *, u_int32_t *, DB_LOCKER **));
 */
int
__lock_id(dbenv, idp, lkp)
	DB_ENV *dbenv;
	u_int32_t *idp;
	DB_LOCKER **lkp;
{
	DB_LOCKER *lk;
	DB_LOCKTAB *lt;
	DB_LOCKREGION *region;
	u_int32_t id, *ids, locker_ndx;
	int nids, ret;

	lt = dbenv->lk_handle;
	region = lt->reginfo.primary;
	ret = 0;

	id = DB_LOCK_INVALIDID;
	lk = NULL;

	LOCK_SYSTEM_LOCK(dbenv);

	/*
	 * Allocate a new lock id.  If we wrap around then we find the minimum
	 * currently in use and make sure we can stay below that.  This code is
	 * similar to code in __txn_begin_int for recovering txn ids.
	 *
	 * Our current valid range can span the maximum valid value, so check
	 * for it and wrap manually.
	 */
	if (region->stat.st_id == DB_LOCK_MAXID &&
	    region->stat.st_cur_maxid != DB_LOCK_MAXID)
		region->stat.st_id = DB_LOCK_INVALIDID;
	if (region->stat.st_id == region->stat.st_cur_maxid) {
		if ((ret = __os_malloc(dbenv,
		    sizeof(u_int32_t) * region->stat.st_nlockers, &ids)) != 0)
			goto err;
		nids = 0;
		SH_TAILQ_FOREACH(lk, &region->lockers, ulinks, __db_locker)
			ids[nids++] = lk->id;
		region->stat.st_id = DB_LOCK_INVALIDID;
		region->stat.st_cur_maxid = DB_LOCK_MAXID;
		if (nids != 0)
			__db_idspace(ids, nids,
			    &region->stat.st_id, &region->stat.st_cur_maxid);
		__os_free(dbenv, ids);
	}
	id = ++region->stat.st_id;

	/* Allocate a locker for this id. */
	LOCKER_LOCK(lt, region, id, locker_ndx);
	ret = __lock_getlocker(lt, id, locker_ndx, 1, &lk);

err:	LOCK_SYSTEM_UNLOCK(dbenv);

	if (idp)
		*idp = id;
	if (lkp)
		*lkp = lk;
	return (ret);
}

/*
 * __lock_set_thread_id --
 *	Set the thread_id in an existing locker.
 * PUBLIC: void __lock_set_thread_id __P((DB_LOCKER *, pid_t, db_threadid_t));
 */
void
__lock_set_thread_id(lref, pid, tid)
	DB_LOCKER *lref;
	pid_t pid;
	db_threadid_t tid;
{
	lref->pid = pid;
	lref->tid = tid;
}

/*
 * __lock_id_free_pp --
 *	DB_ENV->lock_id_free pre/post processing.
 *
 * PUBLIC: int __lock_id_free_pp __P((DB_ENV *, u_int32_t));
 */
int
__lock_id_free_pp(dbenv, id)
	DB_ENV *dbenv;
	u_int32_t id;
{
	DB_THREAD_INFO *ip;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->lk_handle, "DB_ENV->lock_id_free", DB_INIT_LOCK);

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__lock_id_free(dbenv, id)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __lock_id_free --
 *	Free a locker id.
 *
 * PUBLIC: int  __lock_id_free __P((DB_ENV *, u_int32_t));
 */
int
__lock_id_free(dbenv, id)
	DB_ENV *dbenv;
	u_int32_t id;
{
	DB_LOCKER *sh_locker;
	DB_LOCKTAB *lt;
	DB_LOCKREGION *region;
	u_int32_t locker_ndx;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->lk_handle, "DB_ENV->lock_id_free", DB_INIT_LOCK);

	lt = dbenv->lk_handle;
	region = lt->reginfo.primary;

	LOCK_SYSTEM_LOCK(dbenv);
	LOCKER_LOCK(lt, region, id, locker_ndx);
	if ((ret = __lock_getlocker(lt, id, locker_ndx, 0, &sh_locker)) != 0)
		goto err;

	if (sh_locker == NULL) {
		__db_errx(dbenv, "Unknown locker ID: %lx", (u_long)id);
		ret = EINVAL;
		goto err;
	}

	if (sh_locker->nlocks != 0) {
		__db_errx(dbenv, "Locker still has locks");
		ret = EINVAL;
		goto err;
	}

	__lock_freelocker(lt, region, sh_locker, locker_ndx);

err:	LOCK_SYSTEM_UNLOCK(dbenv);
	return (ret);
}

/*
 * __lock_id_set --
 *	Set the current locker ID and current maximum unused ID (for
 *	testing purposes only).
 *
 * PUBLIC: int __lock_id_set __P((DB_ENV *, u_int32_t, u_int32_t));
 */
int
__lock_id_set(dbenv, cur_id, max_id)
	DB_ENV *dbenv;
	u_int32_t cur_id, max_id;
{
	DB_LOCKTAB *lt;
	DB_LOCKREGION *region;

	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->lk_handle, "lock_id_set", DB_INIT_LOCK);

	lt = dbenv->lk_handle;
	region = lt->reginfo.primary;
	region->stat.st_id = cur_id;
	region->stat.st_cur_maxid = max_id;

	return (0);
}

/*
 * __lock_getlocker --
 *	Get a locker in the locker hash table.  The create parameter
 * indicates if the locker should be created if it doesn't exist in
 * the table.
 *
 * This must be called with the locker bucket locked.
 *
 * PUBLIC: int __lock_getlocker __P((DB_LOCKTAB *,
 * PUBLIC:     u_int32_t, u_int32_t, int, DB_LOCKER **));
 */
int
__lock_getlocker(lt, locker, indx, create, retp)
	DB_LOCKTAB *lt;
	u_int32_t locker, indx;
	int create;
	DB_LOCKER **retp;
{
	DB_ENV *dbenv;
	DB_LOCKER *sh_locker;
	DB_LOCKREGION *region;

	dbenv = lt->dbenv;
	region = lt->reginfo.primary;

	/*
	 * If we find the locker, then we can just return it.  If we don't find
	 * the locker, then we need to create it.
	 */
	SH_TAILQ_FOREACH(sh_locker, &lt->locker_tab[indx], links, __db_locker)
		if (sh_locker->id == locker)
			break;
	if (sh_locker == NULL && create) {
		/* Create new locker and then insert it into hash table. */
		if ((sh_locker = SH_TAILQ_FIRST(
		    &region->free_lockers, __db_locker)) == NULL)
			return (__lock_nomem(dbenv, "locker entries"));
		SH_TAILQ_REMOVE(
		    &region->free_lockers, sh_locker, links, __db_locker);
		if (++region->stat.st_nlockers > region->stat.st_maxnlockers)
			region->stat.st_maxnlockers = region->stat.st_nlockers;

		sh_locker->id = locker;
		dbenv->thread_id(dbenv, &sh_locker->pid, &sh_locker->tid);
		sh_locker->dd_id = 0;
		sh_locker->master_locker = INVALID_ROFF;
		sh_locker->parent_locker = INVALID_ROFF;
		SH_LIST_INIT(&sh_locker->child_locker);
		sh_locker->flags = 0;
		SH_LIST_INIT(&sh_locker->heldby);
		sh_locker->nlocks = 0;
		sh_locker->nwrites = 0;
		sh_locker->lk_timeout = 0;
		LOCK_SET_TIME_INVALID(&sh_locker->tx_expire);
		LOCK_SET_TIME_INVALID(&sh_locker->lk_expire);

		SH_TAILQ_INSERT_HEAD(
		    &lt->locker_tab[indx], sh_locker, links, __db_locker);
		SH_TAILQ_INSERT_HEAD(&region->lockers,
		    sh_locker, ulinks, __db_locker);
	}

	*retp = sh_locker;
	return (0);
}

/*
 * __lock_addfamilylocker
 *	Put a locker entry in for a child transaction.
 *
 * PUBLIC: int __lock_addfamilylocker __P((DB_ENV *, u_int32_t, u_int32_t));
 */
int
__lock_addfamilylocker(dbenv, pid, id)
	DB_ENV *dbenv;
	u_int32_t pid, id;
{
	DB_LOCKER *lockerp, *mlockerp;
	DB_LOCKREGION *region;
	DB_LOCKTAB *lt;
	u_int32_t ndx;
	int ret;

	lt = dbenv->lk_handle;
	region = lt->reginfo.primary;
	LOCK_SYSTEM_LOCK(dbenv);

	/* get/create the  parent locker info */
	LOCKER_LOCK(lt, region, pid, ndx);
	if ((ret = __lock_getlocker(lt, pid, ndx, 1, &mlockerp)) != 0)
		goto err;

	/*
	 * We assume that only one thread can manipulate
	 * a single transaction family.
	 * Therefore the master locker cannot go away while
	 * we manipulate it, nor can another child in the
	 * family be created at the same time.
	 */
	LOCKER_LOCK(lt, region, id, ndx);
	if ((ret = __lock_getlocker(lt, id, ndx, 1, &lockerp)) != 0)
		goto err;

	/* Point to our parent. */
	lockerp->parent_locker = R_OFFSET(&lt->reginfo, mlockerp);

	/* See if this locker is the family master. */
	if (mlockerp->master_locker == INVALID_ROFF)
		lockerp->master_locker = R_OFFSET(&lt->reginfo, mlockerp);
	else {
		lockerp->master_locker = mlockerp->master_locker;
		mlockerp = R_ADDR(&lt->reginfo, mlockerp->master_locker);
	}

	/*
	 * Link the child at the head of the master's list.
	 * The guess is when looking for deadlock that
	 * the most recent child is the one thats blocked.
	 */
	SH_LIST_INSERT_HEAD(
	    &mlockerp->child_locker, lockerp, child_link, __db_locker);

err:	LOCK_SYSTEM_UNLOCK(dbenv);

	return (ret);
}

/*
 * __lock_freefamilylocker
 *	Remove a locker from the hash table and its family.
 *
 * This must be called without the locker bucket locked.
 *
 * PUBLIC: int __lock_freefamilylocker  __P((DB_LOCKTAB *, u_int32_t));
 */
int
__lock_freefamilylocker(lt, locker)
	DB_LOCKTAB *lt;
	u_int32_t locker;
{
	DB_ENV *dbenv;
	DB_LOCKER *sh_locker;
	DB_LOCKREGION *region;
	u_int32_t indx;
	int ret;

	dbenv = lt->dbenv;
	region = lt->reginfo.primary;

	LOCK_SYSTEM_LOCK(dbenv);
	LOCKER_LOCK(lt, region, locker, indx);

	if ((ret = __lock_getlocker(lt,
	    locker, indx, 0, &sh_locker)) != 0 || sh_locker == NULL)
		goto err;

	if (SH_LIST_FIRST(&sh_locker->heldby, __db_lock) != NULL) {
		ret = EINVAL;
		__db_errx(dbenv, "Freeing locker with locks");
		goto err;
	}

	/* If this is part of a family, we must fix up its links. */
	if (sh_locker->master_locker != INVALID_ROFF)
		SH_LIST_REMOVE(sh_locker, child_link, __db_locker);

	__lock_freelocker(lt, region, sh_locker, indx);

err:	LOCK_SYSTEM_UNLOCK(dbenv);
	return (ret);
}

/*
 * __lock_freelocker
 *      Common code for deleting a locker; must be called with the
 *	locker bucket locked.
 *
 * PUBLIC: void __lock_freelocker
 * PUBLIC:    __P((DB_LOCKTAB *, DB_LOCKREGION *, DB_LOCKER *, u_int32_t));
 */
void
__lock_freelocker(lt, region, sh_locker, indx)
	DB_LOCKTAB *lt;
	DB_LOCKREGION *region;
	DB_LOCKER *sh_locker;
	u_int32_t indx;

{
	SH_TAILQ_REMOVE(&lt->locker_tab[indx], sh_locker, links, __db_locker);
	SH_TAILQ_INSERT_HEAD(
	    &region->free_lockers, sh_locker, links, __db_locker);
	SH_TAILQ_REMOVE(&region->lockers, sh_locker, ulinks, __db_locker);
	region->stat.st_nlockers--;
}
