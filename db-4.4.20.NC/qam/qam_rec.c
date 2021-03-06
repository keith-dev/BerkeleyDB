/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: qam_rec.c,v 12.8 2005/10/20 18:57:13 bostic Exp $
 */

#include "db_config.h"

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <string.h>
#endif

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_shash.h"
#include "dbinc/db_am.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

/*
 * LSNs in queue data pages are advisory.  They do not have to be accurate
 * as all operations are idempotent on records.  They should not be rolled
 * forward during recovery as committed transaction may obscure updates from
 * an incomplete transaction that updates the same page.  The incomplete
 * transaction may be completed during a later hot backup cycle.
 */

/* Determine if we are restoring prepared transactions from __txn_recover. */
#define	IS_IN_RESTORE(dbenv)						 \
	(((DB_TXNREGION *)((DB_TXNMGR *)				 \
	     (dbenv)->tx_handle)->reginfo.primary)->stat.st_nrestores != 0)

/*
 * __qam_incfirst_recover --
 *	Recovery function for incfirst.
 *
 * PUBLIC: int __qam_incfirst_recover
 * PUBLIC:   __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__qam_incfirst_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__qam_incfirst_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_LOCK lock;
	DB_LSN trunc_lsn;
	DB_MPOOLFILE *mpf;
	QMETA *meta;
	QUEUE_CURSOR *cp;
	db_pgno_t metapg;
	u_int32_t rec_ext;
	int exact, modified, ret, t_ret;

	LOCK_INIT(lock);
	COMPQUIET(meta, NULL);
	REC_PRINT(__qam_incfirst_print);
	REC_INTRO(__qam_incfirst_read, 1, 1);

	metapg = ((QUEUE *)file_dbp->q_internal)->q_meta;

	if ((ret = __db_lget(dbc,
	    LCK_ROLLBACK, metapg,  DB_LOCK_WRITE, 0, &lock)) != 0)
		goto done;
	if ((ret = __memp_fget(mpf, &metapg, 0, &meta)) != 0) {
		if (DB_REDO(op)) {
			if ((ret = __memp_fget(mpf,
			    &metapg, DB_MPOOL_CREATE, &meta)) != 0) {
				(void)__LPUT(dbc, lock);
				goto out;
			}
			meta->dbmeta.pgno = metapg;
			meta->dbmeta.type = P_QAMMETA;
		} else {
			*lsnp = argp->prev_lsn;
			ret = __LPUT(dbc, lock);
			goto out;
		}
	}

	modified = 0;

	/*
	 * Only move first_recno backwards so we pick up the aborted delete.
	 * When going forward we need to be careful since
	 * we may have bumped over a locked record.
	 */
	if (DB_UNDO(op)) {
		if (QAM_BEFORE_FIRST(meta, argp->recno)) {
			meta->first_recno = argp->recno;
			modified = 1;
		}

		trunc_lsn = ((DB_TXNHEAD *)info)->trunc_lsn;
		/* if we are truncating, update the LSN */
		if (!IS_ZERO_LSN(trunc_lsn) &&
		    log_compare(&LSN(meta), &trunc_lsn) > 0) {
			LSN(meta) = trunc_lsn;
			modified = 1;
		}
	} else {
		if (log_compare(&LSN(meta), lsnp) < 0) {
			LSN(meta) = *lsnp;
			modified = 1;
		}
		if (meta->page_ext == 0)
			rec_ext = 0;
		else
			rec_ext = meta->page_ext * meta->rec_page;
		cp = (QUEUE_CURSOR *)dbc->internal;
		if (meta->first_recno == RECNO_OOB)
			meta->first_recno++;
		while (meta->first_recno != meta->cur_recno &&
		    !QAM_BEFORE_FIRST(meta, argp->recno + 1)) {
			if ((ret = __qam_position(dbc,
			    &meta->first_recno, QAM_READ, &exact)) != 0)
				goto err;
			if (cp->page != NULL && (ret =
			    __qam_fput(file_dbp, cp->pgno, cp->page, 0)) != 0)
				goto err;

			if (exact == 1)
				break;
			if (cp->page != NULL &&
			    rec_ext != 0 && meta->first_recno % rec_ext == 0)
				if ((ret =
				    __qam_fremove(file_dbp, cp->pgno)) != 0)
					goto err;
			meta->first_recno++;
			if (meta->first_recno == RECNO_OOB)
				meta->first_recno++;
			modified = 1;
		}
	}

	ret = __memp_fput(mpf, meta, modified ? DB_MPOOL_DIRTY : 0);
	if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	if (ret != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

	if (0) {
err:		(void)__memp_fput(mpf, meta, 0);
		(void)__LPUT(dbc, lock);
	}

out:	REC_CLOSE;
}

