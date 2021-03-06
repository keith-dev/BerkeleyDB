/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1999-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: bt_compact.c,v 12.53 2006/08/24 14:44:43 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/txn.h"

static int __bam_compact_dups __P((DBC *,
     PAGE **, u_int32_t, int, DB_COMPACT *, int *));
static int __bam_compact_int __P((DBC *,
     DBT *, DBT *, u_int32_t, int *, DB_COMPACT *, int *));
static int __bam_csearch __P((DBC *, DBT *, u_int32_t, int));
static int __bam_merge __P((DBC *,
     DBC *,  u_int32_t, DBT *, DB_COMPACT *,int *));
static int __bam_merge_internal __P((DBC *, DBC *, int, DB_COMPACT *, int *));
static int __bam_merge_pages __P((DBC *, DBC *, DB_COMPACT *));
static int __bam_merge_records __P((DBC *, DBC*,  u_int32_t, DB_COMPACT *));
static int __bam_truncate_internal_overflow __P((DBC *, PAGE *, DB_COMPACT *));
static int __bam_truncate_overflow __P((DBC *,
     db_pgno_t, db_pgno_t, DB_COMPACT *));
static int __bam_truncate_page __P((DBC *, PAGE **, int));
static int __bam_truncate_root_page __P((DBC *,
     PAGE *, u_int32_t, DB_COMPACT *));

#ifdef HAVE_FTRUNCATE
static int __bam_free_freelist __P((DB *, DB_TXN *));
static int __bam_savekey __P((DBC *, int, DBT *));
static int __bam_setup_freelist __P((DB *, struct pglist *, u_int32_t));
static int __bam_truncate_internal __P((DB *, DB_TXN *, DB_COMPACT *));
#endif

#define	SAVE_START							\
	do {								\
		save_data = *c_data;					\
		ret = __db_retcopy(dbenv,				\
		     &save_start, end->data, end->size,			\
		     &save_start.data, &save_start.ulen);		\
	} while (0)

/*
 * Only restore those things that are negated by aborting the
 * transaction.  We don't restore the number of deadlocks, for example.
 */

#define	RESTORE_START							\
	do {								\
		c_data->compact_pages_free =				\
		      save_data.compact_pages_free;			\
		c_data->compact_levels = save_data.compact_levels;	\
		c_data->compact_truncate = save_data.compact_truncate;	\
		ret = __db_retcopy(dbenv, end,				\
		     save_start.data, save_start.size,			\
		     &end->data, &end->ulen);				\
	} while (0)
/*
 * __bam_compact -- compact a btree.
 *
 * PUBLIC: int __bam_compact __P((DB *, DB_TXN *,
 * PUBLIC:     DBT *, DBT *, DB_COMPACT *, u_int32_t, DBT *));
 */
int
__bam_compact(dbp, txn, start, stop, c_data, flags, end)
	DB *dbp;
	DB_TXN *txn;
	DBT *start, *stop;
	DB_COMPACT *c_data;
	u_int32_t flags;
	DBT *end;
{
	DBT current, save_start;
	DBC *dbc;
	DB_COMPACT save_data;
	DB_ENV *dbenv;
	db_pgno_t last_pgno;
	struct pglist *list;
	u_int32_t factor, nelems, truncated;
	int deadlock, done, ret, span, t_ret, txn_local;

	dbenv = dbp->dbenv;

	memset(&current, 0, sizeof(current));
	memset(&save_start, 0, sizeof(save_start));
	dbc = NULL;
	deadlock = 0;
	done = 0;
	factor = 0;
	ret = 0;
	span = 0;
	truncated = 0;
	last_pgno = 0;

	/*
	 * We pass "end" to the internal routine, indicating where
	 * that routine should begin its work and expecting that it
	 * will return to us the last key that it processed.
	 */
	if (end == NULL)
		end = &current;
	if (start != NULL && (ret = __db_retcopy(dbenv,
	     end, start->data, start->size, &end->data, &end->ulen)) != 0)
		return (ret);

	list = NULL;
	nelems = 0;

	if (IS_DB_AUTO_COMMIT(dbp, txn))
		txn_local = 1;
	else
		txn_local = 0;
	if (!LF_ISSET(DB_FREE_SPACE | DB_FREELIST_ONLY))
		goto no_free;
	if (LF_ISSET(DB_FREELIST_ONLY))
		LF_SET(DB_FREE_SPACE);

#ifdef HAVE_FTRUNCATE
	/* Sort the freelist and set up the in-memory list representation. */
	if (txn_local && (ret = __txn_begin(dbenv, NULL, &txn, 0)) != 0)
		goto err;

	if ((ret = __db_free_truncate(dbp,
	     txn, flags, c_data, &list, &nelems, &last_pgno)) != 0) {
		LF_CLR(DB_FREE_SPACE);
		goto terr;
	}

	/* If the freelist is empty and we are not filling, get out. */
	if (nelems == 0 && LF_ISSET(DB_FREELIST_ONLY)) {
		ret = 0;
		LF_CLR(DB_FREE_SPACE);
		goto terr;
	}
	if ((ret = __bam_setup_freelist(dbp, list, nelems)) != 0) {
		/* Someone else owns the free list. */
		if (ret == EBUSY)
			ret = 0;
	}

	/* Commit the txn and release the meta page lock. */
terr:	if (txn_local) {
		if ((t_ret = __txn_commit(txn, DB_TXN_NOSYNC)) != 0 && ret == 0)
			ret = t_ret;
		txn = NULL;
	}
	if (ret != 0)
		goto err;

	/* Save the number truncated so far, we will add what we get below. */
	truncated = c_data->compact_pages_truncated;
	if (LF_ISSET(DB_FREELIST_ONLY))
		goto done;
#endif

	/*
	 * We want factor to be the target number of free bytes on each page,
	 * so we know when to stop adding items to a page.   Make sure to
	 * subtract the page overhead when computing this target.  This can
	 * result in a 1-2% error on the smallest page.
	 * First figure out how many bytes we should use:
	 */
no_free:
	factor = dbp->pgsize - SIZEOF_PAGE;
	if (c_data->compact_fillpercent != 0) {
		factor *= c_data->compact_fillpercent;
		factor /= 100;
	}
	/* Now convert to the number of free bytes to target. */
	factor = (dbp->pgsize - SIZEOF_PAGE) - factor;

	if (c_data->compact_pages == 0)
		c_data->compact_pages = DB_MAX_PAGES;

	do {
		deadlock = 0;

		SAVE_START;
		if (ret != 0)
			break;

		if (txn_local) {
			if ((ret = __txn_begin(dbenv, NULL, &txn, 0)) != 0)
				break;

			if (c_data->compact_timeout != 0 &&
			    (ret = __txn_set_timeout(txn,
			    c_data->compact_timeout, DB_SET_LOCK_TIMEOUT)) != 0)
				goto err;
		}

		if ((ret = __db_cursor(dbp, txn, &dbc, 0)) != 0)
			goto err;

		if ((ret = __bam_compact_int(dbc, end, stop, factor,
		     &span, c_data, &done)) == DB_LOCK_DEADLOCK && txn_local) {
			/*
			 * We retry on deadlock.  Cancel the statistics
			 * and reset the start point to before this
			 * iteration.
			 */
			deadlock = 1;
			c_data->compact_deadlock++;
			RESTORE_START;
		}

		if ((t_ret = __db_c_close(dbc)) != 0 && ret == 0)
			ret = t_ret;

err:		if (txn_local && txn != NULL) {
			if (ret == 0 && deadlock == 0)
				ret = __txn_commit(txn, DB_TXN_NOSYNC);
			else if ((t_ret = __txn_abort(txn)) != 0 && ret == 0)
				ret = t_ret;
			txn = NULL;
		}
	} while (ret == 0 && !done);

	if (current.data != NULL)
		__os_free(dbenv, current.data);
	if (save_start.data != NULL)
		__os_free(dbenv, save_start.data);

#ifdef HAVE_FTRUNCATE
	/*
	 * Finish up truncation work.  If there are pages left in the free
	 * list then search the internal nodes of the tree as we may have
	 * missed some while walking the leaf nodes.  Then calculate how
	 * many pages we have truncated and release the in-memory free list.
	 */
done:	if (LF_ISSET(DB_FREE_SPACE)) {
		DBMETA *meta;
		db_pgno_t pgno;

		pgno = PGNO_BASE_MD;
		done = 1;
		if (ret == 0 && !LF_ISSET(DB_FREELIST_ONLY) && (t_ret =
		    __memp_fget(dbp->mpf, &pgno, txn, 0, &meta)) == 0) {
			done = meta->free == PGNO_INVALID;
			ret = __memp_fput(dbp->mpf, meta, 0);
		}

		if (!done)
			ret = __bam_truncate_internal(dbp, txn, c_data);

		/* Clean up the free list. */
		if (list != NULL)
			__os_free(dbenv, list);

		if ((t_ret =
		    __memp_fget(dbp->mpf, &pgno, txn, 0, &meta)) == 0) {
			c_data->compact_pages_truncated =
			    truncated + last_pgno - meta->last_pgno;
			if ((t_ret =
			    __memp_fput(dbp->mpf, meta, 0)) != 0 && ret == 0)
				ret = t_ret;
		} else if (ret == 0)
			ret = t_ret;

		if ((t_ret = __bam_free_freelist(dbp, txn)) != 0 && ret == 0)
			t_ret = ret;
	}
#endif

	return (ret);
}

/*
 * __bam_csearch -- isolate search code for bam_compact.
 * This routine hides the differences between searching
 * a BTREE and a RECNO from the rest of the code.
 */
#define	CS_READ	0	/* We are just reading. */
#define	CS_PARENT	1	/* We want the parent too, write lock. */
#define	CS_NEXT		2	/* Get the next page. */
#define	CS_NEXT_WRITE	3	/* Get the next page and write lock. */
#define	CS_DEL		4	/* Get a stack to delete a page. */
#define	CS_START	5	/* Starting level for stack, write lock. */
#define	CS_GETRECNO     0x80	/* Extract record number from start. */

