/* Do not edit: automatically built by gen_rec.awk. */
#include <errno.h>
#include "db_config.h"

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <ctype.h>
#include <string.h>
#endif

#include "db_int.h"
#include "db_page.h"
#include "db_dispatch.h"
#include "db_am.h"
#include "qam.h"
#include "txn.h"

int __qam_inc_log(dbenv, txnid, ret_lsnp, flags,
	fileid, lsn)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	DB_LSN * lsn;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_qam_inc;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(*lsn);
	if ((ret = __os_malloc(logrec.size, NULL, &logrec.data)) != 0)
		return (ret);

	bp = logrec.data;
	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);
	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);
	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(bp, &fileid, sizeof(fileid));
	bp += sizeof(fileid);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__qam_inc_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__qam_inc_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __qam_inc_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]qam_inc: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__qam_inc_read(recbuf, argpp)
	void *recbuf;
	__qam_inc_args **argpp;
{
	__qam_inc_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__qam_inc_args) +
	    sizeof(DB_TXN), NULL, &argp);
	if (ret != 0)
		return (ret);
	argp->txnid = (DB_TXN *)&argp[1];
	bp = recbuf;
	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);
	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);
	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(&argp->fileid, bp, sizeof(argp->fileid));
	bp += sizeof(argp->fileid);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	*argpp = argp;
	return (0);
}

int __qam_incfirst_log(dbenv, txnid, ret_lsnp, flags,
	fileid, recno)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_recno_t recno;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_qam_incfirst;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(recno);
	if ((ret = __os_malloc(logrec.size, NULL, &logrec.data)) != 0)
		return (ret);

	bp = logrec.data;
	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);
	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);
	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(bp, &fileid, sizeof(fileid));
	bp += sizeof(fileid);
	memcpy(bp, &recno, sizeof(recno));
	bp += sizeof(recno);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__qam_incfirst_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__qam_incfirst_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __qam_incfirst_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]qam_incfirst: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\trecno: %lu\n", (u_long)argp->recno);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__qam_incfirst_read(recbuf, argpp)
	void *recbuf;
	__qam_incfirst_args **argpp;
{
	__qam_incfirst_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__qam_incfirst_args) +
	    sizeof(DB_TXN), NULL, &argp);
	if (ret != 0)
		return (ret);
	argp->txnid = (DB_TXN *)&argp[1];
	bp = recbuf;
	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);
	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);
	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(&argp->fileid, bp, sizeof(argp->fileid));
	bp += sizeof(argp->fileid);
	memcpy(&argp->recno, bp, sizeof(argp->recno));
	bp += sizeof(argp->recno);
	*argpp = argp;
	return (0);
}

