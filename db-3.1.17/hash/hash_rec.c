/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 */
/*
 * Copyright (c) 1995, 1996
 *	Margo Seltzer.  All rights reserved.
 */
/*
 * Copyright (c) 1995, 1996
 *	The President and Fellows of Harvard University.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Margo Seltzer.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "db_config.h"

#ifndef lint
static const char revid[] = "$Id: hash_rec.c,v 11.23 2000/03/29 20:50:47 ubell Exp $";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <errno.h>
#include <string.h>
#endif

#include "db_int.h"
#include "db_page.h"
#include "db_shash.h"
#include "btree.h"
#include "hash.h"
#include "lock.h"
#include "log.h"
#include "mp.h"

static int __ham_alloc_pages __P((DB *, __ham_groupalloc_args *));
static int __ham_free_pages __P((DB *, DBMETA *, __ham_groupalloc_args *));

/*
 * __ham_insdel_recover --
 *
 * PUBLIC: int __ham_insdel_recover
 * PUBLIC:     __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_insdel_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_insdel_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	u_int32_t opcode;
	int cmp_n, cmp_p, getmeta, ret, type;

	COMPQUIET(info, NULL);

	getmeta = 0;
	REC_PRINT(__ham_insdel_print);
	REC_INTRO(__ham_insdel_read, 1);

	if ((ret = memp_fget(mpf, &argp->pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			goto done;
		} else if ((ret = memp_fget(mpf, &argp->pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	if ((ret = __ham_get_meta(dbc)) != 0)
		goto out;
	getmeta = 1;

	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->pagelsn);
	/*
	 * Two possible things going on:
	 * redo a delete/undo a put: delete the item from the page.
	 * redo a put/undo a delete: add the item to the page.
	 * If we are undoing a delete, then the information logged is the
	 * entire entry off the page, not just the data of a dbt.  In
	 * this case, we want to copy it back onto the page verbatim.
	 * We do this by calling __putitem with the type H_OFFPAGE instead
	 * of H_KEYDATA.
	 */
	opcode = OPCODE_OF(argp->opcode);

	if ((opcode == DELPAIR && cmp_n == 0 && DB_UNDO(op)) ||
	    (opcode == PUTPAIR && cmp_p == 0 && DB_REDO(op))) {
		/*
		 * Need to redo a PUT or undo a delete.  If we are undoing a
		 * delete, we've got to restore the item back to its original
		 * position.  That's a royal pain in the butt (because we do
		 * not store item lengths on the page), but there's no choice.
		 */
		if (opcode != DELPAIR ||
		    argp->ndx == (u_int32_t)NUM_ENT(pagep)) {
			__ham_putitem(pagep, &argp->key,
			    DB_UNDO(op) || PAIR_ISKEYBIG(argp->opcode) ?
			    H_OFFPAGE : H_KEYDATA);

			if (PAIR_ISDATADUP(argp->opcode))
				type = H_DUPLICATE;
			else if (DB_UNDO(op) || PAIR_ISDATABIG(argp->opcode))
				type = H_OFFPAGE;
			else
				type = H_KEYDATA;
			__ham_putitem(pagep, &argp->data, type);
		} else
			(void) __ham_reputpair(pagep, file_dbp->pgsize,
			    argp->ndx, &argp->key, &argp->data);

		LSN(pagep) = DB_REDO(op) ? *lsnp : argp->pagelsn;
		if ((ret = __ham_put_page(file_dbp, pagep, 1)) != 0)
			goto out;

	} else if ((opcode == DELPAIR && cmp_p == 0 && DB_REDO(op))
	    || (opcode == PUTPAIR && cmp_n == 0 && DB_UNDO(op))) {
		/* Need to undo a put or redo a delete. */
		__ham_dpair(file_dbp, pagep, argp->ndx);
		LSN(pagep) = DB_REDO(op) ? *lsnp : argp->pagelsn;
		if ((ret = __ham_put_page(file_dbp, (PAGE *)pagep, 1)) != 0)
			goto out;
	} else
		if ((ret = __ham_put_page(file_dbp, (PAGE *)pagep, 0)) != 0)
			goto out;

	/* Return the previous LSN. */
