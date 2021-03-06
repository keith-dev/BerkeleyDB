/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: txn.h,v 12.7 2005/10/13 00:53:00 bostic Exp $
 */

#ifndef	_TXN_H_
#define	_TXN_H_

#include "dbinc/xa.h"

/* Operation parameters to the delayed commit processing code. */
typedef enum {
	TXN_CLOSE,		/* Close a DB handle whose close had failed. */
	TXN_REMOVE,		/* Remove a file. */
	TXN_TRADE,		/* Trade lockers. */
	TXN_TRADED		/* Already traded; downgrade lock. */
} TXN_EVENT_T;

struct __db_txnregion;	typedef struct __db_txnregion DB_TXNREGION;
struct __txn_logrec;	typedef struct __txn_logrec DB_TXNLOGREC;

/*
 * !!!
 * TXN_MINIMUM = (DB_LOCK_MAXID + 1) but this makes compilers complain.
 */
#define	TXN_MINIMUM	0x80000000
#define	TXN_MAXIMUM	0xffffffff	/* Maximum number of txn ids. */
#define	TXN_INVALID	0		/* Invalid transaction ID. */

#define	DEF_MAX_TXNS	20		/* Default max transactions. */

/*
 * Internal data maintained in shared memory for each transaction.
 */
typedef struct __txn_detail {
	u_int32_t txnid;		/* current transaction id
					   used to link free list also */
	pid_t pid;			/* Process owning txn */
	db_threadid_t tid;	/* Thread owning txn */

	DB_LSN	last_lsn;		/* last lsn written for this txn */
	DB_LSN	begin_lsn;		/* lsn of begin record */
	roff_t	parent;			/* Offset of transaction's parent. */
	roff_t	name;			/* Offset of txn name. */

	SH_TAILQ_HEAD(__tdkids)	kids;	/* Linked list of child txn detail. */
	SH_TAILQ_ENTRY		klinks;

#define	TXN_RUNNING		1
#define	TXN_ABORTED		2
#define	TXN_PREPARED		3
#define	TXN_COMMITTED		4
	u_int32_t status;		/* status of the transaction */
#define	TXN_DTL_COLLECTED	0x1	/* collected during txn_recover */
#define	TXN_DTL_RESTORED	0x2	/* prepared txn restored */
#define	TXN_DTL_INMEMORY	0x4	/* uses in memory logs */
	u_int32_t flags;

	SH_TAILQ_ENTRY	links;		/* free/active list */

#define	TXN_XA_ABORTED		1
#define	TXN_XA_DEADLOCKED	2
#define	TXN_XA_ENDED		3
#define	TXN_XA_PREPARED		4
#define	TXN_XA_STARTED		5
#define	TXN_XA_SUSPENDED	6
	u_int32_t xa_status;		/* XA status */

	/*
	 * XID (xid_t) structure: because these fields are logged, the
	 * sizes have to be explicit.
	 */
	u_int8_t xid[XIDDATASIZE];	/* XA global transaction id */
	u_int32_t bqual;		/* bqual_length from XID */
	u_int32_t gtrid;		/* gtrid_length from XID */
	int32_t format;			/* XA format */
} TXN_DETAIL;

/*
 * DB_TXNMGR --
 *	The transaction manager encapsulates the transaction system.
 */
struct __db_txnmgr {
	/*
	 * These fields need to be protected for multi-threaded support.
	 *
	 * Lock list of active transactions (including the content of each
	 * TXN_DETAIL structure on the list).
	 */
	db_mutex_t mutex;
					/* List of active transactions. */
	TAILQ_HEAD(_chain, __db_txn)	txn_chain;

	u_int32_t n_discards;		/* Number of txns discarded. */

	/* These fields are never updated after creation, so not protected. */
	DB_ENV	*dbenv;			/* Environment. */
	REGINFO	 reginfo;		/* Region information. */
};

/* Macros to lock/unlock the transaction region as a whole. */
#define	TXN_SYSTEM_LOCK(dbenv)						\
	MUTEX_LOCK(dbenv, ((DB_TXNREGION *)((DB_TXNMGR *)		\
	    (dbenv)->tx_handle)->reginfo.primary)->mtx_region)
#define	TXN_SYSTEM_UNLOCK(dbenv)					\
	MUTEX_UNLOCK(dbenv, ((DB_TXNREGION *)((DB_TXNMGR *)		\
	    (dbenv)->tx_handle)->reginfo.primary)->mtx_region)

