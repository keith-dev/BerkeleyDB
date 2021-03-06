/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: db_am.h,v 12.8 2005/09/28 17:44:24 margo Exp $
 */
#ifndef _DB_AM_H_
#define	_DB_AM_H_

/*
 * IS_ENV_AUTO_COMMIT --
 *	Auto-commit test for enviroment operations: DbEnv::{open,remove,rename}
 */
#define	IS_ENV_AUTO_COMMIT(dbenv, txn, flags)				\
	(LF_ISSET(DB_AUTO_COMMIT) ||					\
	    ((txn) == NULL && F_ISSET((dbenv), DB_ENV_AUTO_COMMIT) &&	\
	    !LF_ISSET(DB_NO_AUTO_COMMIT)))

/*
 * IS_DB_AUTO_COMMIT --
 *	Auto-commit test for database operations.
 */
#define	IS_DB_AUTO_COMMIT(dbp, txn)					\
	    ((txn) == NULL && F_ISSET((dbp), DB_AM_TXN))

/*
 * STRIP_AUTO_COMMIT --
 *	Releases after 4.3 no longer requires DB operations to specify the
 *	AUTO_COMMIT flag, but the API continues to allow it to be specified.
 */
#define	STRIP_AUTO_COMMIT(f)	FLD_CLR((f), DB_AUTO_COMMIT)

/* DB recovery operation codes. */
#define	DB_ADD_DUP	1
#define	DB_REM_DUP	2
#define	DB_ADD_BIG	3
#define	DB_REM_BIG	4

/*
 * Standard initialization and shutdown macros for all recovery functions.
 */
#define	REC_INTRO(func, inc_count, do_cursor) do {			\
	argp = NULL;							\
	file_dbp = NULL;						\
	COMPQUIET(dbc, NULL);						\
	/* mpf isn't used by all of the recovery functions. */		\
	COMPQUIET(mpf, NULL);						\
	if ((ret = func(dbenv, dbtp->data, &argp)) != 0)		\
		goto out;						\
	if ((ret = __dbreg_id_to_db(dbenv, argp->txnid,			\
	    &file_dbp, argp->fileid, inc_count)) != 0) {		\
		if (ret	== DB_DELETED) {				\
			ret = 0;					\
			goto done;					\
		}							\
		goto out;						\
	}								\
	if (do_cursor) {						\
		if ((ret = __db_cursor(file_dbp, NULL, &dbc, 0)) != 0)	\
			goto out;					\
		F_SET(dbc, DBC_RECOVER);				\
	}								\
	mpf = file_dbp->mpf;						\
} while (0)

#define	REC_CLOSE {							\
	int __t_ret;							\
	if (argp != NULL)						\
		__os_free(dbenv, argp);					\
	if (dbc != NULL &&						\
	    (__t_ret = __db_c_close(dbc)) != 0 && ret == 0)		\
		ret = __t_ret;						\
	}								\
	return (ret)

/*
 * No-op versions of the same macros.
 */
#define	REC_NOOP_INTRO(func) do {					\
	argp = NULL;							\
	if ((ret = func(dbenv, dbtp->data, &argp)) != 0)		\
		return (ret);						\
} while (0)
#define	REC_NOOP_CLOSE							\
	if (argp != NULL)						\
		__os_free(dbenv, argp);					\
	return (ret)

/*
 * Macro for reading pages during recovery.  In most cases we
 * want to avoid an error if the page is not found during rollback
 * or if we are using truncate to remove pages from the file.
 */
#ifndef HAVE_FTRUNCATE
#define	REC_FGET(mpf, pgno, pagep, cont)				\
	if ((ret = __memp_fget(mpf, &(pgno), 0, pagep)) != 0) {		\
		if (ret != DB_PAGE_NOTFOUND || DB_REDO(op)) {		\
			ret = __db_pgerr(file_dbp, pgno, ret);		\
			goto out;					\
		} else							\
			goto cont;					\
	}
#else
#define	REC_FGET(mpf, pgno, pagep, cont)				\
	if ((ret = __memp_fget(mpf, &(pgno), 0, pagep)) != 0) {		\
		if (ret != DB_PAGE_NOTFOUND) {				\
			ret = __db_pgerr(file_dbp, pgno, ret);		\
			goto out;					\
		} else							\
			goto cont;					\
	}
#endif

/*
 * Standard debugging macro for all recovery functions.
 */
#ifdef DEBUG_RECOVER
#define	REC_PRINT(func)							\
	(void)func(dbenv, dbtp, lsnp, op, info);
#else
#define	REC_PRINT(func)
#endif

/*
 * Actions to __db_lget
 */
#define	LCK_ALWAYS		1	/* Lock even for off page dup cursors */
#define	LCK_COUPLE		2	/* Lock Couple */
#define	LCK_COUPLE_ALWAYS	3	/* Lock Couple even in txn. */
#define	LCK_DOWNGRADE		4	/* Downgrade the lock. (internal) */
#define	LCK_ROLLBACK		5	/* Lock even if in rollback */

/*
 * If doing transactions we have to hold the locks associated with a data item
 * from a page for the entire transaction.  However, we don't have to hold the
 * locks associated with walking the tree.  Distinguish between the two so that
 * we don't tie up the internal pages of the tree longer than necessary.
 */
#define	__LPUT(dbc, lock)						\
	__ENV_LPUT((dbc)->dbp->dbenv, lock)

#define	__ENV_LPUT(dbenv, lock)						\
	(LOCK_ISSET(lock) ? __lock_put(dbenv, &(lock)) : 0)

/*
 * __TLPUT -- transactional lock put
 *	If the lock is valid then
 *	   If we are not in a transaction put the lock.
 *	   Else if the cursor is doing dirty reads and this was a read then
 *		put the lock.
 *	   Else if the db is supporting dirty reads and this is a write then
 *		downgrade it.
 *	Else do nothing.
 */
#define	__TLPUT(dbc, lock)						\
	(LOCK_ISSET(lock) ? __db_lput(dbc, &(lock)) : 0)

typedef struct {
	DBC *dbc;
	u_int32_t count;
} db_trunc_param;

/*
 * A database should be required to be readonly if it's been explicitly
 * specified as such or if we're a client in a replicated environment and
 * we don't have the special "client-writer" designation.
 */
#define	DB_IS_READONLY(dbp)						\
    (F_ISSET(dbp, DB_AM_RDONLY) ||					\
    (IS_REP_CLIENT((dbp)->dbenv) &&					\
    !F_ISSET((dbp), DB_AM_CL_WRITER)))

/*
 * For portability, primary keys that are record numbers are stored in
 * secondaries in the same byte order as the secondary database.  As a
 * consequence, we need to swap the byte order of these keys before attempting
 * to use them for lookups in the primary.  We also need to swap user-supplied
 * primary keys that are used in secondary lookups (for example, with the
 * DB_GET_BOTH flag on a secondary get).
 */
#include "dbinc/db_swap.h"

#define	SWAP_IF_NEEDED(pdbp, sdbp, pkey)				\
	do {								\
		if (((pdbp)->type == DB_QUEUE ||			\
		    (pdbp)->type == DB_RECNO) &&			\
		    F_ISSET((sdbp), DB_AM_SWAP))			\
			P_32_SWAP((pkey)->data);			\
	} while (0)

#include "dbinc/db_dispatch.h"
#include "dbinc_auto/db_auto.h"
#include "dbinc_auto/crdel_auto.h"
#include "dbinc_auto/db_ext.h"
#endif /* !_DB_AM_H_ */