int __qam_mvptr_log(dbenv, txnid, ret_lsnp, flags,
	opcode, fileid, old_first, new_first, old_cur, new_cur,
	metalsn)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	u_int32_t opcode;
	int32_t fileid;
	db_recno_t old_first;
	db_recno_t new_first;
	db_recno_t old_cur;
	db_recno_t new_cur;
	DB_LSN * metalsn;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_qam_mvptr;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(opcode)
	    + sizeof(fileid)
	    + sizeof(old_first)
	    + sizeof(new_first)
	    + sizeof(old_cur)
	    + sizeof(new_cur)
	    + sizeof(*metalsn);
	if ((ret = __os_malloc(logrec.size, NULL, &logrec.data)) != 0)
		return (ret);

	bp = logrec.data;
	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);
	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);
	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(bp, &opcode, sizeof(opcode));
	bp += sizeof(opcode);
	memcpy(bp, &fileid, sizeof(fileid));
	bp += sizeof(fileid);
	memcpy(bp, &old_first, sizeof(old_first));
	bp += sizeof(old_first);
	memcpy(bp, &new_first, sizeof(new_first));
	bp += sizeof(new_first);
	memcpy(bp, &old_cur, sizeof(old_cur));
	bp += sizeof(old_cur);
	memcpy(bp, &new_cur, sizeof(new_cur));
	bp += sizeof(new_cur);
	if (metalsn != NULL)
		memcpy(bp, metalsn, sizeof(*metalsn));
	else
		memset(bp, 0, sizeof(*metalsn));
	bp += sizeof(*metalsn);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__qam_mvptr_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__qam_mvptr_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __qam_mvptr_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]qam_mvptr: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\topcode: %lu\n", (u_long)argp->opcode);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\told_first: %lu\n", (u_long)argp->old_first);
	printf("\tnew_first: %lu\n", (u_long)argp->new_first);
	printf("\told_cur: %lu\n", (u_long)argp->old_cur);
	printf("\tnew_cur: %lu\n", (u_long)argp->new_cur);
	printf("\tmetalsn: [%lu][%lu]\n",
	    (u_long)argp->metalsn.file, (u_long)argp->metalsn.offset);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__qam_mvptr_read(recbuf, argpp)
	void *recbuf;
	__qam_mvptr_args **argpp;
{
	__qam_mvptr_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__qam_mvptr_args) +
	    sizeof(DB_TXN), NULL, &argp);
	if (ret != 0)
		return (ret);
	argp->txnid = (DB_TXN *)&argp[1];
	bp = recbuf;
	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);
	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);
	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(&argp->opcode, bp, sizeof(argp->opcode));
	bp += sizeof(argp->opcode);
	memcpy(&argp->fileid, bp, sizeof(argp->fileid));
	bp += sizeof(argp->fileid);
	memcpy(&argp->old_first, bp, sizeof(argp->old_first));
	bp += sizeof(argp->old_first);
	memcpy(&argp->new_first, bp, sizeof(argp->new_first));
	bp += sizeof(argp->new_first);
	memcpy(&argp->old_cur, bp, sizeof(argp->old_cur));
	bp += sizeof(argp->old_cur);
	memcpy(&argp->new_cur, bp, sizeof(argp->new_cur));
	bp += sizeof(argp->new_cur);
	memcpy(&argp->metalsn, bp,  sizeof(argp->metalsn));
	bp += sizeof(argp->metalsn);
	*argpp = argp;
	return (0);
}

int __qam_del_log(dbenv, txnid, ret_lsnp, flags,
	fileid, lsn, pgno, indx, recno)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	DB_LSN * lsn;
	db_pgno_t pgno;
	u_int32_t indx;
	db_recno_t recno;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_qam_del;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(*lsn)
	    + sizeof(pgno)
	    + sizeof(indx)
	    + sizeof(recno);
	if ((ret = __os_malloc(logrec.size, NULL, &logrec.data)) != 0)
		return (ret);

	bp = logrec.data;
	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);
	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);
	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(bp, &fileid, sizeof(fileid));
	bp += sizeof(fileid);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	memcpy(bp, &recno, sizeof(recno));
	bp += sizeof(recno);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__qam_del_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__qam_del_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __qam_del_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]qam_del: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\trecno: %lu\n", (u_long)argp->recno);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__qam_del_read(recbuf, argpp)
	void *recbuf;
	__qam_del_args **argpp;
{
	__qam_del_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__qam_del_args) +
	    sizeof(DB_TXN), NULL, &argp);
	if (ret != 0)
		return (ret);
	argp->txnid = (DB_TXN *)&argp[1];
	bp = recbuf;
	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);
	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);
	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(&argp->fileid, bp, sizeof(argp->fileid));
	bp += sizeof(argp->fileid);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	memcpy(&argp->recno, bp, sizeof(argp->recno));
	bp += sizeof(argp->recno);
	*argpp = argp;
	return (0);
}