/*
 * DB_TXNREGION --
 *	The primary transaction data structure in the shared memory region.
 */
struct __db_txnregion {
	db_mutex_t	mtx_region;	/* Region mutex. */

	u_int32_t	maxtxns;	/* maximum number of active TXNs */
	u_int32_t	last_txnid;	/* last transaction id given out */
	u_int32_t	cur_maxid;	/* current max unused id. */

	db_mutex_t	mtx_ckp;	/* Single thread checkpoints. */
	DB_LSN		last_ckp;	/* lsn of the last checkpoint */
	time_t		time_ckp;	/* time of last checkpoint */

	DB_TXN_STAT	stat;		/* Statistics for txns. */

#define	TXN_IN_RECOVERY	 0x01		/* environment is being recovered */
	u_int32_t	flags;
					/* active TXN list */
	SH_TAILQ_HEAD(__active) active_txn;
};

/*
 * DB_TXNLOGREC --
 *	An in-memory, linked-list copy of a log record.
 */
struct __txn_logrec {
	STAILQ_ENTRY(__txn_logrec) links;/* Linked list. */

	u_int8_t data[1];		/* Log record. */
};

/*
 * Log record types.  Note that these are *not* alphabetical.  This is
 * intentional so that we don't change the meaning of values between
 * software upgrades.
 *
 * EXPECTED, UNEXPECTED, IGNORE, and OK are used in the txnlist functions.
 * Here is an explanation of how the statuses are used.
 *
 * TXN_OK
 *	BEGIN records for transactions found on the txnlist during
 *	OPENFILES (BEGIN records are those with a prev_lsn of 0,0)
 *
 * TXN_COMMIT
 *	Transaction committed and should be rolled forward.
 *
 * TXN_ABORT
 *	This transaction's changes must be undone.  Either there was
 *	never a prepare or commit record for this transaction OR there
 *	was a commit, but we are recovering to a timestamp or particular
 *	LSN and that point is before this transaction's commit.
 *
 * TXN_PREPARE
 *	Prepare record, but no commit record is in the log.
 *
 * TXN_IGNORE
 *	Generic meaning is that this transaction should not be
 *	processed during later recovery passes.  We use it in a
 *	number of different manners:
 *
 *	1. We never saw its BEGIN record.  Therefore, the logs have
 *	   been reclaimed and we *know* that this transaction doesn't
 *	   need to be aborted, because in order for it to be
 *	   reclaimed, there must have been a subsequent checkpoint
 *	   (and any dirty pages for this transaction made it to
 *	   disk).
 *
 *	2. This is a child transaction that created a database.
 *	   For some reason, we don't want to recreate that database
 *	   (i.e., it already exists or some other database created
 *	   after it exists).
 *
 *	3. During recovery open of subdatabases, if the master check fails,
 *	   we use a TXN_IGNORE on the create of the subdb in the nested
 *	   transaction.
 *
 *	4. During a remove, the file with the name being removed isn't
 *	   the file for which we are recovering a remove.
 *
 * TXN_EXPECTED
 *	After a successful open during recovery, we update the
 *	transaction's status to TXN_EXPECTED.  The open was done
 *	in the parent, but in the open log record, we record the
 *	child transaction's ID if we also did a create.  When there
 *	is a valid ID in that field, we use it and mark the child's
 *	status as TXN_EXPECTED (indicating that we don't need to redo
 *	a create for this file).
 *
 *	When recovering a remove, if we don't find or can't open
 *	the file, the child (which does the remove) gets marked
 *	EXPECTED (indicating that we don't need to redo the remove).
 *
 * TXN_UNEXPECTED
 *	During recovery, we attempted an open that should have succeeded
 *	and we got ENOENT, so like with the EXPECTED case, we indicate
 *	in the child that we got the UNEXPECTED return so that we do redo
 *	the creating/deleting operation.
 *
 */
#define	TXN_OK		0
#define	TXN_COMMIT	1
#define	TXN_PREPARE	2
#define	TXN_ABORT	3
#define	TXN_IGNORE	4
#define	TXN_EXPECTED	5
#define	TXN_UNEXPECTED	6

#include "dbinc_auto/txn_auto.h"
#include "dbinc_auto/txn_ext.h"
#include "dbinc_auto/xa_ext.h"
#endif /* !_TXN_H_ */