done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (getmeta)
		(void)__ham_release_meta(dbc);
	REC_CLOSE;
}

/*
 * __ham_newpage_recover --
 *	This log message is used when we add/remove overflow pages.  This
 *	message takes care of the pointer chains, not the data on the pages.
 *
 * PUBLIC: int __ham_newpage_recover
 * PUBLIC:     __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_newpage_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_newpage_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, change, getmeta, ret;

	COMPQUIET(info, NULL);

	getmeta = 0;
	REC_PRINT(__ham_newpage_print);
	REC_INTRO(__ham_newpage_read, 1);

	if ((ret = memp_fget(mpf, &argp->new_pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			ret = 0;
			goto ppage;
		} else if ((ret = memp_fget(mpf, &argp->new_pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	if ((ret = __ham_get_meta(dbc)) != 0)
		goto out;
	getmeta = 1;

	/*
	 * There are potentially three pages we need to check: the one
	 * that we created/deleted, the one before it and the one after
	 * it.
	 */

	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->pagelsn);
	change = 0;

	if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == PUTOVFL) ||
	    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == DELOVFL)) {
		/* Redo a create new page or undo a delete new page. */
		P_INIT(pagep, file_dbp->pgsize, argp->new_pgno,
		    argp->prev_pgno, argp->next_pgno, 0, P_HASH);
		change = 1;
	} else if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == DELOVFL) ||
	    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == PUTOVFL)) {
		/*
		 * Redo a delete or undo a create new page.  All we
		 * really need to do is change the LSN.
		 */
		change = 1;
	}

	if (!change) {
		if ((ret = __ham_put_page(file_dbp, (PAGE *)pagep, 0)) != 0)
			goto out;
	} else {
		LSN(pagep) = DB_REDO(op) ? *lsnp : argp->pagelsn;
		if ((ret = __ham_put_page(file_dbp, (PAGE *)pagep, 1)) != 0)
			goto out;
	}

	/* Now do the prev page. */
ppage:	if (argp->prev_pgno != PGNO_INVALID) {
		if ((ret = memp_fget(mpf, &argp->prev_pgno, 0, &pagep)) != 0) {
			if (DB_UNDO(op)) {
				/*
				 * We are undoing and the page doesn't exist.
				 * That is equivalent to having a pagelsn of 0,
				 * so we would not have to undo anything.  In
				 * this case, don't bother creating a page.
				 */
				ret = 0;
				goto npage;
			} else if ((ret =
			    memp_fget(mpf, &argp->prev_pgno,
			    DB_MPOOL_CREATE, &pagep)) != 0)
				goto out;
		}

		cmp_n = log_compare(lsnp, &LSN(pagep));
		cmp_p = log_compare(&LSN(pagep), &argp->prevlsn);
		CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->prevlsn);
		change = 0;

		if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == PUTOVFL) ||
		    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == DELOVFL)) {
			/* Redo a create new page or undo a delete new page. */
			pagep->next_pgno = argp->new_pgno;
			change = 1;
		} else if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == DELOVFL) ||
		    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == PUTOVFL)) {
			/* Redo a delete or undo a create new page. */
			pagep->next_pgno = argp->next_pgno;
			change = 1;
		}

		if (!change) {
			if ((ret =
			    __ham_put_page(file_dbp, (PAGE *)pagep, 0)) != 0)
				goto out;
		} else {
			LSN(pagep) = DB_REDO(op) ? *lsnp : argp->prevlsn;
			if ((ret =
			    __ham_put_page(file_dbp, (PAGE *)pagep, 1)) != 0)
				goto out;
		}
	}

	/* Now time to do the next page */
