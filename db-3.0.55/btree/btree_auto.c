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
#include "btree.h"
#include "txn.h"

int __bam_pg_alloc_log(dbenv, txnid, ret_lsnp, flags,
	fileid, meta_lsn, page_lsn, pgno, ptype, next)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	DB_LSN * meta_lsn;
	DB_LSN * page_lsn;
	db_pgno_t pgno;
	u_int32_t ptype;
	db_pgno_t next;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_bam_pg_alloc;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(*meta_lsn)
	    + sizeof(*page_lsn)
	    + sizeof(pgno)
	    + sizeof(ptype)
	    + sizeof(next);
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
	if (meta_lsn != NULL)
		memcpy(bp, meta_lsn, sizeof(*meta_lsn));
	else
		memset(bp, 0, sizeof(*meta_lsn));
	bp += sizeof(*meta_lsn);
	if (page_lsn != NULL)
		memcpy(bp, page_lsn, sizeof(*page_lsn));
	else
		memset(bp, 0, sizeof(*page_lsn));
	bp += sizeof(*page_lsn);
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	memcpy(bp, &ptype, sizeof(ptype));
	bp += sizeof(ptype);
	memcpy(bp, &next, sizeof(next));
	bp += sizeof(next);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_pg_alloc_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_pg_alloc_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_pg_alloc_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_pg_alloc: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tmeta_lsn: [%lu][%lu]\n",
	    (u_long)argp->meta_lsn.file, (u_long)argp->meta_lsn.offset);
	printf("\tpage_lsn: [%lu][%lu]\n",
	    (u_long)argp->page_lsn.file, (u_long)argp->page_lsn.offset);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tptype: %lu\n", (u_long)argp->ptype);
	printf("\tnext: %lu\n", (u_long)argp->next);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_pg_alloc_read(recbuf, argpp)
	void *recbuf;
	__bam_pg_alloc_args **argpp;
{
	__bam_pg_alloc_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_pg_alloc_args) +
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
	memcpy(&argp->meta_lsn, bp,  sizeof(argp->meta_lsn));
	bp += sizeof(argp->meta_lsn);
	memcpy(&argp->page_lsn, bp,  sizeof(argp->page_lsn));
	bp += sizeof(argp->page_lsn);
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->ptype, bp, sizeof(argp->ptype));
	bp += sizeof(argp->ptype);
	memcpy(&argp->next, bp, sizeof(argp->next));
	bp += sizeof(argp->next);
	*argpp = argp;
	return (0);
}