static int
__bam_csearch(dbc, start, sflag, level)
	DBC *dbc;
	DBT *start;
	u_int32_t sflag;
	int level;
{
	BTREE_CURSOR *cp;
	int not_used, ret;

	cp = (BTREE_CURSOR *)dbc->internal;

	if (dbc->dbtype == DB_RECNO) {
		/* If GETRECNO is not set the cp->recno is what we want. */
		if (FLD_ISSET(sflag, CS_GETRECNO)) {
			if (start == NULL || start->size == 0)
				cp->recno = 1;
			else if ((ret =
			     __ram_getno(dbc, start, &cp->recno, 0)) != 0)
				return (ret);
			FLD_CLR(sflag, CS_GETRECNO);
		}
		switch (sflag) {
		case CS_READ:
			sflag = SR_READ;
			break;
		case CS_NEXT:
			sflag = SR_PARENT | SR_READ;
			break;
		case CS_START:
			level = LEAFLEVEL;
			/* FALLTHROUGH */
		case CS_DEL:
		case CS_NEXT_WRITE:
			sflag = SR_STACK;
			break;
		case CS_PARENT:
			sflag = SR_PARENT | SR_WRITE;
			break;
		default:
			return (__db_panic(dbc->dbp->dbenv, EINVAL));
		}
		if ((ret = __bam_rsearch(dbc,
		     &cp->recno, sflag, level, &not_used)) != 0)
			return (ret);
		/* Reset the cursor's recno to the beginning of the page. */
		cp->recno -= cp->csp->indx;
	} else {
		FLD_CLR(sflag, CS_GETRECNO);
		switch (sflag) {
		case CS_READ:
			sflag = SR_READ | SR_DUPFIRST;
			break;
		case CS_DEL:
			sflag = SR_DEL;
			break;
		case CS_NEXT:
			sflag = SR_NEXT;
			break;
		case CS_NEXT_WRITE:
			sflag = SR_NEXT | SR_WRITE;
			break;
		case CS_START:
			sflag = SR_START | SR_WRITE;
			break;
		case CS_PARENT:
			sflag = SR_PARENT | SR_WRITE;
			break;
		default:
			return (__db_panic(dbc->dbp->dbenv, EINVAL));
		}
		if (start == NULL || start->size == 0)
			FLD_SET(sflag, SR_MIN);

		if ((ret = __bam_search(dbc,
		     cp->root, start, sflag, level, NULL, &not_used)) != 0)
			return (ret);
	}

	return (0);
}

/*
 * __bam_compact_int -- internal compaction routine.
 *	Called either with a cursor on the main database
 * or a cursor initialized to the root of an off page duplicate
 * tree.
 */
static int
__bam_compact_int(dbc, start, stop, factor, spanp, c_data, donep)
	DBC *dbc;
	DBT *start, *stop;
	u_int32_t factor;
	int *spanp;
	DB_COMPACT *c_data;
	int *donep;
{
	BTREE_CURSOR *cp, *ncp;
	DB *dbp;
	DBC *ndbc;
	DB_ENV *dbenv;
	DB_LOCK nolock;
	EPG *epg;
	DB_MPOOLFILE *dbmp;
	PAGE *pg, *ppg, *npg;
	db_pgno_t npgno;
	db_recno_t next_recno;
	u_int32_t sflag;
	int check_dups, check_trunc, done, level;
	int merged, nentry, next_page, pgs_done, ret, t_ret, tdone;

#ifdef	DEBUG
#define	CTRACE(dbc, location, t, start, f) do {				\
		DBT __trace;						\
		DB_SET_DBT(__trace, t, strlen(t));			\
		DEBUG_LWRITE(						\
		    dbc, (dbc)->txn, location, &__trace, start, f)	\
	} while (0)
#define	PTRACE(dbc, location, p, start, f) do {				\
		char __buf[32];						\
		(void)snprintf(__buf,					\
		    sizeof(__buf), "pgno: %lu", (u_long)p);		\
		CTRACE(dbc, location, __buf, start, f);			\
	} while (0)
#else
#define	CTRACE(dbc, location, t, start, f)
#define	PTRACE(dbc, location, p, start, f)
#endif

	ndbc = NULL;
	pg = NULL;
	npg = NULL;
	done = 0;
	tdone = 0;
	pgs_done = 0;
	next_recno = 0;
	next_page = 0;
	LOCK_INIT(nolock);
	check_trunc = c_data->compact_truncate != PGNO_INVALID;
	check_dups = (!F_ISSET(dbc, DBC_OPD) &&
	     F_ISSET(dbc->dbp, DB_AM_DUP)) || check_trunc;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	dbmp = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;

	/* Search down the tree for the starting point. */
	if ((ret = __bam_csearch(dbc,
	    start, CS_READ | CS_GETRECNO, LEAFLEVEL)) != 0) {
		/* Its not an error to compact an empty db. */
		if (ret == DB_NOTFOUND)
			ret = 0;
		done = 1;
		goto err;
	}

	/*
	 * Get the first leaf page. The loop below will change pg so
	 * we clear the stack reference so we don't put a a page twice.
	 */
	pg = cp->csp->page;
	cp->csp->page = NULL;
	next_recno = cp->recno;
