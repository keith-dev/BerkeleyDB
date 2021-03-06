/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2001
 *	Sleepycat Software.  All rights reserved.
 */

#include "db_config.h"

#ifndef lint
static const char revid[] = "$Id: mutex.c,v 11.23 2001/09/04 02:22:16 bostic Exp $";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <string.h>
#endif

#include "db_int.h"

/*
 * __db_mutex_alloc --
 *	Allocate and initialize a mutex.
 *
 * PUBLIC: int __db_mutex_alloc __P((DB_ENV *, REGINFO *, int, DB_MUTEX **));
 */
int
__db_mutex_alloc(dbenv, infop, is_locked, storep)
	DB_ENV *dbenv;
	REGINFO *infop;
	int is_locked;
	DB_MUTEX **storep;
{
	int ret;

	/*
	 * If the architecture supports mutexes in heap memory, use that
	 * memory.  If it doesn't, we have to allocate space in a region.
	 *
	 * XXX
	 * There's a nasty starvation issue here for applications running
	 * on systems that don't support mutexes in heap memory.  If the
	 * normal state of the entire region is dirty (e.g., mpool), then
	 * we can run out of memory to allocate for mutexes when new files
	 * are opened in the pool.  We're not trying to fix this for now,
	 * because the only known system where we can see this failure at
	 * the moment is HP-UX 10.XX.
	 */
#if	defined(MUTEX_NO_MALLOC_LOCKS) || defined(MUTEX_SYSTEM_RESOURCES)
	if (is_locked == 0)
		R_LOCK(dbenv, infop);
	ret = __db_shalloc(infop->addr, sizeof(DB_MUTEX), MUTEX_ALIGN, storep);
	if (is_locked == 0)
		R_UNLOCK(dbenv, infop);
#else
	COMPQUIET(dbenv, NULL);
	COMPQUIET(infop, NULL);
	COMPQUIET(is_locked, 0);
	ret = __os_calloc(dbenv, 1, sizeof(DB_MUTEX), storep);
#endif
	if (ret != 0)
		__db_err(dbenv, "Unable to allocate memory for mutex");
	return (ret);
}

/*
 * __db_mutex_free --
 *	Free a mutex.
 *
 * PUBLIC: void __db_mutex_free __P((DB_ENV *, REGINFO *, DB_MUTEX *));
 */
void
__db_mutex_free(dbenv, infop, mutexp)
	DB_ENV *dbenv;
	REGINFO *infop;
	DB_MUTEX *mutexp;
{
#if	defined(MUTEX_SYSTEM_RESOURCES)
	if (F_ISSET(mutexp, MUTEX_INITED))
		__db_shlocks_clear(mutexp, infop, NULL);
#endif

#if	defined(MUTEX_NO_MALLOC_LOCKS) || defined(MUTEX_SYSTEM_RESOURCES)
	R_LOCK(dbenv, infop);
	__db_shalloc_free(infop->addr, mutexp);
	R_UNLOCK(dbenv, infop);
#else
	COMPQUIET(dbenv, NULL);
	COMPQUIET(infop, NULL);
	__os_free(dbenv, mutexp, sizeof(*mutexp));
#endif
}

#ifdef MUTEX_SYSTEM_RESOURCES
/*
 * __db_shreg_locks_record --
 *	Record an entry in the shared locks area.
 *	Region lock must be held in caller.
 */
static int
__db_shreg_locks_record(dbenv, mutexp, infop, rp)
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	REGINFO *infop;
	REGMAINT *rp;
{
	u_int i;

	if (!F_ISSET(mutexp, MUTEX_INITED))
		return (0);
	DB_ASSERT(mutexp->reg_off == INVALID_ROFF);
	rp->stat.st_records++;
	i = (roff_t *)R_ADDR(infop, rp->regmutex_hint) - &rp->regmutexes[0];
	if (rp->regmutexes[i] != INVALID_ROFF) {
		/*
		 * Our hint failed, search for an open slot.
		 */
		rp->stat.st_hint_miss++;
		for (i = 0; i < rp->reglocks; i++)
			if (rp->regmutexes[i] == INVALID_ROFF)
				break;
		if (i == rp->reglocks) {
			rp->stat.st_max_locks++;
			__db_err(dbenv,
			    "Region mutexes: Exceeded maximum lock slots %lu",
			    (u_long)rp->reglocks);
			return (ENOMEM);
		}
	} else
		rp->stat.st_hint_hit++;
	/*
	 * When we get here, i is an empty slot.  Record this
	 * mutex, set hint to point to the next slot and we are done.
	 */
	rp->regmutexes[i] = R_OFFSET(infop, mutexp);
	mutexp->reg_off = R_OFFSET(infop, &rp->regmutexes[i]);
	rp->regmutex_hint = (i < rp->reglocks - 1) ?
	    R_OFFSET(infop, &rp->regmutexes[i+1]) :
	    R_OFFSET(infop, &rp->regmutexes[0]);
	return (0);
}