int __bam_pg_free_log(dbenv, txnid, ret_lsnp, flags,
	fileid, pgno, meta_lsn, header, next)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t pgno;
	DB_LSN * meta_lsn;
	const DBT *header;
	db_pgno_t next;
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
	rectype = DB_bam_pg_free;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(pgno)
	    + sizeof(*meta_lsn)
	    + sizeof(u_int32_t) + (header == NULL ? 0 : header->size)
	    + sizeof(next);
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
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	if (meta_lsn != NULL)
		memcpy(bp, meta_lsn, sizeof(*meta_lsn));
	else
		memset(bp, 0, sizeof(*meta_lsn));
	bp += sizeof(*meta_lsn);
	if (header == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &header->size, sizeof(header->size));
		bp += sizeof(header->size);
		memcpy(bp, header->data, header->size);
		bp += header->size;
	}
	memcpy(bp, &next, sizeof(next));
	bp += sizeof(next);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_pg_free_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_pg_free_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_pg_free_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_pg_free: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tmeta_lsn: [%lu][%lu]\n",
	    (u_long)argp->meta_lsn.file, (u_long)argp->meta_lsn.offset);
	printf("\theader: ");
	for (i = 0; i < argp->header.size; i++) {
		ch = ((u_int8_t *)argp->header.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\tnext: %lu\n", (u_long)argp->next);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_pg_free_read(recbuf, argpp)
	void *recbuf;
	__bam_pg_free_args **argpp;
{
	__bam_pg_free_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_pg_free_args) +
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
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->meta_lsn, bp,  sizeof(argp->meta_lsn));
	bp += sizeof(argp->meta_lsn);
	memset(&argp->header, 0, sizeof(argp->header));
	memcpy(&argp->header.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->header.data = bp;
	bp += argp->header.size;
	memcpy(&argp->next, bp, sizeof(argp->next));
	bp += sizeof(argp->next);
	*argpp = argp;
	return (0);
}

int __bam_split_log(dbenv, txnid, ret_lsnp, flags,
	fileid, left, llsn, right, rlsn, indx,
	npgno, nlsn, pg)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t left;
	DB_LSN * llsn;
	db_pgno_t right;
	DB_LSN * rlsn;
	u_int32_t indx;
	db_pgno_t npgno;
	DB_LSN * nlsn;
	const DBT *pg;
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
	rectype = DB_bam_split;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(left)
	    + sizeof(*llsn)
	    + sizeof(right)
	    + sizeof(*rlsn)
	    + sizeof(indx)
	    + sizeof(npgno)
	    + sizeof(*nlsn)
	    + sizeof(u_int32_t) + (pg == NULL ? 0 : pg->size);
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
	memcpy(bp, &left, sizeof(left));
	bp += sizeof(left);
	if (llsn != NULL)
		memcpy(bp, llsn, sizeof(*llsn));
	else
		memset(bp, 0, sizeof(*llsn));
	bp += sizeof(*llsn);
	memcpy(bp, &right, sizeof(right));
	bp += sizeof(right);
	if (rlsn != NULL)
		memcpy(bp, rlsn, sizeof(*rlsn));
	else
		memset(bp, 0, sizeof(*rlsn));
	bp += sizeof(*rlsn);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	memcpy(bp, &npgno, sizeof(npgno));
	bp += sizeof(npgno);
	if (nlsn != NULL)
		memcpy(bp, nlsn, sizeof(*nlsn));
	else
		memset(bp, 0, sizeof(*nlsn));
	bp += sizeof(*nlsn);
	if (pg == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &pg->size, sizeof(pg->size));
		bp += sizeof(pg->size);
		memcpy(bp, pg->data, pg->size);
		bp += pg->size;
	}
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_split_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_split_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_split_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_split: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tleft: %lu\n", (u_long)argp->left);
	printf("\tllsn: [%lu][%lu]\n",
	    (u_long)argp->llsn.file, (u_long)argp->llsn.offset);
	printf("\tright: %lu\n", (u_long)argp->right);
	printf("\trlsn: [%lu][%lu]\n",
	    (u_long)argp->rlsn.file, (u_long)argp->rlsn.offset);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\tnpgno: %lu\n", (u_long)argp->npgno);
	printf("\tnlsn: [%lu][%lu]\n",
	    (u_long)argp->nlsn.file, (u_long)argp->nlsn.offset);
	printf("\tpg: ");
	for (i = 0; i < argp->pg.size; i++) {
		ch = ((u_int8_t *)argp->pg.data)[i];
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
__bam_split_read(recbuf, argpp)
	void *recbuf;
	__bam_split_args **argpp;
{
	__bam_split_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_split_args) +
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
	memcpy(&argp->left, bp, sizeof(argp->left));
	bp += sizeof(argp->left);
	memcpy(&argp->llsn, bp,  sizeof(argp->llsn));
	bp += sizeof(argp->llsn);
	memcpy(&argp->right, bp, sizeof(argp->right));
	bp += sizeof(argp->right);
	memcpy(&argp->rlsn, bp,  sizeof(argp->rlsn));
	bp += sizeof(argp->rlsn);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	memcpy(&argp->npgno, bp, sizeof(argp->npgno));
	bp += sizeof(argp->npgno);
	memcpy(&argp->nlsn, bp,  sizeof(argp->nlsn));
	bp += sizeof(argp->nlsn);
	memset(&argp->pg, 0, sizeof(argp->pg));
	memcpy(&argp->pg.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->pg.data = bp;
	bp += argp->pg.size;
	*argpp = argp;
	return (0);
}

int __bam_rsplit_log(dbenv, txnid, ret_lsnp, flags,
	fileid, pgno, pgdbt, nrec, rootent, rootlsn)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t pgno;
	const DBT *pgdbt;
	db_pgno_t nrec;
	const DBT *rootent;
	DB_LSN * rootlsn;
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
	rectype = DB_bam_rsplit;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(pgno)
	    + sizeof(u_int32_t) + (pgdbt == NULL ? 0 : pgdbt->size)
	    + sizeof(nrec)
	    + sizeof(u_int32_t) + (rootent == NULL ? 0 : rootent->size)
	    + sizeof(*rootlsn);
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
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	if (pgdbt == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &pgdbt->size, sizeof(pgdbt->size));
		bp += sizeof(pgdbt->size);
		memcpy(bp, pgdbt->data, pgdbt->size);
		bp += pgdbt->size;
	}
	memcpy(bp, &nrec, sizeof(nrec));
	bp += sizeof(nrec);
	if (rootent == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &rootent->size, sizeof(rootent->size));
		bp += sizeof(rootent->size);
		memcpy(bp, rootent->data, rootent->size);
		bp += rootent->size;
	}
	if (rootlsn != NULL)
		memcpy(bp, rootlsn, sizeof(*rootlsn));
	else
		memset(bp, 0, sizeof(*rootlsn));
	bp += sizeof(*rootlsn);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_rsplit_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_rsplit_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_rsplit_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_rsplit: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tpgdbt: ");
	for (i = 0; i < argp->pgdbt.size; i++) {
		ch = ((u_int8_t *)argp->pgdbt.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\tnrec: %lu\n", (u_long)argp->nrec);
	printf("\trootent: ");
	for (i = 0; i < argp->rootent.size; i++) {
		ch = ((u_int8_t *)argp->rootent.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\trootlsn: [%lu][%lu]\n",
	    (u_long)argp->rootlsn.file, (u_long)argp->rootlsn.offset);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_rsplit_read(recbuf, argpp)
	void *recbuf;
	__bam_rsplit_args **argpp;
{
	__bam_rsplit_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_rsplit_args) +
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
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memset(&argp->pgdbt, 0, sizeof(argp->pgdbt));
	memcpy(&argp->pgdbt.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->pgdbt.data = bp;
	bp += argp->pgdbt.size;
	memcpy(&argp->nrec, bp, sizeof(argp->nrec));
	bp += sizeof(argp->nrec);
	memset(&argp->rootent, 0, sizeof(argp->rootent));
	memcpy(&argp->rootent.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->rootent.data = bp;
	bp += argp->rootent.size;
	memcpy(&argp->rootlsn, bp,  sizeof(argp->rootlsn));
	bp += sizeof(argp->rootlsn);
	*argpp = argp;
	return (0);
}

int __bam_adj_log(dbenv, txnid, ret_lsnp, flags,
	fileid, pgno, lsn, indx, indx_copy, is_insert)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	u_int32_t indx_copy;
	u_int32_t is_insert;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_bam_adj;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(pgno)
	    + sizeof(*lsn)
	    + sizeof(indx)
	    + sizeof(indx_copy)
	    + sizeof(is_insert);
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
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	memcpy(bp, &indx_copy, sizeof(indx_copy));
	bp += sizeof(indx_copy);
	memcpy(bp, &is_insert, sizeof(is_insert));
	bp += sizeof(is_insert);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_adj_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_adj_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_adj_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_adj: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\tindx_copy: %lu\n", (u_long)argp->indx_copy);
	printf("\tis_insert: %lu\n", (u_long)argp->is_insert);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_adj_read(recbuf, argpp)
	void *recbuf;
	__bam_adj_args **argpp;
{
	__bam_adj_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_adj_args) +
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
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	memcpy(&argp->indx_copy, bp, sizeof(argp->indx_copy));
	bp += sizeof(argp->indx_copy);
	memcpy(&argp->is_insert, bp, sizeof(argp->is_insert));
	bp += sizeof(argp->is_insert);
	*argpp = argp;
	return (0);
}

int __bam_cadjust_log(dbenv, txnid, ret_lsnp, flags,
	fileid, pgno, lsn, indx, adjust, total)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	int32_t adjust;
	int32_t total;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_bam_cadjust;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(pgno)
	    + sizeof(*lsn)
	    + sizeof(indx)
	    + sizeof(adjust)
	    + sizeof(total);
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
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	memcpy(bp, &adjust, sizeof(adjust));
	bp += sizeof(adjust);
	memcpy(bp, &total, sizeof(total));
	bp += sizeof(total);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_cadjust_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_cadjust_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_cadjust_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_cadjust: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\tadjust: %ld\n", (long)argp->adjust);
	printf("\ttotal: %ld\n", (long)argp->total);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_cadjust_read(recbuf, argpp)
	void *recbuf;
	__bam_cadjust_args **argpp;
{
	__bam_cadjust_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_cadjust_args) +
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
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	memcpy(&argp->adjust, bp, sizeof(argp->adjust));
	bp += sizeof(argp->adjust);
	memcpy(&argp->total, bp, sizeof(argp->total));
	bp += sizeof(argp->total);
	*argpp = argp;
	return (0);
}

int __bam_cdel_log(dbenv, txnid, ret_lsnp, flags,
	fileid, pgno, lsn, indx)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_bam_cdel;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(pgno)
	    + sizeof(*lsn)
	    + sizeof(indx);
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
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_cdel_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_cdel_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_cdel_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_cdel: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_cdel_read(recbuf, argpp)
	void *recbuf;
	__bam_cdel_args **argpp;
{
	__bam_cdel_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_cdel_args) +
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
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	*argpp = argp;
	return (0);
}

int __bam_repl_log(dbenv, txnid, ret_lsnp, flags,
	fileid, pgno, lsn, indx, isdeleted, orig,
	repl, prefix, suffix)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t pgno;
	DB_LSN * lsn;
	u_int32_t indx;
	u_int32_t isdeleted;
	const DBT *orig;
	const DBT *repl;
	u_int32_t prefix;
	u_int32_t suffix;
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
	rectype = DB_bam_repl;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(pgno)
	    + sizeof(*lsn)
	    + sizeof(indx)
	    + sizeof(isdeleted)
	    + sizeof(u_int32_t) + (orig == NULL ? 0 : orig->size)
	    + sizeof(u_int32_t) + (repl == NULL ? 0 : repl->size)
	    + sizeof(prefix)
	    + sizeof(suffix);
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
	memcpy(bp, &pgno, sizeof(pgno));
	bp += sizeof(pgno);
	if (lsn != NULL)
		memcpy(bp, lsn, sizeof(*lsn));
	else
		memset(bp, 0, sizeof(*lsn));
	bp += sizeof(*lsn);
	memcpy(bp, &indx, sizeof(indx));
	bp += sizeof(indx);
	memcpy(bp, &isdeleted, sizeof(isdeleted));
	bp += sizeof(isdeleted);
	if (orig == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &orig->size, sizeof(orig->size));
		bp += sizeof(orig->size);
		memcpy(bp, orig->data, orig->size);
		bp += orig->size;
	}
	if (repl == NULL) {
		zero = 0;
		memcpy(bp, &zero, sizeof(u_int32_t));
		bp += sizeof(u_int32_t);
	} else {
		memcpy(bp, &repl->size, sizeof(repl->size));
		bp += sizeof(repl->size);
		memcpy(bp, repl->data, repl->size);
		bp += repl->size;
	}
	memcpy(bp, &prefix, sizeof(prefix));
	bp += sizeof(prefix);
	memcpy(bp, &suffix, sizeof(suffix));
	bp += sizeof(suffix);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_repl_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_repl_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_repl_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_repl: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tpgno: %lu\n", (u_long)argp->pgno);
	printf("\tlsn: [%lu][%lu]\n",
	    (u_long)argp->lsn.file, (u_long)argp->lsn.offset);
	printf("\tindx: %lu\n", (u_long)argp->indx);
	printf("\tisdeleted: %lu\n", (u_long)argp->isdeleted);
	printf("\torig: ");
	for (i = 0; i < argp->orig.size; i++) {
		ch = ((u_int8_t *)argp->orig.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\trepl: ");
	for (i = 0; i < argp->repl.size; i++) {
		ch = ((u_int8_t *)argp->repl.data)[i];
		if (isprint(ch) || ch == 0xa)
			putchar(ch);
		else
			printf("%#x ", ch);
	}
	printf("\n");
	printf("\tprefix: %lu\n", (u_long)argp->prefix);
	printf("\tsuffix: %lu\n", (u_long)argp->suffix);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_repl_read(recbuf, argpp)
	void *recbuf;
	__bam_repl_args **argpp;
{
	__bam_repl_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_repl_args) +
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
	memcpy(&argp->pgno, bp, sizeof(argp->pgno));
	bp += sizeof(argp->pgno);
	memcpy(&argp->lsn, bp,  sizeof(argp->lsn));
	bp += sizeof(argp->lsn);
	memcpy(&argp->indx, bp, sizeof(argp->indx));
	bp += sizeof(argp->indx);
	memcpy(&argp->isdeleted, bp, sizeof(argp->isdeleted));
	bp += sizeof(argp->isdeleted);
	memset(&argp->orig, 0, sizeof(argp->orig));
	memcpy(&argp->orig.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->orig.data = bp;
	bp += argp->orig.size;
	memset(&argp->repl, 0, sizeof(argp->repl));
	memcpy(&argp->repl.size, bp, sizeof(u_int32_t));
	bp += sizeof(u_int32_t);
	argp->repl.data = bp;
	bp += argp->repl.size;
	memcpy(&argp->prefix, bp, sizeof(argp->prefix));
	bp += sizeof(argp->prefix);
	memcpy(&argp->suffix, bp, sizeof(argp->suffix));
	bp += sizeof(argp->suffix);
	*argpp = argp;
	return (0);
}

int __bam_root_log(dbenv, txnid, ret_lsnp, flags,
	fileid, meta_pgno, root_pgno, meta_lsn)
	DB_ENV *dbenv;
	DB_TXN *txnid;
	DB_LSN *ret_lsnp;
	u_int32_t flags;
	int32_t fileid;
	db_pgno_t meta_pgno;
	db_pgno_t root_pgno;
	DB_LSN * meta_lsn;
{
	DBT logrec;
	DB_LSN *lsnp, null_lsn;
	u_int32_t rectype, txn_num;
	int ret;
	u_int8_t *bp;

	if (txnid != NULL &&
	    TAILQ_FIRST(&txnid->kids) != NULL && __txn_activekids(txnid) != 0)
		return (EPERM);
	rectype = DB_bam_root;
	txn_num = txnid == NULL ? 0 : txnid->txnid;
	if (txnid == NULL) {
		ZERO_LSN(null_lsn);
		lsnp = &null_lsn;
	} else
		lsnp = &txnid->last_lsn;
	logrec.size = sizeof(rectype) + sizeof(txn_num) + sizeof(DB_LSN)
	    + sizeof(fileid)
	    + sizeof(meta_pgno)
	    + sizeof(root_pgno)
	    + sizeof(*meta_lsn);
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
	memcpy(bp, &meta_pgno, sizeof(meta_pgno));
	bp += sizeof(meta_pgno);
	memcpy(bp, &root_pgno, sizeof(root_pgno));
	bp += sizeof(root_pgno);
	if (meta_lsn != NULL)
		memcpy(bp, meta_lsn, sizeof(*meta_lsn));
	else
		memset(bp, 0, sizeof(*meta_lsn));
	bp += sizeof(*meta_lsn);
	DB_ASSERT((u_int32_t)(bp - (u_int8_t *)logrec.data) == logrec.size);
	ret = log_put(dbenv, ret_lsnp, (DBT *)&logrec, flags);
	if (txnid != NULL)
		txnid->last_lsn = *ret_lsnp;
	__os_free(logrec.data, logrec.size);
	return (ret);
}

int
__bam_root_print(notused1, dbtp, lsnp, notused2, notused3)
	DB_ENV *notused1;
	DBT *dbtp;
	DB_LSN *lsnp;
	int notused2;
	void *notused3;
{
	__bam_root_args *argp;
	u_int32_t i;
	u_int ch;
	int ret;

	i = 0;
	ch = 0;
	notused1 = NULL;
	notused2 = 0;
	notused3 = NULL;

	if ((ret = __bam_root_read(dbtp->data, &argp)) != 0)
		return (ret);
	printf("[%lu][%lu]bam_root: rec: %lu txnid %lx prevlsn [%lu][%lu]\n",
	    (u_long)lsnp->file,
	    (u_long)lsnp->offset,
	    (u_long)argp->type,
	    (u_long)argp->txnid->txnid,
	    (u_long)argp->prev_lsn.file,
	    (u_long)argp->prev_lsn.offset);
	printf("\tfileid: %lu\n", (u_long)argp->fileid);
	printf("\tmeta_pgno: %lu\n", (u_long)argp->meta_pgno);
	printf("\troot_pgno: %lu\n", (u_long)argp->root_pgno);
	printf("\tmeta_lsn: [%lu][%lu]\n",
	    (u_long)argp->meta_lsn.file, (u_long)argp->meta_lsn.offset);
	printf("\n");
	__os_free(argp, 0);
	return (0);
}

int
__bam_root_read(recbuf, argpp)
	void *recbuf;
	__bam_root_args **argpp;
{
	__bam_root_args *argp;
	u_int8_t *bp;
	int ret;

	ret = __os_malloc(sizeof(__bam_root_args) +
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
	memcpy(&argp->meta_pgno, bp, sizeof(argp->meta_pgno));
	bp += sizeof(argp->meta_pgno);
	memcpy(&argp->root_pgno, bp, sizeof(argp->root_pgno));
	bp += sizeof(argp->root_pgno);
	memcpy(&argp->meta_lsn, bp,  sizeof(argp->meta_lsn));
	bp += sizeof(argp->meta_lsn);
	*argpp = argp;
	return (0);
}

int
__bam_init_print(dbenv)
	DB_ENV *dbenv;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv,
	    __bam_pg_alloc_print, DB_bam_pg_alloc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_pg_free_print, DB_bam_pg_free)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_split_print, DB_bam_split)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_rsplit_print, DB_bam_rsplit)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_adj_print, DB_bam_adj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_cadjust_print, DB_bam_cadjust)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_cdel_print, DB_bam_cdel)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_repl_print, DB_bam_repl)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_root_print, DB_bam_root)) != 0)
		return (ret);
	return (0);
}

/*
 * PUBLIC: int __bam_init_recover __P((DB_ENV *));
 */
int
__bam_init_recover(dbenv)
	DB_ENV *dbenv;
{
	int ret;

	if ((ret = __db_add_recovery(dbenv,
	    __bam_pg_alloc_recover, DB_bam_pg_alloc)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_pg_free_recover, DB_bam_pg_free)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_split_recover, DB_bam_split)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_rsplit_recover, DB_bam_rsplit)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_adj_recover, DB_bam_adj)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_cadjust_recover, DB_bam_cadjust)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_cdel_recover, DB_bam_cdel)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_repl_recover, DB_bam_repl)) != 0)
		return (ret);
	if ((ret = __db_add_recovery(dbenv,
	    __bam_root_recover, DB_bam_root)) != 0)
		return (ret);
	return (0);
}