int __qam_add_log(dbenv, txnid, ret_lsnp, flags,
	fileid, lsn, pgno, indx, recno, data,
	vflag, olddata)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	DB_LSN * lsn;
	db_pgno_t pgno;
	u_int32_t indx;
	db_recno_t recno;
	const DBT *data;
	u_int32_t vflag;
	const DBT *olddata;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t zero;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_qam_add;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(*lsn)
	    + sizeof(pgno)
	    + sizeof(indx)
	    + sizeof(recno)
	    + sizeof(u_int32_t) + (data == NULL ? 0 : data->size)
	    + sizeof(vflag)
	    + sizeof(u_int32_t) + (olddata == NULL ? 0 : olddata->size);
	if ((ret = __os_malloc(logrec.size, NULL, &logrec.data)) != 0)
		return (ret);

	bp = logrec.data;
	memcpy(bp, &rectype, sizeof(rectype));
	bp += sizeof(rectype);
	memcpy(bp, &txn_num, sizeof(txn_num));
	bp += sizeof(txn_num);
	memcpy(bp, lsnp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(bp, &fileid, sizeof(fileid));
	bp += sizeof(fileid);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	memcpy(bp, &recno, sizeof(recno));
	bp += sizeof(recno);
	if (data == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &data->size, sizeof(data->size));
		bp += sizeof(data->size);
		memcpy(bp, data->data, data->size);
		bp += data->size;
	}
	memcpy(bp, &vflag, sizeof(vflag));
	bp += sizeof(vflag);
	if (olddata == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &olddata->size, sizeof(olddata->size));
		bp += sizeof(olddata->size);
		memcpy(bp, olddata->data, olddata->size);
		bp += olddata->size;
	}
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__qam_add_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__qam_add_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __qam_add_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]qam_add: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\trecno: %lu\n", (u_long)argp->recno);
	printf("\tdata: ");
	for (i = 0; i < argp->data.size; i++) {
		ch = ((u_int8_t *)argp->data.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\tvflag: %lu\n", (u_long)argp->vflag);
	printf("\tolddata: ");
	for (i = 0; i < argp->olddata.size; i++) {
		ch = ((u_int8_t *)argp->olddata.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__qam_add_read(recbuf, argpp)
	void *recbuf;
	__qam_add_args **argpp;
{
	__qam_add_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__qam_add_args) +
	    sizeof(DB_TXN), NULL, &argp);
	if (ret != 0)
		return (ret);
	argp->txnid = (DB_TXN *)&argp[1];
	bp = recbuf;
	memcpy(&argp->type, bp, sizeof(argp->type));
	bp += sizeof(argp->type);
	memcpy(&argp->txnid->txnid,  bp, sizeof(argp->txnid->txnid));
	bp += sizeof(argp->txnid->txnid);
	memcpy(&argp->prev_lsn, bp, sizeof(DB_LSN));
	bp += sizeof(DB_LSN);
	memcpy(&argp->fileid, bp, sizeof(argp->fileid));
	bp += sizeof(argp->fileid);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	memcpy(&argp->recno, bp, sizeof(argp->recno));
	bp += sizeof(argp->recno);
	memset(&argp->data, 0, sizeof(argp->data));
	memcpy(&argp->data.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->data.data = bp;
	bp += argp->data.size;
	memcpy(&argp->vflag, bp, sizeof(argp->vflag));
	bp += sizeof(argp->vflag);
	memset(&argp->olddata, 0, sizeof(argp->olddata));
	memcpy(&argp->olddata.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->olddata.data = bp;
	bp += argp->olddata.size;
	*argpp = argp;
	return (0);
}

int
__qam_init_print(dbenv)
	DB_ENV *dbenv;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv,
	    __qam_inc_print, DB_qam_inc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_incfirst_print, DB_qam_incfirst)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_mvptr_print, DB_qam_mvptr)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_del_print, DB_qam_del)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_add_print, DB_qam_add)) != 0)
		return (ret);
	return (0);
}

/*
 * PUBLIC: int __qam_init_recover __P((DB_ENV *));
 */
int
__qam_init_recover(dbenv)
	DB_ENV *dbenv;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv,
	    __qam_inc_recover, DB_qam_inc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_incfirst_recover, DB_qam_incfirst)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_mvptr_recover, DB_qam_mvptr)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_del_recover, DB_qam_del)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __qam_add_recover, DB_qam_add)) != 0)
		return (ret);
	return (0);
}