npage:	if (argp->next_pgno != PGNO_INVALID) {
		if ((ret = memp_fget(mpf, &argp->next_pgno, 0, &pagep)) != 0) {
			if (DB_UNDO(op)) {
				/*
				 * We are undoing and the page doesn't exist.
				 * That is equivalent to having a pagelsn of 0,
				 * so we would not have to undo anything.  In
				 * this case, don't bother creating a page.
				 */
				goto done;
			} else if ((ret =
			    memp_fget(mpf, &argp->next_pgno,
			    DB_MPOOL_CREATE, &pagep)) != 0)
				goto out;
		}

		cmp_n = log_compare(lsnp, &LSN(pagep));
		cmp_p = log_compare(&LSN(pagep), &argp->nextlsn);
		CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->nextlsn);
		change = 0;

		if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == PUTOVFL) ||
		    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == DELOVFL)) {
			/* Redo a create new page or undo a delete new page. */
			pagep->prev_pgno = argp->new_pgno;
			change = 1;
		} else if ((cmp_p == 0 && DB_REDO(op) && argp->opcode == DELOVFL) ||
		    (cmp_n == 0 && DB_UNDO(op) && argp->opcode == PUTOVFL)) {
			/* Redo a delete or undo a create new page. */
			pagep->prev_pgno = argp->prev_pgno;
			change = 1;
		}

		if (!change) {
			if ((ret =
			    __ham_put_page(file_dbp, (PAGE *)pagep, 0)) != 0)
				goto out;
		} else {
			LSN(pagep) = DB_REDO(op) ? *lsnp : argp->nextlsn;
			if ((ret =
			    __ham_put_page(file_dbp, (PAGE *)pagep, 1)) != 0)
				goto out;
		}
	}
done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (getmeta)
		(void)__ham_release_meta(dbc);
	REC_CLOSE;
}