next:	/*
	 * This is the start of the main compaction loop.  There are 3
	 * parts to the process:
	 * 1) Walk the leaf pages of the tree looking for a page to
	 *	process.  We do this with read locks.  Save the
	 *	key from the page and release it.
	 * 2) Set up a cursor stack which will write lock the page
	 *	and enough of its ancestors to get the job done.
	 *	This could go to the root if we might delete a subtree
	 *	or we have record numbers to update.
	 * 3) Loop fetching pages after the above page and move enough
	 *	data to fill it.
	 * We exit the loop if we are at the end of the leaf pages, are
	 * about to lock a new subtree (we span) or on error.
	 */

	/* Walk the pages looking for something to fill up. */
	while ((npgno = NEXT_PGNO(pg)) != PGNO_INVALID) {
		c_data->compact_pages_examine++;
		PTRACE(dbc, "Next", PGNO(pg), start, 0);

		/* If we have fetched the next page, get the new key. */
		if (next_page == 1 &&
		    dbc->dbtype != DB_RECNO && NUM_ENT(pg) != 0) {
			if ((ret = __db_ret(dbp, dbc->txn, pg,
			     0, start, &start->data, &start->ulen)) != 0)
				goto err;
		}
		next_recno += NUM_ENT(pg);
		if (P_FREESPACE(dbp, pg) > factor ||
		     (check_trunc && PGNO(pg) > c_data->compact_truncate))
			break;
		/*
		 * The page does not need more data or to be swapped,
		 * check to see if we want to look at possible duplicate
		 * trees or overflow records and the move on to the next page.
		 */
		cp->recno += NUM_ENT(pg);
		next_page = 1;
		tdone = pgs_done;
		PTRACE(dbc, "Dups", PGNO(pg), start, 0);
		if (check_dups && (ret = __bam_compact_dups(
		     dbc, &pg, factor, 0, c_data, &pgs_done)) != 0)
			goto err;
		npgno = NEXT_PGNO(pg);
		if ((ret = __memp_fput(dbmp, pg, 0)) != 0)
			goto err;
		pg = NULL;
		/*
		 * If we don't do anything we don't need to hold
		 * the lock on the previous page, so couple always.
		 */
		if ((ret = __db_lget(dbc,
		    tdone == pgs_done ? LCK_COUPLE_ALWAYS : LCK_COUPLE,
		    npgno, DB_LOCK_READ, 0, &cp->csp->lock)) != 0)
			goto err;
		if ((ret = __memp_fget(dbmp, &npgno, dbc->txn, 0, &pg)) != 0)
			goto err;
	}

	/*
	 * When we get here we have 3 cases:
	 * 1) We've reached the end of the leaf linked list and are done.
	 * 2) A page whose freespace exceeds our target and therefore needs
	 *	to have data added to it.
	 * 3) A page that doesn't have too much free space but needs to be
	 *	checked for truncation.
	 * In both cases 2 and 3, we need that page's first key or record
	 * number.  We may already have it, if not get it here.
	 */
	if ((nentry = NUM_ENT(pg)) != 0) {
		next_page = 0;
		/* Get a copy of the first recno on the page. */
		if (dbc->dbtype == DB_RECNO) {
			if ((ret = __db_retcopy(dbp->dbenv, start,
			     &cp->recno, sizeof(cp->recno),
			     &start->data, &start->ulen)) != 0)
				goto err;
		} else if (start->size == 0 &&
		     (ret = __db_ret(dbp, dbc->txn, pg,
		     0, start, &start->data, &start->ulen)) != 0)
			goto err;

		if (npgno == PGNO_INVALID) {
			/* End of the tree, check its duplicates and exit. */
			PTRACE(dbc, "GoDone", PGNO(pg), start, 0);
			if (check_dups && (ret = __bam_compact_dups(dbc,
			   &pg, factor, 0, c_data, &pgs_done)) != 0)
				goto err;
			c_data->compact_pages_examine++;
			done = 1;
			goto done;
		}
	}

	/* Release the page so we don't deadlock getting its parent. */
	if ((ret = __LPUT(dbc, cp->csp->lock)) != 0)
		goto err;
	if ((ret = __memp_fput(dbmp, pg, 0)) != 0)
		goto err;
	BT_STK_CLR(cp);
	pg = NULL;

	/*
	 * Setup the cursor stack. There are 3 cases:
	 * 1) the page is empty and will be deleted: nentry == 0.
	 * 2) the next page has the same parent: *spanp == 0.
	 * 3) the next page has a different parent: *spanp == 1.
	 *
	 * We now need to search the tree again, getting a write lock
	 * on the page we are going to merge or delete.  We do this by
	 * searching down the tree and locking as much of the subtree
	 * above the page as needed.  In the case of a delete we will
	 * find the maximal subtree that can be deleted. In the case
	 * of merge if the current page and the next page are siblings
	 * with the same parent then we only need to lock the parent.
	 * Otherwise *span will be set and we need to search to find the
	 * lowest common ancestor.  Dbc will be set to contain the subtree
	 * containing the page to be merged or deleted. Ndbc will contain
	 * the minimal subtree containing that page and its next sibling.
	 * In all cases for DB_RECNO we simplify things and get the whole
	 * tree if we need more than a single parent.
	 */

	/* Case 1 -- page is empty. */
	if (nentry == 0) {
		CTRACE(dbc, "Empty", "", start, 0);
		if (next_page == 1)
			sflag = CS_NEXT_WRITE;
		else
			sflag = CS_DEL;
		if ((ret = __bam_csearch(dbc, start, sflag, LEAFLEVEL)) != 0)
			goto err;

		pg = cp->csp->page;
		/* Check to see if the page is still empty. */
		if (NUM_ENT(pg) != 0)
			npgno = PGNO(pg);
		else {
			npgno = NEXT_PGNO(pg);
			/* If this is now the root, we are very done. */
			if (PGNO(pg) == cp->root)
				done = 1;
			else {
				if ((ret = __bam_dpages(dbc, 0, 0)) != 0)
					goto err;
				c_data->compact_pages_free++;
				goto next_no_release;
			}
		}
		goto next_page;
	}

	/* case 3 -- different parents. */
	if (*spanp) {
		CTRACE(dbc, "Span", "", start, 0);
		if (ndbc == NULL && (ret = __db_c_dup(dbc, &ndbc, 0)) != 0)
			goto err;
		ncp = (BTREE_CURSOR *)ndbc->internal;
		ncp->recno = next_recno;
		/*
		 * Search the tree looking for the next page after the
		 * current key.  For RECNO get the whole stack.
		 * For BTREE the return will contain the stack that
		 * dominates both the current and next pages.
		 */
		if ((ret = __bam_csearch(ndbc, start, CS_NEXT_WRITE, 0)) != 0)
			goto err;

		if (dbc->dbtype == DB_RECNO) {
			/*
			 * The record we are looking for may have moved
			 * to the previous page.  This page should
			 * be at the beginning of its parent.
			 * If not, then start over.
			 */
			if (ncp->csp[-1].indx != 0) {
				*spanp = 0;
				goto deleted;
			}

		}
		if ((ret =
		    __memp_dirty(dbp->mpf, &ncp->csp->page, dbc->txn, 0)) != 0)
			goto err;
		PTRACE(dbc, "SDups", PGNO(ncp->csp->page), start, 0);
		if (check_dups && (ret = __bam_compact_dups(ndbc,
		     &ncp->csp->page, factor, 1, c_data, &pgs_done)) != 0)
			goto err;

		/* Check to see if the tree collapsed. */
		if (PGNO(ncp->csp->page) == ncp->root)
			goto done;
		/*
		 * We need the stacks to be the same height
		 * so that we can merge parents.
		 */
		level = LEVEL(ncp->sp->page);
		sflag = CS_START;
		if ((ret = __bam_csearch(dbc, start, sflag, level)) != 0)
			goto err;
		pg = cp->csp->page;
		*spanp = 0;

		/*
		 * The page may have emptied while we waited for the lock.
		 * Reset npgno so we re-get this page when we go back to the
		 * top.
		 */
		if (NUM_ENT(pg) == 0) {
			npgno = PGNO(pg);
			goto next_page;
		}
		if (check_trunc && PGNO(pg) > c_data->compact_truncate) {
			pgs_done++;
			/* Get a fresh low numbered page. */
			if ((ret = __bam_truncate_page(dbc, &pg, 1)) != 0)
				goto err1;
		}

		if ((ret =
		    __memp_dirty(dbp->mpf, &cp->csp->page, dbc->txn, 0)) != 0)
			goto err1;
		pg = cp->csp->page;
		npgno = NEXT_PGNO(pg);
		PTRACE(dbc, "SDups", PGNO(pg), start, 0);
		if (check_dups && (ret =
		     __bam_compact_dups(dbc, &cp->csp->page,
		     factor, 1, c_data, &pgs_done)) != 0)
			goto err1;

		/*
		 * We may have dropped our locks, check again
		 * to see if we still need to fill this page and
		 * we are in a spanning situation.
		 */

		if (P_FREESPACE(dbp, pg) <= factor ||
		     cp->csp[-1].indx != NUM_ENT(cp->csp[-1].page) - 1)
			goto next_page;

		/*
		 * Try to move things into a single parent.
		 */
		merged = 0;
		for (epg = cp->sp; epg != cp->csp; epg++) {
			if (PGNO(epg->page) == cp->root)
				continue;
			PTRACE(dbc, "PMerge", PGNO(epg->page), start, 0);
			if ((ret = __bam_merge_internal(dbc,
			       ndbc, LEVEL(epg->page), c_data, &merged)) != 0)
				goto err1;
			if (merged)
				break;
		}

		/* If we merged the parent, then we nolonger span. */
		if (merged) {
			pgs_done++;
			if (cp->csp->page == NULL)
				goto deleted;
			npgno = PGNO(pg);
			goto next_page;
		}
		PTRACE(dbc, "SMerge", PGNO(cp->csp->page), start, 0);
		npgno = NEXT_PGNO(ncp->csp->page);
		if ((ret = __bam_merge(dbc,
		     ndbc, factor, stop, c_data, &done)) != 0)
			goto err1;
		pgs_done++;
		/*
		 * __bam_merge could have freed our stack if it
		 * deleted a page possibly collapsing the tree.
		 */
		if (cp->csp->page == NULL)
			goto deleted;
		cp->recno += NUM_ENT(pg);

		/* If we did not bump to the next page something did not fit. */
		if (npgno != NEXT_PGNO(pg)) {
			npgno = NEXT_PGNO(pg);
			goto next_page;
		}
	} else {
		/* Case 2 -- same parents. */
		CTRACE(dbc, "Sib", "", start, 0);
		if ((ret =
		    __bam_csearch(dbc, start, CS_PARENT, LEAFLEVEL)) != 0)
			goto err;

		pg = cp->csp->page;
		npgno = PGNO(pg);

		/* We now have a write lock, recheck the page. */
		if ((nentry = NUM_ENT(pg)) == 0)
			goto next_page;

		if ((ret = __memp_dirty(dbp->mpf, &cp->csp->page,
		    dbc->txn, 0)) != 0)
			goto err;
		pg = cp->csp->page;

		npgno = NEXT_PGNO(pg);

		/* Check duplicate trees, we have a write lock on the page. */
		PTRACE(dbc, "SibDup", PGNO(pg), start, 0);
		if (check_dups && (ret =
		     __bam_compact_dups(dbc, &cp->csp->page,
		     factor, 1, c_data, &pgs_done)) != 0)
			goto err1;
		pg = cp->csp->page;

		/* Check to see if the tree collapsed. */
		if (PGNO(pg) == cp->root)
			goto err1;
		DB_ASSERT(dbenv, cp->csp - cp->sp == 1);

		if (check_trunc && PGNO(pg) > c_data->compact_truncate) {
			pgs_done++;
			/* Get a fresh low numbered page. */
			if ((ret = __bam_truncate_page(dbc, &pg, 1)) != 0)
				goto err1;
		}

		/* After re-locking check to see if we still need to fill. */
		if (P_FREESPACE(dbp, pg) <= factor)
			goto next_page;

		/* If they have the same parent, just dup the cursor */
		if (ndbc != NULL && (ret = __db_c_close(ndbc)) != 0)
			goto err1;
		if ((ret = __db_c_dup(dbc, &ndbc, DB_POSITION)) != 0)
			goto err1;
		ncp = (BTREE_CURSOR *)ndbc->internal;

		/*
		 * ncp->recno needs to have the recno of the next page.
		 * Bump it by the number of records on the current page.
		 */
		ncp->recno += NUM_ENT(pg);
	}

	/* Fetch pages until we fill this one. */
	while (!done && npgno != PGNO_INVALID &&
	     P_FREESPACE(dbp, pg) > factor && c_data->compact_pages != 0) {
		/*
		 * If our current position is the last one on a parent
		 * page, then we are about to merge across different
		 * internal nodes.  Thus, we need to lock higher up
		 * in the tree.  We will exit the routine and commit
		 * what we have done so far.  Set spanp so we know
		 * we are in this case when we come back.
		 */
		if (cp->csp[-1].indx == NUM_ENT(cp->csp[-1].page) - 1) {
			*spanp = 1;
			npgno = PGNO(pg);
			next_recno = cp->recno;
			goto next_page;
		}

		/* Lock and get the next page. */
		if ((ret = __db_lget(dbc, LCK_COUPLE,
		     npgno, DB_LOCK_WRITE, 0, &ncp->lock)) != 0)
			goto err1;
		if ((ret = __memp_fget(dbmp, &npgno, dbc->txn,
		    DB_MPOOL_DIRTY, &npg)) != 0)
			goto err1;

		/* Fix up the next page cursor with its parent node. */
		if ((ret = __memp_fget(dbmp, &PGNO(cp->csp[-1].page),
		    dbc->txn, 0, &ppg)) != 0)
			goto err1;
		BT_STK_PUSH(dbenv, ncp, ppg,
		     cp->csp[-1].indx + 1, nolock, DB_LOCK_NG, ret);
		if (ret != 0)
			goto err1;

		/* Put the page on the stack. */
		BT_STK_ENTER(dbenv, ncp, npg, 0, ncp->lock, DB_LOCK_WRITE, ret);

		LOCK_INIT(ncp->lock);
		npg = NULL;

		c_data->compact_pages_examine++;

		PTRACE(dbc, "MDups", PGNO(ncp->csp->page), start, 0);
		if (check_dups && (ret = __bam_compact_dups(ndbc,
		     &ncp->csp->page, factor, 1, c_data, &pgs_done)) != 0)
			goto err1;

		npgno = NEXT_PGNO(ncp->csp->page);
		/*
		 * Merge the pages.  This will either free the next
		 * page or just update its parent pointer.
		 */
		PTRACE(dbc, "Merge", PGNO(cp->csp->page), start, 0);
		if ((ret = __bam_merge(dbc,
		     ndbc, factor, stop, c_data, &done)) != 0)
			goto err1;

		pgs_done++;

		/*
		 * __bam_merge could have freed our stack if it
		 * deleted a page possibly collapsing the tree.
		 */
		if (cp->csp->page == NULL)
			goto deleted;
		/* If we did not bump to the next page something did not fit. */
		if (npgno != NEXT_PGNO(pg))
			break;
	}

	/* Bottom of the main loop.  Move to the next page. */
	npgno = NEXT_PGNO(pg);
	cp->recno += NUM_ENT(pg);
	next_recno = cp->recno;

next_page:
	if ((ret = __bam_stkrel(dbc, pgs_done == 0 ? STK_NOLOCK : 0)) != 0)
		goto err1;
	if (ndbc != NULL &&
	     (ret = __bam_stkrel(ndbc, pgs_done == 0 ? STK_NOLOCK : 0)) != 0)
		goto err1;