/*
 * __db_shreg_locks_clear --
 *	Erase an entry in the shared locks area.
 *	Region lock must be held in caller.
 *
 * PUBLIC: void __db_shreg_locks_clear __P((DB_MUTEX *, REGINFO *, REGMAINT *));
 */
void
__db_shreg_locks_clear(mutexp, infop, rp)
	DB_MUTEX *mutexp;
	REGINFO *infop;
	REGMAINT *rp;
{
	if (!F_ISSET(mutexp, MUTEX_INITED))
		return;
	/*
	 * This function is generally only called on a forcible
	 * remove of an environment.  We recorded our index in
	 * the mutex.  Find it and clear it.
	 */
	DB_ASSERT(mutexp->reg_off != INVALID_ROFF);
	DB_ASSERT(*(roff_t *)R_ADDR(infop, mutexp->reg_off) == \
	    R_OFFSET(infop, mutexp));
	*(roff_t *)R_ADDR(infop, mutexp->reg_off) = 0;
	if (rp != NULL) {
		rp->regmutex_hint = mutexp->reg_off;
		rp->stat.st_clears++;
	}
	mutexp->reg_off = INVALID_ROFF;
	__db_mutex_destroy(mutexp);
}

/*
 * __db_shreg_locks_destroy --
 *	Destroy all mutexes in a region's range.
 *
 * PUBLIC: void __db_shreg_locks_destroy __P((REGINFO *, REGMAINT *));
 */
void
__db_shreg_locks_destroy(infop, rp)
	REGINFO *infop;
	REGMAINT *rp;
{
	u_int32_t i;

	/*
	 * Go through the list of all mutexes and destroy them.
	 */
	for (i = 0; i < rp->reglocks; i++)
		if (rp->regmutexes[i] != 0) {
			rp->stat.st_destroys++;
			__db_mutex_destroy((DB_MUTEX *)R_ADDR(infop,
			    rp->regmutexes[i]));
		}
}

/*
 * __db_shreg_mutex_init --
 *	Initialize a shared memory mutex.
 *
 * PUBLIC: int __db_shreg_mutex_init __P((DB_ENV *, DB_MUTEX *, u_int32_t,
 * PUBLIC:    u_int32_t, REGINFO *, REGMAINT *));
 */
int
__db_shreg_mutex_init(dbenv, mutexp, offset, flags, infop, rp)
	DB_ENV *dbenv;
	DB_MUTEX *mutexp;
	u_int32_t offset;
	u_int32_t flags;
	REGINFO *infop;
	REGMAINT *rp;
{
	int ret;

	if ((ret = __db_mutex_init(dbenv, mutexp, offset, flags)) != 0)
		return (ret);
	/*
	 * !!!
	 * Since __db_mutex_init is a macro, we may not be
	 * using the 'offset' as it is only used for one type
	 * of mutex.  We COMPQUIET it here, after the call above.
	 */
	COMPQUIET(offset, 0);
	ret = __db_shreg_locks_record(dbenv, mutexp, infop, rp);

	/*
	 * If we couldn't record it and we are returning an error,
	 * we need to destroy the mutex we just created.
	 */
	if (ret)
		__db_mutex_destroy(mutexp);

	return (ret);
}

/*
 * __db_shreg_maintinit --
 *	Initialize a region's maintenance information.
 *
 * PUBLIC: void __db_shreg_maintinit __P((REGINFO *, void *addr, size_t));
 */
void
__db_shreg_maintinit(infop, addr, size)
	REGINFO *infop;
	void *addr;
	size_t size;
{
	REGMAINT *rp;
	u_int32_t i;

	rp = (REGMAINT *)addr;
	memset(addr, 0, sizeof(REGMAINT));
	rp->reglocks = size / sizeof(roff_t);
	rp->regmutex_hint = R_OFFSET(infop, &rp->regmutexes[0]);
	for (i = 0; i < rp->reglocks; i++)
		rp->regmutexes[i] = INVALID_ROFF;
}
#endif /* MUTEX_SYSTEM_RESOURCES */