/*
 * __ham_replace_recover --
 *	This log message refers to partial puts that are local to a single
 *	page.  You can think of them as special cases of the more general
 *	insdel log message.
 *
 * PUBLIC: int __ham_replace_recover
 * PUBLIC:    __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_replace_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_replace_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	DBT dbt;
	PAGE *pagep;
	int32_t grow;
	int change, cmp_n, cmp_p, getmeta, ret;
	u_int8_t *hk;

	COMPQUIET(info, NULL);

	getmeta = 0;
	REC_PRINT(__ham_replace_print);
	REC_INTRO(__ham_replace_read, 1);

	if ((ret = memp_fget(mpf, &argp->pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			goto done;
		} else if ((ret = memp_fget(mpf, &argp->pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	if ((ret = __ham_get_meta(dbc)) != 0)
		goto out;
	getmeta = 1;

	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->pagelsn);

	memset(&dbt, 0, sizeof(dbt));
	if (cmp_p == 0 && DB_REDO(op)) {
		change = 1;
		/* Reapply the change as specified. */
		dbt.data = argp->newitem.data;
		dbt.size = argp->newitem.size;
		grow = argp->newitem.size - argp->olditem.size;
		LSN(pagep) = *lsnp;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		change = 1;
		/* Undo the already applied change. */
		dbt.data = argp->olditem.data;
		dbt.size = argp->olditem.size;
		grow = argp->olditem.size - argp->newitem.size;
		LSN(pagep) = argp->pagelsn;
	} else {
		change = 0;
		grow = 0;
	}

	if (change) {
		__ham_onpage_replace(pagep,
		    file_dbp->pgsize, argp->ndx, argp->off, grow, &dbt);
		if (argp->makedup) {
			hk = P_ENTRY(pagep, argp->ndx);
			if (DB_REDO(op))
				HPAGE_PTYPE(hk) = H_DUPLICATE;
			else
				HPAGE_PTYPE(hk) = H_KEYDATA;
		}
	}

	if ((ret = __ham_put_page(file_dbp, pagep, change)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (getmeta)
		(void)__ham_release_meta(dbc);
	REC_CLOSE;
}

/*
 * __ham_splitdata_recover --
 *
 * PUBLIC: int __ham_splitdata_recover
 * PUBLIC:    __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_splitdata_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_splitdata_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int change, cmp_n, cmp_p, getmeta, ret;

	COMPQUIET(info, NULL);

	getmeta = 0;
	REC_PRINT(__ham_splitdata_print);
	REC_INTRO(__ham_splitdata_read, 1);

	if ((ret = memp_fget(mpf, &argp->pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			goto done;
		} else if ((ret = memp_fget(mpf, &argp->pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	if ((ret = __ham_get_meta(dbc)) != 0)
		goto out;
	getmeta = 1;

	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->pagelsn);

	/*
	 * There are two types of log messages here, one for the old page
	 * and one for the new pages created.  The original image in the
	 * SPLITOLD record is used for undo.  The image in the SPLITNEW
	 * is used for redo.  We should never have a case where there is
	 * a redo operation and the SPLITOLD record is on disk, but not
	 * the SPLITNEW record.  Therefore, we only have work to do when
	 * redo NEW messages and undo OLD messages, but we have to update
	 * LSNs in both cases.
	 */
	change = 0;
	if (cmp_p == 0 && DB_REDO(op)) {
		if (argp->opcode == SPLITNEW)
			/* Need to redo the split described. */
			memcpy(pagep, argp->pageimage.data,
			    argp->pageimage.size);
		LSN(pagep) = *lsnp;
		change = 1;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		if (argp->opcode == SPLITOLD) {
			/* Put back the old image. */
			memcpy(pagep, argp->pageimage.data,
			    argp->pageimage.size);
		} else
			P_INIT(pagep, file_dbp->pgsize, argp->pgno,
			    PGNO_INVALID, PGNO_INVALID, 0, P_HASH);
		LSN(pagep) = argp->pagelsn;
		change = 1;
	}
	if ((ret = __ham_put_page(file_dbp, pagep, change)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (getmeta)
		(void)__ham_release_meta(dbc);
	REC_CLOSE;
}

/*
 * __ham_copypage_recover --
 *	Recovery function for copypage.
 *
 * PUBLIC: int __ham_copypage_recover
 * PUBLIC:   __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_copypage_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_copypage_args *argp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	int cmp_n, cmp_p, getmeta, modified, ret;

	COMPQUIET(info, NULL);

	getmeta = 0;
	REC_PRINT(__ham_copypage_print);
	REC_INTRO(__ham_copypage_read, 1);

	if ((ret = __ham_get_meta(dbc)) != 0)
		goto out;
	getmeta = 1;
	modified = 0;

	/* This is the bucket page. */
	if ((ret = memp_fget(mpf, &argp->pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			ret = 0;
			goto donext;
		} else if ((ret = memp_fget(mpf, &argp->pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->pagelsn);

	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		memcpy(pagep, argp->page.data, argp->page.size);
		LSN(pagep) = *lsnp;
		modified = 1;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		P_INIT(pagep, file_dbp->pgsize, argp->pgno, PGNO_INVALID,
		    argp->next_pgno, 0, P_HASH);
		LSN(pagep) = argp->pagelsn;
		modified = 1;
	}
	if ((ret = memp_fput(mpf, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

donext:	/* Now fix up the "next" page. */
	if ((ret = memp_fget(mpf, &argp->next_pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			ret = 0;
			goto do_nn;
		} else if ((ret = memp_fget(mpf, &argp->next_pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	modified = 0;
	/* For REDO just update the LSN. For UNDO copy page back. */
	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->nextlsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->nextlsn);
	if (cmp_p == 0 && DB_REDO(op)) {
		LSN(pagep) = *lsnp;
		modified = 1;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		memcpy(pagep, argp->page.data, argp->page.size);
		modified = 1;
	}
	if ((ret = memp_fput(mpf, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

	/* Now fix up the next's next page. */
do_nn:	if (argp->nnext_pgno == PGNO_INVALID)
		goto done;

	if ((ret = memp_fget(mpf, &argp->nnext_pgno, 0, &pagep)) != 0) {
		if (DB_UNDO(op)) {
			/*
			 * We are undoing and the page doesn't exist.  That
			 * is equivalent to having a pagelsn of 0, so we
			 * would not have to undo anything.  In this case,
			 * don't bother creating a page.
			 */
			goto done;
		} else if ((ret = memp_fget(mpf, &argp->nnext_pgno,
		    DB_MPOOL_CREATE, &pagep)) != 0)
			goto out;
	}

	modified = 0;
	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->nnextlsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->nnextlsn);

	if (cmp_p == 0 && DB_REDO(op)) {
		/* Need to redo update described. */
		PREV_PGNO(pagep) = argp->pgno;
		LSN(pagep) = *lsnp;
		modified = 1;
	} else if (cmp_n == 0 && DB_UNDO(op)) {
		/* Need to undo update described. */
		PREV_PGNO(pagep) = argp->next_pgno;
		LSN(pagep) = argp->nnextlsn;
		modified = 1;
	}
	if ((ret = memp_fput(mpf, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	if (getmeta)
		(void)__ham_release_meta(dbc);
	REC_CLOSE;
}

/*
 * __ham_metagroup_recover --
 *	Recovery function for metagroup.
 *
 * PUBLIC: int __ham_metagroup_recover
 * PUBLIC:   __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_metagroup_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_metagroup_args *argp;
	HASH_CURSOR *hcp;
	DB *file_dbp;
	DBC *dbc;
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pgno_t last_pgno;
	int cmp_n, cmp_p, groupgrow, modified, ret;

	COMPQUIET(info, NULL);
	REC_PRINT(__ham_metagroup_print);
	REC_INTRO(__ham_metagroup_read, 1);

	/*
	 * This logs the virtual create of pages pgno to pgno + bucket
	 * Since the mpool page-allocation is not really able to be
	 * transaction protected, we can never undo it.  Even in an abort,
	 * we have to allocate these pages to the hash table.
	 * The log record contains:
	 * bucket: new bucket being allocated.
	 * pgno: page number of the new bucket.
	 * if bucket is a power of 2, then we allocated a whole batch of
	 * pages; if it's not, then we simply allocated one new page.
	 */
	groupgrow =
	    (u_int32_t)(1 << __db_log2(argp->bucket + 1)) == argp->bucket + 1;

	last_pgno = argp->pgno;
	if (groupgrow)
		/* Read the last page. */
		last_pgno += argp->bucket;

	if ((ret = memp_fget(mpf, &last_pgno, DB_MPOOL_CREATE, &pagep)) != 0)
		goto out;

	modified = 0;
	cmp_n = log_compare(lsnp, &LSN(pagep));
	cmp_p = log_compare(&LSN(pagep), &argp->pagelsn);
	CHECK_LSN(op, cmp_p, &LSN(pagep), &argp->pagelsn);

	if ((cmp_p == 0 && DB_REDO(op)) || (cmp_n == 0 && DB_UNDO(op))) {
		/*
		 * We need to make sure that we redo the allocation of the
		 * pages.
		 */
		if (DB_REDO(op))
			pagep->lsn = *lsnp;
		else
			pagep->lsn = argp->pagelsn;
		modified = 1;
	}
	if ((ret = memp_fput(mpf, pagep, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

	/* Now we have to update the meta-data page. */
	hcp = (HASH_CURSOR *)dbc->internal;
	if ((ret = __ham_get_meta(dbc)) != 0)
		goto out;
	cmp_n = log_compare(lsnp, &hcp->hdr->dbmeta.lsn);
	cmp_p = log_compare(&hcp->hdr->dbmeta.lsn, &argp->metalsn);
	CHECK_LSN(op, cmp_p, &hcp->hdr->dbmeta.lsn, &argp->metalsn);
	if ((cmp_p == 0 && DB_REDO(op)) || (cmp_n == 0 && DB_UNDO(op))) {
		if (DB_REDO(op)) {
			/* Redo the actual updating of bucket counts. */
			++hcp->hdr->max_bucket;
			if (groupgrow) {
				hcp->hdr->low_mask = hcp->hdr->high_mask;
				hcp->hdr->high_mask =
				    (argp->bucket + 1) | hcp->hdr->low_mask;
			}
			hcp->hdr->dbmeta.lsn = *lsnp;
		} else {
			/* Undo the actual updating of bucket counts. */
			--hcp->hdr->max_bucket;
			if (groupgrow) {
				hcp->hdr->high_mask = hcp->hdr->low_mask;
				hcp->hdr->low_mask = hcp->hdr->high_mask >> 1;
			}
			hcp->hdr->dbmeta.lsn = argp->metalsn;
		}
		if (groupgrow &&
		    hcp->hdr->spares[__db_log2(argp->bucket + 1) + 1] ==
		    PGNO_INVALID)
			hcp->hdr->spares[__db_log2(argp->bucket + 1) + 1] =
			    argp->pgno - argp->bucket - 1;
		F_SET(hcp, H_DIRTY);
	}
	if ((ret = __ham_release_meta(dbc)) != 0)
		goto out;

done:	*lsnp = argp->prev_lsn;
	ret = 0;

out:	REC_CLOSE;
}

/*
 * __ham_groupalloc_recover --
 *	Recover the batch creation of a set of pages for a new database.
 *
 * PUBLIC: int __ham_groupalloc_recover
 * PUBLIC:   __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
 */
int
__ham_groupalloc_recover(dbenv, dbtp, lsnp, op, info)
	DB_ENV *dbenv;
	DBT *dbtp;
	DB_LSN *lsnp;
	db_recops op;
	void *info;
{
	__ham_groupalloc_args *argp;
	DBMETA *mmeta;
	DB_MPOOLFILE *mpf;
	DB *file_dbp;
	DBC *dbc;
	db_pgno_t pgno;
	int cmp_n, cmp_p, modified, ret;

	modified = 0;
	COMPQUIET(info, NULL);
	REC_PRINT(__ham_groupalloc_print);
	REC_INTRO(__ham_groupalloc_read, 0);

	pgno = PGNO_BASE_MD;
	if ((ret = memp_fget(mpf, &pgno, 0, &mmeta)) != 0) {
		if (DB_REDO(op)) {
			/* Page should have existed. */
			(void)__db_pgerr(file_dbp, pgno);
			goto out;
		} else {
			ret = 0;
			goto done;
		}
	}

	cmp_n = log_compare(lsnp, &mmeta->alloc_lsn);
	cmp_p = log_compare(&mmeta->alloc_lsn, &argp->alloc_lsn);
	CHECK_LSN(op, cmp_p, &mmeta->alloc_lsn, &argp->alloc_lsn);

	/*
	 * Basically, we used mpool to allocate a chunk of pages.
	 * We need to either add those to a free list (in the undo
	 * case) or initialize them (in the redo case).
	 *
	 * If we are redoing and this is a hash subdatabase, it's possible
	 * that the pages were never allocated, so we'd better check for
	 * that and handle it here.
	 */

	if (DB_REDO(op)) {
		if ((ret = __ham_alloc_pages(file_dbp, argp)) != 0)
			goto out1;
		if (cmp_p == 0) {
			mmeta->alloc_lsn = *lsnp;
			if (log_compare(&LSN(mmeta), &argp->meta_lsn) == 0)
				LSN(mmeta) = *lsnp;
			modified = 1;
		}
	}

	/*
	 * We must determine if this is the first allocation to be
	 * undone.  If the LSN is not right, then it must be the first.
	 * If the LSN is correct, then look to see if the pages allocated
	 * by this log record are at the head of the list. If so then we
	 * have previously undone this allocation and we are starting over.
	 * If not, we have not undone this allocation and some previous
	 * undo of an allocation has put us in the right state.  We
	 * just undo the allocation by putting the pages on the free list.
	 */
	else if (DB_UNDO(op)) {
		if (cmp_n != 0 ||
		    (mmeta->free >= argp->start_pgno &&
		    mmeta->free < argp->start_pgno + argp->num))
			mmeta->free = argp->free;
		if ((ret = __ham_free_pages(file_dbp, mmeta, argp)) != 0)
			goto out1;
		mmeta->alloc_lsn = argp->alloc_lsn;
		if (log_compare(lsnp, &LSN(mmeta)) == 0)
			LSN(mmeta) = argp->meta_lsn;
		modified = 1;
	}

out1:	if ((ret = memp_fput(mpf, mmeta, modified ? DB_MPOOL_DIRTY : 0)) != 0)
		goto out;

done:	if (ret == 0)
		*lsnp = argp->prev_lsn;

out:	REC_CLOSE;
}

/*
 * __ham_free_pages --
 *
 * Called during abort/undo of a file create.  We create new pages in the file
 * using the MPOOL_NEW_GROUP flag.  We then log the meta-data page with a
 * __crdel_metasub message.  If we fail we need to take those newly allocated
 * pages and put them on a free list.  Normally this would happen in the
 * recovery for __db_new, but that doesn't get called in this case.
 * We do not look at LSNs here because we are forcing the pages to their
 * deallocated state.  If they were reallocated after this, they
 * will get rolled forward later.
 */
static int
__ham_free_pages(dbp, mmeta, argp)
	DB *dbp;
	DBMETA *mmeta;
	__ham_groupalloc_args *argp;
{
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	u_int32_t i;
	db_pgno_t last_free, pgno;
	int ret;

	mpf = dbp->mpf;
	last_free = mmeta->free;

	for (i = 0; i < argp->num; i++, last_free = pgno) {
		pgno = argp->start_pgno + i;
		if ((ret =
		    memp_fget(mpf, &pgno, DB_MPOOL_CREATE, &pagep)) != 0) {
			(void)__db_pgerr(dbp, pgno);
			return (ret);
		}

		/* Fix up the allocated page. */
		P_INIT(pagep,
		    dbp->pgsize, pgno, PGNO_INVALID, last_free, 0, P_INVALID);
		ZERO_LSN(pagep->lsn);

		if ((ret = memp_fput(mpf, pagep, DB_MPOOL_DIRTY)) != 0)
			return (ret);
	}

	mmeta->free = last_free;
	return (0);
}

/*
 * __ham_alloc_pages --
 *
 * Called during redo of a file create.  We create new pages in the file
 * using the MPOOL_NEW_GROUP flag.  We then log the meta-data page with a
 * __crdel_metasub message.  If we manage to crash without the newly written
 * pages getting to disk (I'm not sure this can happen anywhere except our
 * test suite?!), then we need to go through a recreate the final pages.
 * Hash normally has holes in its files and handles them appropriately.
 */
static int
__ham_alloc_pages(dbp, argp)
	DB *dbp;
	__ham_groupalloc_args *argp;
{
	DB_MPOOLFILE *mpf;
	PAGE *pagep;
	db_pgno_t pgno;
	int ret;

	mpf = dbp->mpf;

	/* Read the last page of the allocation. */
	pgno = argp->start_pgno + argp->num - 1;

	/* If the page exists, and it has been initialized, then we're done. */
	if ((ret = memp_fget(mpf, &pgno, 0, &pagep)) == 0) {
		if ((pagep->type == P_INVALID) && IS_ZERO_LSN(pagep->lsn))
			goto reinit_page;
		if ((ret = memp_fput(mpf, pagep, 0)) != 0)
			return (ret);
		return (0);
	}

	/*
	 * Had to create the page.  On some systems (read "Windows"),
	 * you can find random garbage on pages to which you haven't
	 * yet written.  So, we have an os layer that will do the
	 * right thing for group allocations.  We call that directly
	 * to make sure all the pages are allocated and then continue
	 * merrily on our way with normal recovery.
	 */
	if ((ret = __os_fpinit(dbp->dbenv, &mpf->fh,
	    argp->start_pgno, argp->num, dbp->pgsize)) != 0)
		return (ret);

	if ((ret = memp_fget(mpf, &pgno, DB_MPOOL_CREATE, &pagep)) != 0) {
		(void)__db_pgerr(dbp, pgno);
		return (ret);
	}

reinit_page:
	/* Initialize the newly allocated page. */
	P_INIT(pagep,
	    dbp->pgsize, pgno, PGNO_INVALID, PGNO_INVALID, 0, P_HASH);
	ZERO_LSN(pagep->lsn);

	if ((ret = memp_fput(mpf, pagep, DB_MPOOL_DIRTY)) != 0)
		return (ret);

	return (0);
}
