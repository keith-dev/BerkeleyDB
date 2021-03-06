/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: db_ret.c,v 12.6 2006/08/24 14:45:16 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"

/*
 * __db_ret --
 *	Build return DBT.
 *
 * PUBLIC: int __db_ret __P((DB *, DB_TXN *,
 * PUBLIC:    PAGE *, u_int32_t, DBT *, void **, u_int32_t *));
 */
int
__db_ret(dbp, txn, h, indx, dbt, memp, memsize)
	DB *dbp;
	DB_TXN *txn;
	PAGE *h;
	u_int32_t indx;
	DBT *dbt;
	void **memp;
	u_int32_t *memsize;
{
	BKEYDATA *bk;
	HOFFPAGE ho;
	BOVERFLOW *bo;
	u_int32_t len;
	u_int8_t *hk;
	void *data;

	switch (TYPE(h)) {
	case P_HASH:
		hk = P_ENTRY(dbp, h, indx);
		if (HPAGE_PTYPE(hk) == H_OFFPAGE) {
			memcpy(&ho, hk, sizeof(HOFFPAGE));
			return (__db_goff(dbp, txn, dbt,
			    ho.tlen, ho.pgno, memp, memsize));
		}
		len = LEN_HKEYDATA(dbp, h, dbp->pgsize, indx);
		data = HKEYDATA_DATA(hk);
		break;
	case P_LBTREE:
	case P_LDUP:
	case P_LRECNO:
		bk = GET_BKEYDATA(dbp, h, indx);
		if (B_TYPE(bk->type) == B_OVERFLOW) {
			bo = (BOVERFLOW *)bk;
			return (__db_goff(dbp, txn, dbt,
			    bo->tlen, bo->pgno, memp, memsize));
		}
		len = bk->len;
		data = bk->data;
		break;
	default:
		return (__db_pgfmt(dbp->dbenv, h->pgno));
	}

	return (__db_retcopy(dbp->dbenv, dbt, data, len, memp, memsize));
}

/*
 * __db_retcopy --
 *	Copy the returned data into the user's DBT, handling special flags.
 *
 * PUBLIC: int __db_retcopy __P((DB_ENV *, DBT *,
 * PUBLIC:    void *, u_int32_t, void **, u_int32_t *));
 */
int
__db_retcopy(dbenv, dbt, data, len, memp, memsize)
	DB_ENV *dbenv;
	DBT *dbt;
	void *data;
	u_int32_t len;
	void **memp;
	u_int32_t *memsize;
{
	int ret;

	ret = 0;

	/* If returning a partial record, reset the length. */
	if (F_ISSET(dbt, DB_DBT_PARTIAL)) {
		data = (u_int8_t *)data + dbt->doff;
		if (len > dbt->doff) {
			len -= dbt->doff;
			if (len > dbt->dlen)
				len = dbt->dlen;
		} else
			len = 0;
	}

	/*
	 * Allocate memory to be owned by the application: DB_DBT_MALLOC,
	 * DB_DBT_REALLOC.
	 *
	 * !!!
	 * We always allocate memory, even if we're copying out 0 bytes. This
	 * guarantees consistency, i.e., the application can always free memory
	 * without concern as to how many bytes of the record were requested.
	 *
	 * Use the memory specified by the application: DB_DBT_USERMEM.
	 *
	 * !!!
	 * If the length we're going to copy is 0, the application-supplied
	 * memory pointer is allowed to be NULL.
	 */
	if (F_ISSET(dbt, DB_DBT_USERCOPY)) {
		dbt->size = len;
		return (len == 0 ? 0 : dbenv->dbt_usercopy(dbt, 0, data,
		    len, DB_USERCOPY_SETDATA));

	} else if (F_ISSET(dbt, DB_DBT_MALLOC))
		ret = __os_umalloc(dbenv, len, &dbt->data);
	else if (F_ISSET(dbt, DB_DBT_REALLOC)) {
		if (dbt->data == NULL || dbt->size == 0 || dbt->size < len)
			ret = __os_urealloc(dbenv, len, &dbt->data);
	} else if (F_ISSET(dbt, DB_DBT_USERMEM)) {
		if (len != 0 && (dbt->data == NULL || dbt->ulen < len))
			ret = DB_BUFFER_SMALL;
	} else if (memp == NULL || memsize == NULL)
		ret = EINVAL;
	else {
		if (len != 0 && (*memsize == 0 || *memsize < len)) {
			if ((ret = __os_realloc(dbenv, len, memp)) == 0)
				*memsize = len;
			else
				*memsize = 0;
		}
		if (ret == 0)
			dbt->data = *memp;
	}

	if (ret == 0 && len != 0)
		memcpy(dbt->data, data, len);

	/*
	 * Return the length of the returned record in the DBT size field.
	 * This satisfies the requirement that if we're using user memory
	 * and insufficient memory was provided, return the amount necessary
	 * in the size field.
	 */
	dbt->size = len;

	return (ret);
}