/*
 * __qam_mvptr_recover --
 *	Recovery function for mvptr.
 *
 * PUBLIC: int __qam_mvptr_recover
 * PUBLIC:   __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__qam_mvptr_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__qam_mvptr_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_LSN trunc_lsn;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	QMETA *meta;
	QUEUE_CURSOR *cp;
	db_pgno_t metapg;
	int cmp_n, cmp_p, exact, modified, ret;

	REC_PRINT(__qam_mvptr_print);
	REC_INTRO(__qam_mvptr_read, 1, 1);

	metapg = ((QUEUE *)file_dbp->q_internal)->q_meta;

	if ((ret = __db_lget(dbc,
	    LCK_ROLLBACK, metapg,  DB_LOCK_WRITE, 0, &lock)) != 0)
		goto done;
	if ((ret = __memp_fget(mpf, &metapg, 0, &meta)) != 0) {
		if (DB_REDO(op)) {
			if ((ret = __memp_fget(mpf,
			    &metapg, DB_MPOOL_CREATE, &meta)) != 0) {
				(void)__LPUT(dbc, lock);
				goto out;
			}
			meta->dbmeta.pgno = metapg;
			meta->dbmeta.type = P_QAMMETA;
		} else {
			*lsnp = argp->prev_lsn;
			ret = __LPUT(dbc, lock);
			goto out;
		}
	}

	modified = 0;
	cmp_n = log_compare(lsnp, &LSN(meta));
	cmp_p = log_compare(&LSN(meta), &argp->metalsn);

	/*
	 * Under normal circumstances, we never undo a movement of one of
	 * the pointers.  Just move them along regardless of abort/commit.
	 * When going forward we need to verify that this is really where
	 * the pointer belongs.  A transaction may roll back and reinsert
	 * a record that was missing at the time of this action.
	 *
	 * If we're undoing a truncate, we need to reset the pointers to
	 * their state before the truncate.
	 */
	if (DB_UNDO(op)) {
		if ((argp->opcode & QAM_TRUNCATE) && cmp_n <= 0) {
			meta->first_recno = argp->old_first;
			meta->cur_recno = argp->old_cur;
			LSN(meta) = argp->metalsn;
			modified = 1;
		}
		/* If the page lsn is beyond the truncate point, move it back */
		trunc_lsn = ((DB_TXNHEAD *)info)->trunc_lsn;
		if (!IS_ZERO_LSN(trunc_lsn) &&
		    log_compare(&trunc_lsn, &LSN(meta)) < 0) {
			LSN(meta) = argp->metalsn;
			modified = 1;
		}
	} else if (op == DB_TXN_APPLY || cmp_p == 0) {
		cp = (QUEUE_CURSOR *)dbc->internal;
		if ((argp->opcode & QAM_SETFIRST) &&
		    meta->first_recno == argp->old_first) {
			if (argp->old_first > argp->new_first)
				meta->first_recno = argp->new_first;
			else {
				if ((ret = __qam_position(dbc,
				    &meta->first_recno, QAM_READ, &exact)) != 0)
					goto err;
				if (!exact)
					meta->first_recno = argp->new_first;
				if (cp->page != NULL &&
				    (ret = __qam_fput(file_dbp,
				    cp->pgno, cp->page, 0)) != 0)
					goto err;
			}
		}

		if ((argp->opcode & QAM_SETCUR) &&
		    meta->cur_recno == argp->old_cur) {
			if (argp->old_cur < argp->new_cur)
				meta->cur_recno = argp->new_cur;
			else {
				if ((ret = __qam_position(dbc,
				    &meta->cur_recno, QAM_READ, &exact)) != 0)
					goto err;
				if (!exact)
					meta->cur_recno = argp->new_cur;
				if (cp->page != NULL
				    && (ret = __qam_fput(file_dbp,
				    cp->pgno, cp->page, 0)) != 0)
					goto err;
			}
		}

		modified = 1;
		meta->dbmeta.lsn = *lsnp;
	}

	if ((ret = __memp_fput(mpf, meta, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

	if ((ret = __LPUT(dbc, lock)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

	if (0) {
err:		(void)__memp_fput(mpf, meta, 0);
		(void)__LPUT(dbc, lock);
	}

out:	REC_CLOSE;
}

/*
 * __qam_del_recover --
 *	Recovery function for del.
 *		Non-extent version or if there is no data (zero len).
 *
 * PUBLIC: int __qam_del_recover
 * PUBLIC:     __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__qam_del_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__qam_del_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	QAMDATA *qp;
	QMETA *meta;
	QPAGE *pagep;
	db_pgno_t metapg;
	int cmp_n, modified, ret, t_ret;

	COMPQUIET(info, NULL);
	COMPQUIET(pagep, NULL);
	REC_PRINT(__qam_del_print);
	REC_INTRO(__qam_del_read, 1, 1);

	if ((ret = __qam_fget(file_dbp,
	    &argp->pgno, DB_MPOOL_CREATE, &pagep)) != 0)
		goto out;

	modified = 0;
	if (pagep->pgno == PGNO_INVALID) {
		pagep->pgno = argp->pgno;
		pagep->type = P_QAMDATA;
		modified = 1;
	}

	cmp_n = log_compare(lsnp, &LSN(pagep));

	if (DB_UNDO(op)) {
		/* make sure first is behind us */
		metapg = ((QUEUE *)file_dbp->q_internal)->q_meta;
		if ((ret = __db_lget(dbc,
		    LCK_ROLLBACK, metapg, DB_LOCK_WRITE, 0, &lock)) != 0)
			goto err;
		if ((ret = __memp_fget(mpf, &metapg, 0, &meta)) != 0) {
			(void)__LPUT(dbc, lock);
			goto err;
		}
		if (meta->first_recno == RECNO_OOB ||
		    (QAM_BEFORE_FIRST(meta, argp->recno) &&
		    (meta->first_recno <= meta->cur_recno ||
		    meta->first_recno -
		    argp->recno < argp->recno - meta->cur_recno))) {
			meta->first_recno = argp->recno;
			ret = __memp_fput(mpf, meta, DB_MPOOL_DIRTY);
		} else
			ret = __memp_fput(mpf, meta, 0);
		if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;

		/* Need to undo delete - mark the record as present */
		qp = QAM_GET_RECORD(file_dbp, pagep, argp->indx);
		F_SET(qp, QAM_VALID);

		/*
		 * Move the LSN back to this point;  do not move it forward.
		 * Only move it back if we're in recovery.  If we're in
		 * an abort, because we don't hold a page lock, we could
		 * foul up a concurrent put.  Having too late an LSN
		 * is harmless in queue except when we're determining
		 * what we need to roll forward during recovery.  [#2588]
		 * If we are aborting a restored transaction then it
		 * might get rolled forward later so the LSN needs to
		 * be correct in that case too. [#12181]
		 */
		if (cmp_n <= 0 &&
		      (op == DB_TXN_BACKWARD_ROLL || IS_IN_RESTORE(dbenv)))
			LSN(pagep) = argp->lsn;
		modified = 1;
	} else if (op == DB_TXN_APPLY || (cmp_n > 0 && DB_REDO(op))) {
		/* Need to redo delete - clear the valid bit */
		qp = QAM_GET_RECORD(file_dbp, pagep, argp->indx);
		F_CLR(qp, QAM_VALID);
		/*
		 * We only move the LSN forward during replication.
		 * During recovery we could obsucre an update from
		 * a partially completed transaction while processing
		 * a hot backup.  [#13823]
		 */
		if (op == DB_TXN_APPLY)
			LSN(pagep) = *lsnp;
		modified = 1;
	}
	if ((ret = __qam_fput(file_dbp,
	    argp->pgno, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

	if (0) {
err:		(void)__qam_fput(file_dbp, argp->pgno, pagep, 0);
	}
out:	REC_CLOSE;
}

/*
 * __qam_delext_recover --
 *	Recovery function for del in an extent based queue.
 *
 * PUBLIC: int __qam_delext_recover __P((DB_ENV *,
 * PUBLIC:     DBT *, DB_LSN *, db_recops, void *));
 */
int
__qam_delext_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__qam_delext_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_LOCK lock;
	DB_MPOOLFILE *mpf;
	QAMDATA *qp;
	QMETA *meta;
	QPAGE *pagep;
	db_pgno_t metapg;
	int cmp_n, modified, ret, t_ret;

	COMPQUIET(info, NULL);
	COMPQUIET(pagep, NULL);
	REC_PRINT(__qam_delext_print);
	REC_INTRO(__qam_delext_read, 1, 1);

	if ((ret = __qam_fget(file_dbp, &argp->pgno, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND && ret != ENOENT)
			goto out;
		/*
		 * If we are redoing a delete and the page is not there
		 * we are done.
		 */
		if (DB_REDO(op))
			goto done;
		if ((ret = __qam_fget(file_dbp,
		    &argp->pgno, DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	modified = 0;
	if (pagep->pgno == PGNO_INVALID) {
		pagep->pgno = argp->pgno;
		pagep->type = P_QAMDATA;
		modified = 1;
	}

	cmp_n = log_compare(lsnp, &LSN(pagep));

	if (DB_UNDO(op)) {
		/* make sure first is behind us */
		metapg = ((QUEUE *)file_dbp->q_internal)->q_meta;
		if ((ret = __db_lget(dbc,
		    LCK_ROLLBACK, metapg, DB_LOCK_WRITE, 0, &lock)) != 0)
			goto err;
		if ((ret = __memp_fget(mpf, &metapg, 0, &meta)) != 0) {
			(void)__LPUT(dbc, lock);
			goto err;
		}
		if (meta->first_recno == RECNO_OOB ||
		    (QAM_BEFORE_FIRST(meta, argp->recno) &&
		    (meta->first_recno <= meta->cur_recno ||
		    meta->first_recno -
		    argp->recno < argp->recno - meta->cur_recno))) {
			meta->first_recno = argp->recno;
			ret = __memp_fput(mpf, meta, DB_MPOOL_DIRTY);
		} else
			ret = __memp_fput(mpf, meta, 0);
		if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
			ret = t_ret;
		if (ret != 0)
			goto err;

		if ((ret = __qam_pitem(dbc, pagep,
		    argp->indx, argp->recno, &argp->data)) != 0)
			goto err;

		/*
		 * Move the LSN back to this point;  do not move it forward.
		 * Only move it back if we're in recovery.  If we're in
		 * an abort, because we don't hold a page lock, we could
		 * foul up a concurrent put.  Having too late an LSN
		 * is harmless in queue except when we're determining
		 * what we need to roll forward during recovery.  [#2588]
		 */
		if (cmp_n <= 0 &&
		      (op == DB_TXN_BACKWARD_ROLL || IS_IN_RESTORE(dbenv)))
			LSN(pagep) = argp->lsn;
		modified = 1;
	} else if (op == DB_TXN_APPLY || (cmp_n > 0 && DB_REDO(op))) {
		/* Need to redo delete - clear the valid bit */
		qp = QAM_GET_RECORD(file_dbp, pagep, argp->indx);
		F_CLR(qp, QAM_VALID);
		/*
		 * We only move the LSN forward during replication.
		 * During recovery we could obsucre an update from
		 * a partially completed transaction while processing
		 * a hot backup.  [#13823]
		 */
		if (op == DB_TXN_APPLY)
			LSN(pagep) = *lsnp;
		modified = 1;
	}
	if ((ret = __qam_fput(file_dbp,
	    argp->pgno, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

	if (0) {
err:		(void)__qam_fput(file_dbp, argp->pgno, pagep, 0);
	}
out:	REC_CLOSE;
}

/*
 * __qam_add_recover --
 *	Recovery function for add.
 *
 * PUBLIC: int __qam_add_recover
 * PUBLIC:     __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__qam_add_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__qam_add_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	QAMDATA *qp;
	QMETA *meta;
	QPAGE *pagep;
	db_pgno_t metapg;
	int cmp_n, meta_dirty, modified, ret;

	COMPQUIET(info, NULL);
	COMPQUIET(pagep, NULL);
	REC_PRINT(__qam_add_print);
	REC_INTRO(__qam_add_read, 1, 1);

	modified = 0;
	if ((ret = __qam_fget(file_dbp, &argp->pgno, 0, &pagep)) != 0) {
		if (ret != DB_PAGE_NOTFOUND && ret != ENOENT)
			goto out;
		/*
		 * If we are undoing an append and the page is not there
		 * we are done.
		 */
		if (DB_UNDO(op))
			goto done;
		if ((ret = __qam_fget(file_dbp,
		    &argp->pgno, DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	if (pagep->pgno == PGNO_INVALID) {
		pagep->pgno = argp->pgno;
		pagep->type = P_QAMDATA;
		modified = 1;
	}

	cmp_n = log_compare(lsnp, &LSN(pagep));

	if (DB_REDO(op)) {
		/* Fix meta-data page. */
		metapg = ((QUEUE *)file_dbp->q_internal)->q_meta;
		if ((ret = __memp_fget(mpf, &metapg, 0, &meta)) != 0)
			goto err;
		meta_dirty = 0;
		if (QAM_BEFORE_FIRST(meta, argp->recno)) {
			meta->first_recno = argp->recno;
			meta_dirty = 1;
		}
		if (argp->recno == meta->cur_recno ||
		   QAM_AFTER_CURRENT(meta, argp->recno)) {
			meta->cur_recno = argp->recno + 1;
			meta_dirty = 1;
		}
		if ((ret = __memp_fput(mpf,
		    meta, meta_dirty? DB_MPOOL_DIRTY : 0)) != 0)
			goto err;

		/* Now update the actual page if necessary. */
		if (op == DB_TXN_APPLY || cmp_n > 0) {
			/* Need to redo add - put the record on page */
			if ((ret = __qam_pitem(dbc,
			    pagep, argp->indx, argp->recno, &argp->data)) != 0)
				goto err;
			/*
			 * We only move the LSN forward during replication.
			 * During recovery we could obsucre an update from
			 * a partially completed transaction while processing
			 * a hot backup.  [#13823]
			 */
			if (op == DB_TXN_APPLY)
				LSN(pagep) = *lsnp;
			modified = 1;
		}
	} else if (DB_UNDO(op)) {
		/*
		 * Need to undo add
		 *	If this was an overwrite, put old record back.
		 *	Otherwise just clear the valid bit
		 */
		if (argp->olddata.size != 0) {
			if ((ret = __qam_pitem(dbc, pagep,
			    argp->indx, argp->recno, &argp->olddata)) != 0)
				goto err;

			if (!(argp->vflag & QAM_VALID)) {
				qp = QAM_GET_RECORD(
				    file_dbp, pagep, argp->indx);
				F_CLR(qp, QAM_VALID);
			}
			modified = 1;
		} else {
			qp = QAM_GET_RECORD(file_dbp, pagep, argp->indx);
			qp->flags = 0;
			modified = 1;
		}

		/*
		 * Move the LSN back to this point;  do not move it forward.
		 * Only move it back if we're in recovery.  If we're in
		 * an abort, because we don't hold a page lock, we could
		 * foul up a concurrent put.  Having too late an LSN
		 * is harmless in queue except when we're determining
		 * what we need to roll forward during recovery.  [#2588]
		 */
		if (cmp_n <= 0 &&
		      (op == DB_TXN_BACKWARD_ROLL || IS_IN_RESTORE(dbenv)))
			LSN(pagep) = argp->lsn;
	}

	if ((ret = __qam_fput(file_dbp,
	    argp->pgno, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

	if (0) {
err:		(void)__qam_fput(file_dbp, argp->pgno, pagep, 0);
	}

out:	REC_CLOSE;
}