next_no_release:
	pg = NULL;

	if (npgno == PGNO_INVALID || c_data->compact_pages  == 0)
		done = 1;
	if (!done) {
		/*
		 * If we are at the end of this parent commit the
		 * transaction so we don't tie things up.
		 */
		if (pgs_done != 0 && *spanp) {
deleted:		if (((ret = __bam_stkrel(ndbc, 0)) != 0 ||
			     (ret = __db_c_close(ndbc)) != 0))
				goto err;
			*donep = 0;
			return (0);
		}

		/* Reget the next page to look at. */
		cp->recno = next_recno;
		if ((ret = __db_lget(dbc,
		    pgs_done ? LCK_COUPLE_ALWAYS : LCK_COUPLE,
		    npgno, DB_LOCK_READ, 0, &cp->csp->lock)) != 0 ||
		    (ret = __memp_fget(dbmp, &npgno, dbc->txn, 0, &pg)) != 0)
			goto err;
		next_page = 1;
		goto next;
	}

done:
	if (0) {
		/* We come here if pg is the same as cp->csp->page. */
err1:		pg = NULL;
	}
err:	if (dbc != NULL &&
	    (t_ret = __bam_stkrel(dbc, STK_CLRDBC)) != 0 && ret == 0)
		ret = t_ret;
	if (ndbc != NULL) {
		if ((t_ret = __bam_stkrel(ndbc, STK_CLRDBC)) != 0 && ret == 0)
			ret = t_ret;
		else if ((t_ret = __db_c_close(ndbc)) != 0 && ret == 0)
			ret = t_ret;
	}

	if (pg != NULL && (t_ret = __memp_fput(dbmp, pg, 0) != 0) && ret == 0)
		ret = t_ret;
	if (npg != NULL && (t_ret = __memp_fput(dbmp, npg, 0) != 0) && ret == 0)
		ret = t_ret;

	*donep = done;

	return (ret);
}

/*
 * __bam_merge -- do actual merging of leaf pages.
 */
static int
__bam_merge(dbc, ndbc, factor, stop, c_data, donep)
	DBC *dbc, *ndbc;
	u_int32_t factor;
	DBT *stop;
	DB_COMPACT *c_data;
	int *donep;
{
	BTREE_CURSOR *cp, *ncp;
	BTREE *t;
	DB *dbp;
	PAGE *pg, *npg;
	db_indx_t adj, nent;
	db_recno_t recno;
	int cmp, ret;
	int (*func) __P((DB *, const DBT *, const DBT *));

	dbp = dbc->dbp;
	t = dbp->bt_internal;
	cp = (BTREE_CURSOR *)dbc->internal;
	ncp = (BTREE_CURSOR *)ndbc->internal;
	pg = cp->csp->page;
	npg = ncp->csp->page;

	nent = NUM_ENT(npg);

	/* If the page is empty just throw it away. */
	if (nent == 0)
		goto free;
	adj = TYPE(npg) == P_LBTREE ? P_INDX : O_INDX;
	/* Find if the stopping point is on this page. */
	if (stop != NULL && stop->size != 0) {
		if (dbc->dbtype == DB_RECNO) {
			if ((ret = __ram_getno(dbc, stop, &recno, 0)) != 0)
				goto err;
			if (ncp->recno > recno) {
				*donep = 1;
				if (cp->recno > recno)
					goto done;
			}
		} else {
			func = TYPE(npg) == P_LBTREE ?
			     (dbp->dup_compare == NULL ?
			     __bam_defcmp : dbp->dup_compare) : t->bt_compare;

			if ((ret = __bam_cmp(dbp, dbc->txn,
			    stop, npg, nent - adj, func, &cmp)) != 0)
				goto err;

			/*
			 * If the last record is beyond the stopping
			 * point we are done after this page.  If the
			 * first record is beyond the stopping point
			 * don't even bother with this page.
			 */
			if (cmp <= 0) {
				*donep = 1;
				if ((ret = __bam_cmp(dbp, dbc->txn,
				    stop, npg, 0, func, &cmp)) != 0)
					goto err;
				if (cmp <= 0)
					goto done;
			}
		}
	}

	/*
	 * If there is too much data then just move records one at a time.
	 * Otherwise copy the data space over and fix up the index table.
	 * If we are on the left most child we will effect our parent's
	 * index entry so we call merge_records to figure out key sizes.
	 */
	if ((dbc->dbtype == DB_BTREE &&
	    ncp->csp[-1].indx == 0 && ncp->csp[-1].entries != 1) ||
	    (int)(P_FREESPACE(dbp, pg) -
	    ((dbp->pgsize - P_OVERHEAD(dbp)) -
	    P_FREESPACE(dbp, npg))) < (int)factor)
		ret = __bam_merge_records(dbc, ndbc, factor, c_data);
	else
free:		ret = __bam_merge_pages(dbc, ndbc, c_data);

done:
err:	return (ret);
}

static int
__bam_merge_records(dbc, ndbc, factor, c_data)
	DBC *dbc, *ndbc;
	u_int32_t factor;
	DB_COMPACT *c_data;
{
	BINTERNAL *bi;
	BKEYDATA *bk, *tmp_bk;
	BTREE *t;
	BTREE_CURSOR *cp, *ncp;
	DB *dbp;
	DBT a, b, data, hdr;
	DB_ENV *dbenv;
	EPG *epg;
	PAGE *pg, *npg;
	db_indx_t adj, indx, nent, *ninp, pind;
	int32_t adjust;
	u_int32_t freespace, nksize, pfree, size;
	int first_dup, is_dup, next_dup, n_ok, ret;
	size_t (*func) __P((DB *, const DBT *, const DBT *));

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	t = dbp->bt_internal;
	cp = (BTREE_CURSOR *)dbc->internal;
	ncp = (BTREE_CURSOR *)ndbc->internal;
	pg = cp->csp->page;
	npg = ncp->csp->page;
	memset(&hdr, 0, sizeof(hdr));
	pind = NUM_ENT(pg);
	n_ok = 0;
	adjust = 0;
	ret = 0;
	nent = NUM_ENT(npg);

	DB_ASSERT(dbenv, nent != 0);

	/* See if we want to swap out this page. */
	if (c_data->compact_truncate != PGNO_INVALID &&
	     PGNO(npg) > c_data->compact_truncate) {
		/* Get a fresh low numbered page. */
		if ((ret = __bam_truncate_page(ndbc, &npg, 1)) != 0)
			goto err;
	}

	ninp = P_INP(dbp, npg);

	/*
	 * pg is the page that is being filled, it is in the stack in cp.
	 * npg is the next page, it is in the stack in ncp.
	 */
	freespace = P_FREESPACE(dbp, pg);

	adj = TYPE(npg) == P_LBTREE ? P_INDX : O_INDX;
	/*
	 * Loop through the records and find the stopping point.
	 */
	for (indx = 0; indx < nent; indx += adj)  {
		bk = GET_BKEYDATA(dbp, npg, indx);

		/* Size of the key. */
		size = BITEM_PSIZE(bk);

		/* Size of the data. */
		if (TYPE(pg) == P_LBTREE)
			size += BITEM_PSIZE(GET_BKEYDATA(dbp, npg, indx + 1));
		/*
		 * If we are at a duplicate set, skip ahead to see and
		 * get the total size for the group.
		 */
		n_ok = adj;
		if (TYPE(pg) == P_LBTREE &&
		     indx < nent - adj &&
		     ninp[indx] == ninp[indx + adj]) {
			do {
				/* Size of index for key reference. */
				size += sizeof(db_indx_t);
				n_ok++;
				/* Size of data item. */
				size += BITEM_PSIZE(
				    GET_BKEYDATA(dbp, npg, indx + n_ok));
				n_ok++;
			} while (indx + n_ok < nent &&
			    ninp[indx] == ninp[indx + n_ok]);
		}
		/* if the next set will not fit on the page we are done. */
		if (freespace < size)
			break;

		/*
		 * Otherwise figure out if we are past the goal and if
		 * adding this set will put us closer to the goal than
		 * we are now.
		 */
		if ((freespace - size) < factor) {
			if (freespace - factor > factor - (freespace - size))
				indx += n_ok;
			break;
		}
		freespace -= size;
		indx += n_ok - adj;
	}
	if (indx == 0)
		goto done;
	if (TYPE(pg) != P_LBTREE && TYPE(pg) != P_LDUP) {
		if (indx == nent)
			return (__bam_merge_pages(dbc, ndbc, c_data));
		goto no_check;
	}
	/*
	 * We need to update npg's parent key.  Avoid creating a new key
	 * that will be too big. Get what space will be available on the
	 * parents. Then if there will not be room for this key, see if
	 * prefix compression will make it work, if not backup till we
	 * find something that will.  (Needless to say, this is a very
	 * unlikely event.)  If we are deleting this page then we will
	 * need to propagate the next key to our grand parents, so we
	 * see if that will fit.
	 */
	pfree = dbp->pgsize;
	for (epg = &ncp->csp[-1]; epg >= ncp->sp; epg--)
		if ((freespace = P_FREESPACE(dbp, epg->page)) < pfree) {
			bi = GET_BINTERNAL(dbp, epg->page, epg->indx);
			/* Add back in the key we will be deleting. */
			freespace += BINTERNAL_PSIZE(bi->len);
			if (freespace < pfree)
				pfree = freespace;
			if (epg->indx != 0)
				break;
		}

	/*
	 * If we are at the end, we will delete this page.  We need to
	 * check the next parent key only if we are the leftmost page and
	 * will therefore have to propagate the key up the tree.
	 */
	if (indx == nent) {
		if (ncp->csp[-1].indx != 0 || ncp->csp[-1].entries == 1 ||
		     BINTERNAL_PSIZE(GET_BINTERNAL(dbp,
		     ncp->csp[-1].page, 1)->len) <= pfree)
			return (__bam_merge_pages(dbc, ndbc, c_data));
		indx -= adj;
	}
	bk = GET_BKEYDATA(dbp, npg, indx);
	if (indx != 0 && BINTERNAL_SIZE(bk->len) >= pfree) {
		if (F_ISSET(dbc, DBC_OPD)) {
			if (dbp->dup_compare == __bam_defcmp)
				func = __bam_defpfx;
			else
				func = NULL;
		} else
			func = t->bt_prefix;
	} else
		func = NULL;

	/* Skip to the beginning of a duplicate set. */
	while (indx != 0 && ninp[indx] == ninp[indx - adj])
		indx -= adj;

	while (indx != 0 && BINTERNAL_SIZE(bk->len) >= pfree) {
		if (B_TYPE(bk->type) != B_KEYDATA)
			goto noprefix;
		/*
		 * Figure out if we can truncate this key.
		 * Code borrowed from bt_split.c
		 */
		if (func == NULL)
			goto noprefix;
		tmp_bk = GET_BKEYDATA(dbp, npg, indx - adj);
		if (B_TYPE(tmp_bk->type) != B_KEYDATA)
			goto noprefix;
		memset(&a, 0, sizeof(a));
		a.size = tmp_bk->len;
		a.data = tmp_bk->data;
		memset(&b, 0, sizeof(b));
		b.size = bk->len;
		b.data = bk->data;
		nksize = (u_int32_t)func(dbp, &a, &b);
		if (BINTERNAL_PSIZE(nksize) < pfree)
			break;
noprefix:
		/* Skip to the beginning of a duplicate set. */
		do {
			indx -= adj;
		} while (indx != 0 &&  ninp[indx] == ninp[indx - adj]);

		bk = GET_BKEYDATA(dbp, npg, indx);
	}

	if (indx == 0)
		goto done;
	DB_ASSERT(dbenv, indx <= nent);

	/* Loop through the records and move them from npg to pg. */
no_check: is_dup = first_dup = next_dup = 0;
	if ((ret = __memp_dirty(dbp->mpf, &cp->csp->page, dbc->txn, 0)) != 0 ||
	    (ret = __memp_dirty(dbp->mpf, &ncp->csp->page, dbc->txn, 0)) != 0)
		goto err;
	pg = cp->csp->page;
	npg = ncp->csp->page;
	ninp = P_INP(dbp, npg);
	do {
		bk = GET_BKEYDATA(dbp, npg, 0);
		/* Figure out if we are in a duplicate group or not. */
		if ((NUM_ENT(npg) % 2) == 0) {
			if (NUM_ENT(npg) > 2 && ninp[0] == ninp[2]) {
				if (!is_dup) {
					first_dup = 1;
					is_dup = 1;
				} else
					first_dup = 0;

				next_dup = 1;
			} else if (next_dup) {
				is_dup = 1;
				first_dup = 0;
				next_dup = 0;
			} else
				is_dup = 0;
		}

		if (is_dup && !first_dup && (pind % 2) == 0) {
			/* Duplicate key. */
			if ((ret = __bam_adjindx(dbc,
			     pg, pind, pind - P_INDX, 1)) != 0)
				goto err;
			if (!next_dup)
				is_dup = 0;
		} else switch (B_TYPE(bk->type)) {
		case B_KEYDATA:
			hdr.data = bk;
			hdr.size = SSZA(BKEYDATA, data);
			data.size = bk->len;
			data.data = bk->data;
			if ((ret = __db_pitem(dbc, pg, pind,
			     BKEYDATA_SIZE(bk->len), &hdr, &data)) != 0)
				goto err;
			break;
		case B_OVERFLOW:
		case B_DUPLICATE:
			data.size = BOVERFLOW_SIZE;
			data.data = bk;
			if ((ret = __db_pitem(dbc, pg, pind,
			     BOVERFLOW_SIZE, &data, NULL)) != 0)
				goto err;
			break;
		default:
			__db_errx(dbenv,
			    "Unknown record format, page %lu, indx 0",
			    (u_long)PGNO(pg));
			ret = EINVAL;
			goto err;
		}
		pind++;
		if (next_dup && (NUM_ENT(npg) % 2) == 0) {
			if ((ret = __bam_adjindx(ndbc,
			     npg, 0, O_INDX, 0)) != 0)
				goto err;
		} else {
			if ((ret = __db_ditem(ndbc,
			     npg, 0, BITEM_SIZE(bk))) != 0)
				goto err;
		}
		adjust++;
	} while (--indx != 0);

	DB_ASSERT(dbenv, NUM_ENT(npg) != 0);

	if (adjust != 0 &&
	     (F_ISSET(cp, C_RECNUM) || F_ISSET(dbc, DBC_OPD))) {
		DB_ASSERT(dbenv, cp->csp - cp->sp == ncp->csp - ncp->sp);
		if (TYPE(pg) == P_LBTREE)
			adjust /= P_INDX;
		if ((ret = __bam_adjust(ndbc, -adjust)) != 0)
			goto err;

		if ((ret = __bam_adjust(dbc, adjust)) != 0)
			goto err;
	}

	/* Update parent with new key. */
	if (ndbc->dbtype == DB_BTREE &&
	    (ret = __bam_pupdate(ndbc, pg)) != 0)
		goto err;

done:	ret = __bam_stkrel(ndbc, STK_CLRDBC);

err:	return (ret);
}

static int
__bam_merge_pages(dbc, ndbc, c_data)
	DBC *dbc, *ndbc;
	DB_COMPACT *c_data;
{
	BTREE_CURSOR *cp, *ncp;
	DB *dbp;
	DBT data, hdr, ind;
	DB_MPOOLFILE *dbmp;
	PAGE *pg, *npg;
	db_indx_t nent, *ninp, *pinp;
	db_pgno_t ppgno;
	u_int8_t *bp;
	u_int32_t len;
	int i, level, ret;

	COMPQUIET(ppgno, PGNO_INVALID);
	dbp = dbc->dbp;
	dbmp = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	ncp = (BTREE_CURSOR *)ndbc->internal;
	pg = cp->csp->page;
	npg = ncp->csp->page;
	memset(&hdr, 0, sizeof(hdr));
	nent = NUM_ENT(npg);

	/* If the page is empty just throw it away. */
	if (nent == 0)
		goto free;

	if ((ret = __memp_dirty(dbp->mpf, &cp->csp->page, dbc->txn, 0)) != 0 ||
	    (ret = __memp_dirty(dbp->mpf, &ncp->csp->page, dbc->txn, 0)) != 0)
		goto err;
	pg = cp->csp->page;
	npg = ncp->csp->page;
	DB_ASSERT(dbp->dbenv, nent == NUM_ENT(npg));

	/* Bulk copy the data to the new page. */
	len = dbp->pgsize - HOFFSET(npg);
	if (DBC_LOGGING(dbc)) {
		data.data = (u_int8_t *)npg + HOFFSET(npg);
		data.size = len;
		ind.data = P_INP(dbp, npg);
		ind.size = NUM_ENT(npg) * sizeof(db_indx_t);
		if ((ret = __bam_merge_log(dbp,
		     dbc->txn, &LSN(pg), 0, PGNO(pg),
		     &LSN(pg), PGNO(npg), &LSN(npg), NULL, &data, &ind)) != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(LSN(pg));
	LSN(npg) = LSN(pg);
	bp = (u_int8_t *)pg + HOFFSET(pg) - len;
	memcpy(bp, (u_int8_t *)npg + HOFFSET(npg), len);

	/* Copy index table offset by what was there already. */
	pinp = P_INP(dbp, pg) + NUM_ENT(pg);
	ninp = P_INP(dbp, npg);
	for (i = 0; i < NUM_ENT(npg); i++)
		*pinp++ = *ninp++ - (dbp->pgsize - HOFFSET(pg));
	HOFFSET(pg) -= len;
	NUM_ENT(pg) += i;

	NUM_ENT(npg) = 0;
	HOFFSET(npg) += len;

	if (F_ISSET(cp, C_RECNUM) || F_ISSET(dbc, DBC_OPD)) {
		DB_ASSERT(dbp->dbenv, cp->csp - cp->sp == ncp->csp - ncp->sp);
		if (TYPE(pg) == P_LBTREE)
			i /= P_INDX;
		if ((ret = __bam_adjust(ndbc, -i)) != 0)
			goto err;

		if ((ret = __bam_adjust(dbc, i)) != 0)
			goto err;
	}

free:	/*
	 * __bam_dpages may decide to collapse the tree.
	 * This can happen if we have the root and there
	 * are exactly 2 pointers left in it.
	 * If it can collapse the tree we must free the other
	 * stack since it will nolonger be valid.  This
	 * must be done before hand because we cannot
	 * hold a page pinned if it might be truncated.
	 */
	if (PGNO(ncp->sp->page) == ncp->root &&
	    NUM_ENT(ncp->sp->page) == 2) {
		if ((ret = __bam_stkrel(dbc, STK_CLRDBC | STK_PGONLY)) != 0)
			goto err;
		level = LEVEL(ncp->sp->page);
		ppgno = PGNO(ncp->csp[-1].page);
	} else
		level = 0;
	if (c_data->compact_truncate > PGNO(npg))
		c_data->compact_truncate--;
	if ((ret = __bam_dpages(ndbc,
	    0, ndbc->dbtype == DB_RECNO ? 0 : 1)) != 0)
		goto err;
	npg = NULL;
	c_data->compact_pages_free++;
	c_data->compact_pages--;
	if (level != 0) {
		if ((ret = __memp_fget(dbmp, &ncp->root, dbc->txn,
		    0, &npg)) != 0)
			goto err;
		if (level == LEVEL(npg))
			level = 0;
		if ((ret = __memp_fput(dbmp, npg, 0)) != 0)
			goto err;
		npg = NULL;
		if (level != 0) {
			c_data->compact_levels++;
			c_data->compact_pages_free++;
			if (c_data->compact_truncate > ppgno)
				c_data->compact_truncate--;
			if (c_data->compact_pages != 0)
				c_data->compact_pages--;
		}
	}

err:	return (ret);
}

/*
 * __bam_merge_internal --
 *	Merge internal nodes of the tree.
 */
static int
__bam_merge_internal(dbc, ndbc, level, c_data, merged)
	DBC *dbc, *ndbc;
	int level;
	DB_COMPACT *c_data;
	int *merged;
{
	BINTERNAL bi, *bip, *fip;
	BTREE_CURSOR *cp, *ncp;
	DB *dbp;
	DBT data, hdr;
	DB_MPOOLFILE *dbmp;
	EPG *epg, *save_csp, *nsave_csp;
	PAGE *pg, *npg;
	RINTERNAL *rk;
	db_indx_t indx, pind;
	db_pgno_t ppgno;
	int32_t trecs;
	u_int16_t size;
	u_int32_t freespace, pfree;
	int ret;

	COMPQUIET(bip, NULL);
	COMPQUIET(ppgno, PGNO_INVALID);

	/*
	 * ndbc will contain the the dominating parent of the subtree.
	 * dbc will have the tree containing the left child.
	 *
	 * The stacks descend to the leaf level.
	 * If this is a recno tree then both stacks will start at the root.
	 */
	dbp = dbc->dbp;
	dbmp = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	ncp = (BTREE_CURSOR *)ndbc->internal;
	*merged = 0;
	ret = 0;

	/*
	 * Set the stacks to the level requested.
	 * Save the old value to restore when we exit.
	 */
	save_csp = cp->csp;
	cp->csp = &cp->csp[-level + 1];
	pg = cp->csp->page;
	pind = NUM_ENT(pg);

	nsave_csp = ncp->csp;
	ncp->csp = &ncp->csp[-level + 1];
	npg = ncp->csp->page;
	indx = NUM_ENT(npg);

	/*
	 * The caller may have two stacks that include common ancestors, we
	 * check here for convenience.
	 */
	if (npg == pg)
		goto done;

	if ((ret = __memp_dirty(dbmp, &cp->csp->page, dbc->txn, 0)) != 0 ||
	    (ret = __memp_dirty(dbmp, &ncp->csp->page, dbc->txn, 0)) != 0)
		goto err;
	pg = cp->csp->page;
	npg = ncp->csp->page;

	if (TYPE(pg) == P_IBTREE) {
		/*
		 * Check for overflow keys on both pages while we have
		 * them locked.
		 */
		 if ((ret =
		      __bam_truncate_internal_overflow(dbc, pg, c_data)) != 0)
			goto err;
		 if ((ret =
		      __bam_truncate_internal_overflow(dbc, npg, c_data)) != 0)
			goto err;
	}

	/*
	 * If we are about to move data off the left most page of an
	 * internal node we will need to update its parents, make sure there
	 * will be room for the new key on all the parents in the stack.
	 * If not, move less data.
	 */
	fip = NULL;
	if (TYPE(pg) == P_IBTREE) {
		/* See where we run out of space. */
		freespace = P_FREESPACE(dbp, pg);
		/*
		 * The leftmost key of an internal page is not accurate.
		 * Go up the tree to find a non-leftmost parent.
		 */
		epg = ncp->csp;
		while (--epg >= ncp->sp && epg->indx == 0)
			continue;
		fip = bip = GET_BINTERNAL(dbp, epg->page, epg->indx);
		epg = ncp->csp;

		for (indx = 0;;) {
			size = BINTERNAL_PSIZE(bip->len);
			if (size > freespace)
				break;
			freespace -= size;
			if (++indx >= NUM_ENT(npg))
				break;
			bip = GET_BINTERNAL(dbp, npg, indx);
		}

		/* See if we are deleting the page and we are not left most. */
		if (indx == NUM_ENT(npg) && epg[-1].indx != 0)
			goto fits;

		pfree = dbp->pgsize;
		for (epg--; epg >= ncp->sp; epg--)
			if ((freespace = P_FREESPACE(dbp, epg->page)) < pfree) {
				bip = GET_BINTERNAL(dbp, epg->page, epg->indx);
				/* Add back in the key we will be deleting. */
				freespace += BINTERNAL_PSIZE(bip->len);
				if (freespace < pfree)
					pfree = freespace;
				if (epg->indx != 0)
					break;
			}
		epg = ncp->csp;

		/* If we are at the end of the page we will delete it. */
		if (indx == NUM_ENT(npg)) {
			if (NUM_ENT(epg[-1].page) == 1)
				goto fits;
			bip =
			     GET_BINTERNAL(dbp, epg[-1].page, epg[-1].indx + 1);
		} else
			bip = GET_BINTERNAL(dbp, npg, indx);

		/* Back up until we have a key that fits. */
		while (indx != 0 && BINTERNAL_PSIZE(bip->len) > pfree) {
			indx--;
			bip = GET_BINTERNAL(dbp, npg, indx);
		}
		if (indx == 0)
			goto done;
	}

fits:	memset(&bi, 0, sizeof(bi));
	memset(&hdr, 0, sizeof(hdr));
	memset(&data, 0, sizeof(data));
	trecs = 0;

	/*
	 * Copy data between internal nodes till one is full
	 * or the other is empty.
	 */
	do {
		if (dbc->dbtype == DB_BTREE) {
			bip = GET_BINTERNAL(dbp, npg, 0);
			size = fip == NULL ?
			     BINTERNAL_SIZE(bip->len) :
			     BINTERNAL_SIZE(fip->len);
			if (P_FREESPACE(dbp, pg) < size + sizeof(db_indx_t))
				break;

			if (fip == NULL) {
				data.size = bip->len;
				data.data = bip->data;
			} else {
				data.size = fip->len;
				data.data = fip->data;
			}
			bi.len = data.size;
			B_TSET(bi.type, bip->type);
			bi.pgno = bip->pgno;
			bi.nrecs = bip->nrecs;
			hdr.data = &bi;
			hdr.size = SSZA(BINTERNAL, data);
			if (F_ISSET(cp, C_RECNUM) || F_ISSET(dbc, DBC_OPD))
				trecs += (int32_t)bip->nrecs;
		} else {
			rk = GET_RINTERNAL(dbp, npg, 0);
			size = RINTERNAL_SIZE;
			if (P_FREESPACE(dbp, pg) < size + sizeof(db_indx_t))
				break;

			hdr.data = rk;
			hdr.size = size;
			trecs += (int32_t)rk->nrecs;
		}
		if ((ret = __db_pitem(dbc, pg, pind, size, &hdr, &data)) != 0)
			goto err;
		pind++;
		if (fip != NULL) {
			/* reset size to be for the record being deleted. */
			size = BINTERNAL_SIZE(bip->len);
			fip = NULL;
		}
		if ((ret = __db_ditem(ndbc, npg, 0, size)) != 0)
			goto err;
		*merged = 1;
	} while (--indx != 0);

	if (c_data->compact_truncate != PGNO_INVALID &&
	     PGNO(pg) > c_data->compact_truncate && cp->csp != cp->sp) {
		if ((ret = __bam_truncate_page(dbc, &pg, 1)) != 0)
			goto err;
	}

	if (NUM_ENT(npg) != 0 && c_data->compact_truncate != PGNO_INVALID &&
	    PGNO(npg) > c_data->compact_truncate && ncp->csp != ncp->sp) {
		if ((ret = __bam_truncate_page(ndbc, &npg, 1)) != 0)
			goto err;
	}

	if (!*merged)
		goto done;

	if (trecs != 0) {
		DB_ASSERT(dbp->dbenv, cp->csp - cp->sp == ncp->csp - ncp->sp);
		cp->csp--;
		if ((ret = __bam_adjust(dbc, trecs)) != 0)
			goto err;

		ncp->csp--;
		if ((ret = __bam_adjust(ndbc, -trecs)) != 0)
			goto err;
		ncp->csp++;
	}
	cp->csp = save_csp;

	/*
	 * Either we emptied the page or we need to update its
	 * parent to reflect the first page we now point to.
	 * First get rid of the bottom of the stack,
	 * bam_dpages will clear the stack.  We can drop
	 * the locks on those pages as we have not done
	 * anything to them.
	 */
	do {
		if ((ret = __memp_fput(dbmp, nsave_csp->page, 0)) != 0)
			goto err;
		if ((ret = __LPUT(dbc, nsave_csp->lock)) != 0)
			goto err;
		nsave_csp--;
	} while (nsave_csp != ncp->csp);

	if (NUM_ENT(npg) == 0)  {
		/*
		 * __bam_dpages may decide to collapse the tree
		 * so we need to free our other stack.  The tree
		 * will change in hight and our stack will nolonger
		 * be valid.
		 */
		if (PGNO(ncp->sp->page) == ncp->root &&
		    NUM_ENT(ncp->sp->page) == 2) {
			if ((ret = __bam_stkrel(dbc, STK_CLRDBC)) != 0)
				goto err;
			level = LEVEL(ncp->sp->page);
			ppgno = PGNO(ncp->csp[-1].page);
		} else
			level = 0;

		if (c_data->compact_truncate > PGNO(npg))
			c_data->compact_truncate--;
		ret = __bam_dpages(ndbc,
		     0, ndbc->dbtype == DB_RECNO ? 0 : 1);
		c_data->compact_pages_free++;
		if (ret == 0 && level != 0) {
			if ((ret = __memp_fget(dbmp, &ncp->root, dbc->txn,
			    0, &npg)) != 0)
				goto err;
			if (level == LEVEL(npg))
				level = 0;
			if ((ret = __memp_fput(dbmp, npg, 0)) != 0)
				goto err;
			npg = NULL;
			if (level != 0) {
				c_data->compact_levels++;
				c_data->compact_pages_free++;
				if (c_data->compact_truncate > ppgno)
					c_data->compact_truncate--;
				if (c_data->compact_pages != 0)
					c_data->compact_pages--;
			}
		}
	} else
		ret = __bam_pupdate(ndbc, npg);
	return (ret);

done:
err:	cp->csp = save_csp;
	ncp->csp = nsave_csp;

	return (ret);
}

/*
 * __bam_compact_dups -- try to compress off page dup trees.
 * We may or may not have a write lock on this page.
 */
static int
__bam_compact_dups(dbc, ppg, factor, have_lock, c_data, donep)
	DBC *dbc;
	PAGE **ppg;
	u_int32_t factor;
	int have_lock;
	DB_COMPACT *c_data;
	int *donep;
{
	BOVERFLOW *bo;
	BTREE_CURSOR *cp;
	DB *dbp;
	DBC *opd;
	DBT start;
	DB_ENV *dbenv;
	DB_MPOOLFILE *dbmp;
	PAGE *dpg, *pg;
	db_indx_t i;
	int done, level, ret, span, t_ret;

	span = 0;
	ret = 0;
	opd = NULL;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	dbmp = dbp->mpf;
	cp = (BTREE_CURSOR *)dbc->internal;
	pg = *ppg;

	for (i = 0; i <  NUM_ENT(pg); i++) {
		bo = GET_BOVERFLOW(dbp, pg, i);
		if (B_TYPE(bo->type) == B_KEYDATA)
			continue;
		c_data->compact_pages_examine++;
		if (bo->pgno > c_data->compact_truncate) {
			(*donep)++;
			if (!have_lock) {
				if ((ret = __db_lget(dbc, 0, PGNO(pg),
				     DB_LOCK_WRITE, 0, &cp->csp->lock)) != 0)
					goto err;
				have_lock = 1;
				if ((ret = __memp_dirty(dbp->mpf, ppg,
				    dbc->txn, 0)) != 0)
					goto err;
				pg = *ppg;
			}
			if ((ret =
			     __bam_truncate_root_page(dbc, pg, i, c_data)) != 0)
				goto err;
			/* Just in case it should move.  Could it? */
			bo = GET_BOVERFLOW(dbp, pg, i);
		}

		if (B_TYPE(bo->type) == B_OVERFLOW) {
			if ((ret = __bam_truncate_overflow(dbc, bo->pgno,
			     have_lock ? PGNO_INVALID : PGNO(pg), c_data)) != 0)
				goto err;
			(*donep)++;
			continue;
		}
		/*
		 * Take a peek at the root.  If it's a leaf then
		 * there is no tree here, avoid all the trouble.
		 */
		if ((ret = __memp_fget(dbmp, &bo->pgno, dbc->txn,
		    0, &dpg)) != 0)
			goto err;

		level = dpg->level;
		if ((ret = __memp_fput(dbmp, dpg, 0)) != 0)
			goto err;
		if (level == LEAFLEVEL)
			continue;
		if ((ret = __db_c_newopd(dbc, bo->pgno, NULL, &opd)) != 0)
			return (ret);
		if (!have_lock) {
			if ((ret = __db_lget(dbc, 0,
			     PGNO(pg), DB_LOCK_WRITE, 0, &cp->csp->lock)) != 0)
				goto err;
			have_lock = 1;
			if ((ret = __memp_dirty(dbp->mpf, ppg,
			    dbc->txn, 0)) != 0)
				goto err;
			pg = *ppg;
		}
		(*donep)++;
		memset(&start, 0, sizeof(start));
		do {
			if ((ret = __bam_compact_int(opd, &start,
			     NULL, factor, &span, c_data, &done)) != 0)
				break;
		} while (!done);

		if (start.data != NULL)
			__os_free(dbenv, start.data);

		if (ret != 0)
			goto err;

		ret = __db_c_close(opd);
		opd = NULL;
		if (ret != 0)
			goto err;
	}

err:	if (opd != NULL && (t_ret = __db_c_close(opd)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __bam_truncate_page -- swap a page with a lower numbered page.
 *	The cusor has a stack which includes at least the
 * immediate parent of this page.
 */
static int
__bam_truncate_page(dbc, pgp, update_parent)
	DBC *dbc;
	PAGE **pgp;
	int update_parent;
{
	BTREE_CURSOR *cp;
	DB *dbp;
	DBT data, hdr, ind;
	DB_LSN lsn;
	EPG *epg;
	PAGE *newpage;
	db_pgno_t newpgno, *pgnop;
	int ret;

	dbp = dbc->dbp;

	/*
	 * We want to free a page that lives in the part of the file that
	 * can be truncated, so we're going to move it onto a free page
	 * that is in the part of the file that need not be truncated.
	 * Since the freelist is ordered now, we can simply call __db_new
	 * which will grab the first element off the freelist; we know this
	 * is the lowest numbered free page.
	 */
	if ((ret = __db_new(dbc, P_DONTEXTEND | TYPE(*pgp), &newpage)) != 0)
		return (ret);

	/*
	 * If newpage is null then __db_new would have had to allocate
	 * a new page from the filesystem, so there is no reason
	 * to continue this action.
	 */
	if (newpage == NULL)
		return (0);

	/*
	 * It is possible that a higher page is allocated if other threads
	 * are allocating at the same time, if so, just put it back.
	 */
	if (PGNO(newpage) > PGNO(*pgp)) {
		/* Its unfortunate but you can't just free a new overflow. */
		if (TYPE(newpage) == P_OVERFLOW)
			OV_LEN(newpage) = 0;
		return (__db_free(dbc, newpage));
	}

	if ((ret = __memp_dirty(dbp->mpf, &newpage, dbc->txn, 0)) != 0)
		goto err;

	/* Log if necessary. */
	if (DBC_LOGGING(dbc)) {
		hdr.data = *pgp;
		hdr.size = P_OVERHEAD(dbp);
		if (TYPE(*pgp) == P_OVERFLOW) {
			data.data = (u_int8_t *)*pgp + P_OVERHEAD(dbp);
			data.size = OV_LEN(*pgp);
			ind.size = 0;
		} else {
			data.data = (u_int8_t *)*pgp + HOFFSET(*pgp);
			data.size = dbp->pgsize - HOFFSET(*pgp);
			ind.data = P_INP(dbp, *pgp);
			ind.size = NUM_ENT(*pgp) * sizeof(db_indx_t);
		}
		if ((ret = __bam_merge_log(dbp, dbc->txn,
		      &LSN(newpage), 0, PGNO(newpage), &LSN(newpage),
		      PGNO(*pgp), &LSN(*pgp), &hdr, &data, &ind)) != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(LSN(newpage));

	newpgno = PGNO(newpage);
	lsn = LSN(newpage);
	memcpy(newpage, *pgp, dbp->pgsize);
	PGNO(newpage) = newpgno;
	LSN(newpage) = lsn;

	/* Empty the old page. */
	if ((ret = __memp_dirty(dbp->mpf, pgp, dbc->txn, 0)) != 0)
		goto err;
	if (TYPE(*pgp) == P_OVERFLOW)
		OV_LEN(*pgp) = 0;
	else {
		HOFFSET(*pgp) = dbp->pgsize;
		NUM_ENT(*pgp) = 0;
	}
	LSN(*pgp) = lsn;

	/* Update siblings. */
	switch (TYPE(newpage)) {
	case P_OVERFLOW:
	case P_LBTREE:
	case P_LRECNO:
	case P_LDUP:
		if (NEXT_PGNO(newpage) == PGNO_INVALID &&
		    PREV_PGNO(newpage) == PGNO_INVALID)
			break;
		if ((ret = __bam_relink(dbc, *pgp, PGNO(newpage))) != 0)
			goto err;
		break;
	default:
		break;
	}
	cp = (BTREE_CURSOR*)dbc->internal;

	/*
	 * Now, if we free this page, it will get truncated, when we free
	 * all the pages after it in the file.
	 */
	ret = __db_free(dbc, *pgp);
	/* db_free always puts the page. */
	*pgp = newpage;

	if (ret != 0)
		return (ret);

	if (!update_parent)
		goto done;

	/* Update the parent. */
	epg = &cp->csp[-1];
	if ((ret = __memp_dirty(dbp->mpf, &epg->page, dbc->txn, 0)) != 0)
		return (ret);

	switch (TYPE(epg->page)) {
	case P_IBTREE:
		pgnop = &GET_BINTERNAL(dbp, epg->page, epg->indx)->pgno;
		break;
	case P_IRECNO:
		pgnop = &GET_RINTERNAL(dbp, epg->page, epg->indx)->pgno;
		break;
	default:
		pgnop = &GET_BOVERFLOW(dbp, epg->page, epg->indx)->pgno;
		break;
	}
	if (DBC_LOGGING(dbc)) {
		if ((ret = __bam_pgno_log(dbp, dbc->txn, &LSN(epg->page),
		    0, PGNO(epg->page), &LSN(epg->page), (u_int32_t)epg->indx,
		    *pgnop, PGNO(newpage))) != 0)
			return (ret);
	} else
		LSN_NOT_LOGGED(LSN(epg->page));

	*pgnop = PGNO(newpage);
	cp->csp->page = newpage;

done:	return (0);

err:	(void)__memp_fput(dbp->mpf, newpage, 0);
	return (ret);
}

/*
 * __bam_truncate_overflow -- find overflow pages to truncate.
 *	Walk the pages of an overflow chain and swap out
 * high numbered pages.  We are passed the first page
 * but only deal with the second and subsequent pages.
 */

static int
__bam_truncate_overflow(dbc, pgno, pg_lock, c_data)
	DBC *dbc;
	db_pgno_t pgno;
	db_pgno_t pg_lock;
	DB_COMPACT *c_data;
{
	DB *dbp;
	DB_LOCK lock;
	PAGE *page;
	int ret, t_ret;

	dbp = dbc->dbp;
	page = NULL;
	LOCK_INIT(lock);

	if ((ret = __memp_fget(dbp->mpf, &pgno, dbc->txn, 0, &page)) != 0)
		return (ret);

	while ((pgno = NEXT_PGNO(page)) != PGNO_INVALID) {
		if ((ret = __memp_fput(dbp->mpf, page, 0)) != 0)
			return (ret);
		if ((ret = __memp_fget(dbp->mpf, &pgno, dbc->txn,
		    0, &page)) != 0)
			return (ret);
		if (pgno <= c_data->compact_truncate)
			continue;
		if (pg_lock != PGNO_INVALID) {
			if ((ret = __db_lget(dbc,
			     0, pg_lock, DB_LOCK_WRITE, 0, &lock)) != 0)
				break;
			pg_lock = PGNO_INVALID;
		}
		if ((ret = __bam_truncate_page(dbc, &page, 0)) != 0)
			break;
	}

	if (page != NULL &&
	    (t_ret = __memp_fput(dbp->mpf, page, 0)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __bam_truncate_root_page -- swap a page which is
 *    the root of an off page dup tree or the head of an overflow.
 * The page is reference by the pg/indx passed in.
 */
static int
__bam_truncate_root_page(dbc, pg, indx, c_data)
	DBC *dbc;
	PAGE *pg;
	u_int32_t indx;
	DB_COMPACT *c_data;
{
	BINTERNAL *bi;
	BOVERFLOW *bo;
	DB *dbp;
	DBT orig;
	PAGE *page;
	db_pgno_t newpgno, *pgnop;
	int ret, t_ret;

	COMPQUIET(c_data, NULL);
	COMPQUIET(bo, NULL);
	COMPQUIET(newpgno, PGNO_INVALID);
	dbp = dbc->dbp;
	page = NULL;
	if (TYPE(pg) == P_IBTREE) {
		bi = GET_BINTERNAL(dbp, pg, indx);
		if (B_TYPE(bi->type) == B_OVERFLOW) {
			bo = (BOVERFLOW *)(bi->data);
			pgnop = &bo->pgno;
		} else
			pgnop = &bi->pgno;
	} else {
		bo = GET_BOVERFLOW(dbp, pg, indx);
		pgnop = &bo->pgno;
	}

	DB_ASSERT(dbp->dbenv, IS_DIRTY(pg));

	if ((ret = __memp_fget(dbp->mpf, pgnop, dbc->txn, 0, &page)) != 0)
		goto err;

	/*
	 * If this is a multiply reference overflow key, then we will just
	 * copy it and decrement the reference count.  This is part of a
	 * fix to get rid of multiple references.
	 */
	if (TYPE(page) == P_OVERFLOW && OV_REF(page) > 1) {
		if ((ret = __db_ovref(dbc, bo->pgno)) != 0)
			goto err;
		memset(&orig, 0, sizeof(orig));
		if ((ret = __db_goff(dbp, dbc->txn, &orig,
		    bo->tlen, bo->pgno, &orig.data, &orig.size)) == 0)
			ret = __db_poff(dbc, &orig, &newpgno);
		if (orig.data != NULL)
			__os_free(dbp->dbenv, orig.data);
		if (ret != 0)
			goto err;
	} else {
		if ((ret = __bam_truncate_page(dbc, &page, 0)) != 0)
			goto err;
		newpgno = PGNO(page);
		/* If we could not allocate from the free list, give up.*/
		if (newpgno == *pgnop)
			goto err;
	}

	/* Update the reference. */
	if (DBC_LOGGING(dbc)) {
		if ((ret = __bam_pgno_log(dbp,
		     dbc->txn, &LSN(pg), 0, PGNO(pg),
		     &LSN(pg), (u_int32_t)indx, *pgnop, newpgno)) != 0)
			goto err;
	} else
		LSN_NOT_LOGGED(LSN(pg));

	*pgnop = newpgno;

err:	if (page != NULL && (t_ret =
	      __memp_fput(dbp->mpf, page, 0)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * -- bam_truncate_internal_overflow -- find overflow keys
 *	on internal pages and if they have high page
 * numbers swap them with lower pages and truncate them.
 * Note that if there are overflow keys in the internal
 * nodes they will get copied adding pages to the database.
 */
static int
__bam_truncate_internal_overflow(dbc, page, c_data)
	DBC *dbc;
	PAGE *page;
	DB_COMPACT *c_data;
{
	BINTERNAL *bi;
	BOVERFLOW *bo;
	db_indx_t indx;
	int ret;

	COMPQUIET(bo, NULL);
	ret = 0;
	for (indx = 0; indx < NUM_ENT(page); indx++) {
		bi = GET_BINTERNAL(dbc->dbp, page, indx);
		if (B_TYPE(bi->type) != B_OVERFLOW)
			continue;
		bo = (BOVERFLOW *)(bi->data);
		if (bo->pgno > c_data->compact_truncate && (ret =
		     __bam_truncate_root_page(dbc, page, indx, c_data)) != 0)
			break;
		if ((ret = __bam_truncate_overflow(
		     dbc, bo->pgno, PGNO_INVALID, c_data)) != 0)
			break;
	}
	return (ret);
}

#ifdef HAVE_FTRUNCATE
/*
 * __bam_savekey -- save the key from an internal page.
 *  We need to save information so that we can
 * fetch then next internal node of the tree.  This means
 * we need the btree key on this current page, or the
 * next record number.
 */
static int
__bam_savekey(dbc, next, start)
	DBC *dbc;
	int next;
	DBT *start;
{
	BINTERNAL *bi;
	BOVERFLOW *bo;
	BTREE_CURSOR *cp;
	DB *dbp;
	DB_ENV *dbenv;
	PAGE *pg;
	RINTERNAL *ri;
	db_indx_t indx, top;

	dbp = dbc->dbp;
	dbenv = dbp->dbenv;
	cp = (BTREE_CURSOR *)dbc->internal;
	pg = cp->csp->page;

	if (dbc->dbtype == DB_RECNO) {
		if (next)
			for (indx = 0, top = NUM_ENT(pg); indx != top; indx++) {
				ri = GET_RINTERNAL(dbp, pg, indx);
				cp->recno += ri->nrecs;
			}
		return (__db_retcopy(dbenv, start, &cp->recno,
		     sizeof(cp->recno), &start->data, &start->ulen));

	}
	bi = GET_BINTERNAL(dbp, pg, NUM_ENT(pg) - 1);
	if (B_TYPE(bi->type) == B_OVERFLOW) {
		bo = (BOVERFLOW *)(bi->data);
		return (__db_goff(dbp, dbc->txn, start,
		     bo->tlen, bo->pgno, &start->data, &start->ulen));
	}
	return (__db_retcopy(dbenv,
	     start, bi->data, bi->len,  &start->data, &start->ulen));
}

/*
 * bam_truncate_internal --
 *	Find high numbered pages in the internal nodes of a tree and
 *	swap them.
 */
static int
__bam_truncate_internal(dbp, txn, c_data)
	DB *dbp;
	DB_TXN *txn;
	DB_COMPACT *c_data;
{
	BTREE_CURSOR *cp;
	DBC *dbc;
	DBT start;
	PAGE *pg;
	db_pgno_t pgno;
	u_int32_t sflag;
	int level, local_txn, ret, t_ret;

	dbc = NULL;
	memset(&start, 0, sizeof(start));

	if (IS_DB_AUTO_COMMIT(dbp, txn)) {
		local_txn = 1;
		txn = NULL;
	} else
		local_txn = 0;

	level = LEAFLEVEL + 1;
	sflag = CS_READ | CS_GETRECNO;

new_txn:
	if (local_txn && (ret = __txn_begin(dbp->dbenv, NULL, &txn, 0)) != 0)
		goto err;

	if ((ret = __db_cursor(dbp, txn, &dbc, 0)) != 0)
		goto err;
	cp = (BTREE_CURSOR *)dbc->internal;

	pgno = PGNO_INVALID;
	do {
		if ((ret = __bam_csearch(dbc, &start, sflag, level)) != 0) {
			/* No more at this level, go up one. */
			if (ret == DB_NOTFOUND) {
				level++;
				if (start.data != NULL)
					__os_free(dbp->dbenv, start.data);
				memset(&start, 0, sizeof(start));
				sflag = CS_READ | CS_GETRECNO;
				continue;
			}
			goto err;
		}
		c_data->compact_pages_examine++;

		pg = cp->csp->page;
		pgno = PGNO(pg);

		sflag = CS_NEXT | CS_GETRECNO;
		/* Grab info about the page and drop the stack. */
		if (pgno != cp->root && (ret = __bam_savekey(dbc,
		    pgno <= c_data->compact_truncate, &start)) != 0)
			goto err;

		if ((ret = __bam_stkrel(dbc, STK_NOLOCK)) != 0)
			goto err;
		if (pgno == cp->root)
			break;

		if (pgno <= c_data->compact_truncate)
			continue;

		/* Reget the page with a write lock, and its parent too. */
		if ((ret = __bam_csearch(dbc,
		    &start, CS_PARENT | CS_GETRECNO, level)) != 0)
			goto err;
		pg = cp->csp->page;
		pgno = PGNO(pg);

		if (pgno > c_data->compact_truncate) {
			if ((ret = __bam_truncate_page(dbc, &pg, 1)) != 0)
				goto err;
		}
		if ((ret = __bam_stkrel(dbc,
		     pgno > c_data->compact_truncate ? 0 : STK_NOLOCK)) != 0)
			goto err;

		/* We are locking subtrees, so drop the write locks asap. */
		if (local_txn && pgno > c_data->compact_truncate)
			break;
	} while (pgno != cp->root);

	if ((ret = __db_c_close(dbc)) != 0)
		goto err;
	dbc = NULL;
	if (local_txn) {
		if ((ret = __txn_commit(txn, DB_TXN_NOSYNC)) != 0)
			goto err;
		txn = NULL;
	}
	if (pgno != ((BTREE *)dbp->bt_internal)->bt_root)
		goto new_txn;

err:	if (dbc != NULL && (t_ret = __bam_stkrel(dbc, 0)) != 0 && ret == 0)
		ret = t_ret;
	if (dbc != NULL && (t_ret = __db_c_close(dbc)) != 0 && ret == 0)
		ret = t_ret;
	if (local_txn &&
	    txn != NULL && (t_ret = __txn_abort(txn)) != 0 && ret == 0)
		ret = t_ret;
	if (start.data != NULL)
		__os_free(dbp->dbenv, start.data);
	return (ret);
}

static int
__bam_setup_freelist(dbp, list, nelems)
	DB *dbp;
	struct pglist *list;
	u_int32_t nelems;
{
	DB_MPOOLFILE *mpf;
	db_pgno_t *plist;
	int ret;

	mpf = dbp->mpf;

	if ((ret = __memp_alloc_freelist(mpf, nelems, &plist)) != 0)
		return (ret);

	while (nelems-- != 0)
		*plist++ = list++->pgno;

	return (0);
}

static int
__bam_free_freelist(dbp, txn)
	DB *dbp;
	DB_TXN *txn;
{
	DBC *dbc;
	DB_LOCK lock;
	int ret, t_ret;

	LOCK_INIT(lock);
	ret = 0;

	/*
	 * If we are not in a transaction then we need to get
	 * a lock on the meta page, otherwise we should already
	 * have the lock.
	 */

	dbc = NULL;
	if (IS_DB_AUTO_COMMIT(dbp, txn)) {
		/* Get a cursor so we can call __db_lget. */
		if ((ret = __db_cursor(dbp, NULL, &dbc, 0)) != 0)
			return (ret);

		if ((ret = __db_lget(dbc,
		     0, PGNO_BASE_MD, DB_LOCK_WRITE, 0, &lock)) != 0)
			goto err;
	}

	ret = __memp_free_freelist(dbp->mpf);

err:	if ((t_ret = __LPUT(dbc, lock)) != 0 && ret == 0)
		ret = t_ret;

	if (dbc != NULL && (t_ret = __db_c_close(dbc)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}
#endif
