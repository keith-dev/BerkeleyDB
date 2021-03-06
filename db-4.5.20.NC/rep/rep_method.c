/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: rep_method.c,v 12.46 2006/09/09 14:19:20 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/btree.h"
#include "dbinc/log.h"
#include "dbinc/txn.h"

static int  __rep_abort_prepared __P((DB_ENV *));
static int  __rep_bt_cmp __P((DB *, const DBT *, const DBT *));
static void __rep_config_map __P((DB_ENV *, u_int32_t *, u_int32_t *));
static u_int32_t __rep_conv_vers __P((DB_ENV *, u_int32_t));
static int  __rep_restore_prepared __P((DB_ENV *));

/*
 * __rep_dbenv_create --
 *	Replication-specific initialization of the DB_ENV structure.
 *
 * PUBLIC: int __rep_dbenv_create __P((DB_ENV *));
 */
int
__rep_dbenv_create(dbenv)
	DB_ENV *dbenv;
{
	DB_REP *db_rep;
	int ret;

	if ((ret = __os_calloc(dbenv, 1, sizeof(DB_REP), &db_rep)) != 0)
		return (ret);

	db_rep->eid = DB_EID_INVALID;
	db_rep->request_gap = DB_REP_REQUEST_GAP;
	db_rep->max_gap = DB_REP_MAX_GAP;

#ifdef HAVE_REPLICATION_THREADS
	if ((ret = __repmgr_dbenv_create(dbenv, db_rep)) != 0) {
		__os_free(dbenv, db_rep);
		return (ret);
	}
#endif

	dbenv->rep_handle = db_rep;
	return (0);
}

/*
 * __rep_dbenv_destroy --
 *	Replication-specific destruction of the DB_ENV structure.
 *
 * PUBLIC: void __rep_dbenv_destroy __P((DB_ENV *));
 */
void
__rep_dbenv_destroy(dbenv)
	DB_ENV *dbenv;
{
	if (dbenv->rep_handle != NULL) {
#ifdef HAVE_REPLICATION_THREADS
		__repmgr_dbenv_destroy(dbenv, dbenv->rep_handle);
#endif
		__os_free(dbenv, dbenv->rep_handle);
		dbenv->rep_handle = NULL;
	}
}

/*
 * __rep_get_config --
 *	Return the replication subsystem configuration.
 *
 * PUBLIC: int __rep_get_config __P((DB_ENV *, u_int32_t, int *));
 */
int
__rep_get_config(dbenv, which, onp)
	DB_ENV *dbenv;
	u_int32_t which;
	int *onp;
{
	DB_REP *db_rep;
	REP *rep;
	u_int32_t mapped;

#undef	OK_FLAGS
#define	OK_FLAGS							\
	(DB_REP_CONF_BULK | DB_REP_CONF_DELAYCLIENT |			\
	DB_REP_CONF_NOAUTOINIT | DB_REP_CONF_NOWAIT)

	if (FLD_ISSET(which, ~OK_FLAGS))
		return (__db_ferr(dbenv, "DB_ENV->rep_get_config", 0));

	db_rep = dbenv->rep_handle;
	ENV_NOT_CONFIGURED(
	    dbenv, db_rep->region, "DB_ENV->rep_get_config", DB_INIT_REP);

	mapped = 0;
	__rep_config_map(dbenv, &which, &mapped);
	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		if (FLD_ISSET(rep->config, mapped))
			*onp = 1;
		else
			*onp = 0;
	} else {
		if (FLD_ISSET(db_rep->config, mapped))
			*onp = 1;
		else
			*onp = 0;
	}
	return (0);
}

/*
 * __rep_set_config --
 *	Configure the replication subsystem.
 *
 * PUBLIC: int __rep_set_config __P((DB_ENV *, u_int32_t, int));
 */
int
__rep_set_config(dbenv, which, on)
	DB_ENV *dbenv;
	u_int32_t which;
	int on;
{
	DB_LOG *dblp;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	REP_BULK bulk;
	int ret;
	u_int32_t mapped, orig;

	ret = 0;

#undef	OK_FLAGS
#define	OK_FLAGS							\
    (DB_REP_CONF_BULK | DB_REP_CONF_DELAYCLIENT |			\
    DB_REP_CONF_NOAUTOINIT | DB_REP_CONF_NOWAIT)

	if (FLD_ISSET(which, ~OK_FLAGS))
		return (__db_ferr(dbenv, "DB_ENV->rep_set_config", 0));

	db_rep = dbenv->rep_handle;
	ENV_NOT_CONFIGURED(
	    dbenv, db_rep->region, "DB_ENV->rep_set_config", DB_INIT_REP);

	mapped = 0;
	__rep_config_map(dbenv, &which, &mapped);
	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		MUTEX_LOCK(dbenv, rep->mtx_clientdb);
		REP_SYSTEM_LOCK(dbenv);
		orig = rep->config;
		if (on)
			FLD_SET(rep->config, mapped);
		else
			FLD_CLR(rep->config, mapped);

		/*
		 * Bulk transfer requires special processing if it is getting
		 * toggled.
		 */
		dblp = dbenv->lg_handle;
		lp = dblp->reginfo.primary;
		if (FLD_ISSET(rep->config, REP_C_BULK) &&
		    !FLD_ISSET(orig, REP_C_BULK))
			db_rep->bulk = R_ADDR(&dblp->reginfo, lp->bulk_buf);
		REP_SYSTEM_UNLOCK(dbenv);

		/*
		 * If turning bulk off and it was on, send out whatever is in
		 * the buffer already.
		 */
		if (FLD_ISSET(orig, REP_C_BULK) &&
		    !FLD_ISSET(rep->config, REP_C_BULK) && lp->bulk_off != 0) {
			memset(&bulk, 0, sizeof(bulk));
			if (db_rep->bulk == NULL)
				bulk.addr =
				    R_ADDR(&dblp->reginfo, lp->bulk_buf);
			else
				bulk.addr = db_rep->bulk;
			bulk.offp = &lp->bulk_off;
			bulk.len = lp->bulk_len;
			bulk.type = REP_BULK_LOG;
			bulk.eid = DB_EID_BROADCAST;
			bulk.flagsp = &lp->bulk_flags;
			ret = __rep_send_bulk(dbenv, &bulk, 0);
		}
		MUTEX_UNLOCK(dbenv, rep->mtx_clientdb);
	} else {
		if (on)
			FLD_SET(db_rep->config, mapped);
		else
			FLD_CLR(db_rep->config, mapped);
	}
	return (ret);
}

static void
__rep_config_map(dbenv, inflagsp, outflagsp)
	DB_ENV *dbenv;
	u_int32_t *inflagsp, *outflagsp;
{
	COMPQUIET(dbenv, NULL);

	if (FLD_ISSET(*inflagsp, DB_REP_CONF_BULK)) {
		FLD_SET(*outflagsp, REP_C_BULK);
		FLD_CLR(*inflagsp, DB_REP_CONF_BULK);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_DELAYCLIENT)) {
		FLD_SET(*outflagsp, REP_C_DELAYCLIENT);
		FLD_CLR(*inflagsp, DB_REP_CONF_DELAYCLIENT);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_NOAUTOINIT)) {
		FLD_SET(*outflagsp, REP_C_NOAUTOINIT);
		FLD_CLR(*inflagsp, DB_REP_CONF_NOAUTOINIT);
	}
	if (FLD_ISSET(*inflagsp, DB_REP_CONF_NOWAIT)) {
		FLD_SET(*outflagsp, REP_C_NOWAIT);
		FLD_CLR(*inflagsp, DB_REP_CONF_NOWAIT);
	}
}

/*
 * __rep_start --
 *	Become a master or client, and start sending messages to participate
 * in the replication environment.  Must be called after the environment
 * is open.
 *
 * We must protect rep_start, which may change the world, with the rest
 * of the DB library.  Each API interface will count itself as it enters
 * the library.  Rep_start checks the following:
 *
 * rep->msg_th - this is the count of threads currently in rep_process_message
 * rep->lockout_th - this is set if a thread is in rep_start or other
 *  operation requiring lockout with rep_proc_msg threads.
 * rep->handle_cnt - number of threads actively using a dbp in library.
 * rep->txn_cnt - number of active txns.
 * REP_F_READY - Replication flag that indicates that we wish to run
 * recovery, and want to prohibit new transactions from entering and cause
 * existing ones to return immediately (with a DB_LOCK_DEADLOCK error).
 *
 * There is also the renv->rep_timestamp which is updated whenever significant
 * events (i.e., new masters, log rollback, etc).  Upon creation, a handle
 * is associated with the current timestamp.  Each time a handle enters the
 * library it must check if the handle timestamp is the same as the one
 * stored in the replication region.  This prevents the use of handles on
 * clients that reference non-existent files whose creation was backed out
 * during a synchronizing recovery.
 *
 * PUBLIC: int __rep_start __P((DB_ENV *, DBT *, u_int32_t));
 */
int
__rep_start(dbenv, dbt, flags)
	DB_ENV *dbenv;
	DBT *dbt;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	u_int32_t oldvers, pending_event, repflags;
	int announce, init_db, locked, redo_prepared, ret, role_chg;
	int t_ret;
#ifdef DIAGNOSTIC
	DB_MSGBUF mb;
#endif

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG_XX(
	    dbenv, rep_handle, "DB_ENV->rep_start", DB_INIT_REP);

	db_rep = dbenv->rep_handle;
	rep = db_rep->region;
	locked = 0;

	if ((ret = __db_fchk(dbenv, "DB_ENV->rep_start", flags,
	    DB_REP_CLIENT | DB_REP_MASTER)) != 0)
		return (ret);

	/* Exactly one of CLIENT and MASTER must be specified. */
	if ((ret = __db_fcchk(dbenv,
	    "DB_ENV->rep_start", flags, DB_REP_CLIENT, DB_REP_MASTER)) != 0)
		return (ret);
	if (!LF_ISSET(DB_REP_CLIENT | DB_REP_MASTER)) {
		__db_errx(dbenv,
	"DB_ENV->rep_start: replication mode must be specified");
		return (EINVAL);
	}

	/* We need a transport function. */
	if (db_rep->send == NULL) {
		__db_errx(dbenv,
    "DB_ENV->rep_set_transport must be called before DB_ENV->rep_start");
		return (EINVAL);
	}

	/*
	 * In order to correctly check log files for old versions, we
	 * need to flush the logs.
	 */
	if ((ret = __log_flush(dbenv, NULL)) != 0)
		return (ret);

	pending_event = DB_EVENT_NO_SUCH_EVENT;
	REP_SYSTEM_LOCK(dbenv);
	/*
	 * We only need one thread to start-up replication, so if
	 * there is another thread in rep_start, we'll let it finish
	 * its work and have this thread simply return.  Similarly,
	 * if a thread is in a critical lockout section we return.
	 */
	if (rep->lockout_th != 0) {
		/*
		 * There is already someone in lockout.  Return.
		 */
		RPRINT(dbenv, (dbenv, &mb, "Thread already in lockout"));
		goto err;
	} else if ((ret = __rep_lockout_msg(dbenv, rep, 0)) != 0)
		goto errunlock;

	role_chg = (!F_ISSET(rep, REP_F_MASTER) && LF_ISSET(DB_REP_MASTER)) ||
	    (!F_ISSET(rep, REP_F_CLIENT) && LF_ISSET(DB_REP_CLIENT));

	/*
	 * Wait for any active txns or mpool ops to complete, and
	 * prevent any new ones from occurring, only if we're
	 * changing roles.
	 */
	if (role_chg) {
		if ((ret = __rep_lockout_api(dbenv, rep)) != 0)
			goto errunlock;
		locked = 1;
	}

	if (LF_ISSET(DB_REP_MASTER)) {
		if (role_chg) {
			/*
			 * If we're upgrading from having been a client,
			 * preclose, so that we close our temporary database
			 * and any files we opened while doing a rep_apply.
			 * If we don't we can infinitely leak file ids if
			 * the master crashed with files open (the likely
			 * case).  If we don't close them we can run into
			 * problems if we try to remove that file or long
			 * running applications end up with an unbounded
			 * number of used fileids, each getting written
			 * on checkpoint.  Just close them.
			 * Then invalidate all files open in the logging
			 * region.  These are files open by other processes
			 * attached to the environment.  They must be
			 * closed by the other processes when they notice
			 * the change in role.
			 */
			if ((ret = __rep_preclose(dbenv)) != 0)
				goto errunlock;

		}

		redo_prepared = 0;
		if (!F_ISSET(rep, REP_F_MASTER)) {
			/* Master is not yet set. */
			if (role_chg) {
				if (rep->w_gen > rep->recover_gen)
					rep->gen = ++rep->w_gen;
				else if (rep->gen > rep->recover_gen)
					rep->gen++;
				else
					rep->gen = rep->recover_gen + 1;
				/*
				 * There could have been any number of failed
				 * elections, so jump the gen if we need to now.
				 */
				if (rep->egen > rep->gen)
					rep->gen = rep->egen;
				redo_prepared = 1;
			} else if (rep->gen == 0)
				rep->gen = rep->recover_gen + 1;
			if (F_ISSET(rep, REP_F_MASTERELECT)) {
				__rep_elect_done(dbenv, rep);
				F_CLR(rep, REP_F_MASTERELECT);
			}
			if (rep->egen <= rep->gen)
				rep->egen = rep->gen + 1;
			RPRINT(dbenv, (dbenv, &mb,
			    "New master gen %lu, egen %lu",
			    (u_long)rep->gen, (u_long)rep->egen));
		}
		rep->master_id = rep->eid;
		/*
		 * Clear out almost everything, and then set MASTER.  Leave
		 * READY alone in case we did a lockout above; we'll clear it in
		 * a moment (below), once we've written the txn_recycle into the
		 * log.
		 */
		repflags = F_ISSET(rep, REP_F_READY);
		FLD_SET(repflags, REP_F_MASTER);
		rep->flags = repflags;

		dblp = (DB_LOG *)dbenv->lg_handle;
		lp = dblp->reginfo.primary;
		/*
		 * We're master.  Set the versions to the current ones.
		 */
		oldvers = lp->persist.version;
		/*
		 * If we're moving forward to the current version, we need
		 * to force the log file to advance and reset the
		 * recovery table since it contains pointers to old
		 * recovery functions.
		 */
		RPRINT(dbenv, (dbenv, &mb,
		    "rep_start: Old log version was %lu", (u_long)oldvers));
		if (lp->persist.version != DB_LOGVERSION) {
			if ((ret = __env_init_rec(dbenv, DB_LOGVERSION)) != 0)
				goto errunlock;
		}
		rep->version = DB_REPVERSION;
		rep->lockout_th = 0;
		REP_SYSTEM_UNLOCK(dbenv);
		LOG_SYSTEM_LOCK(dbenv);
		lsn = lp->lsn;
		LOG_SYSTEM_UNLOCK(dbenv);

		/*
		 * Send the NEWMASTER message first so that clients know
		 * subsequent messages are coming from the right master.
		 * We need to perform all actions below no matter what
		 * regarding errors.
		 */
		(void)__rep_send_message(dbenv,
		    DB_EID_BROADCAST, REP_NEWMASTER, &lsn, NULL, 0, 0);
		ret = 0;
		if (role_chg) {
			pending_event = DB_EVENT_REP_MASTER;
			ret = __dbreg_invalidate_files(dbenv);
			if ((t_ret = __rep_closefiles(dbenv)) != 0 && ret == 0)
				ret = t_ret;
			if ((t_ret = __txn_reset(dbenv)) != 0 && ret == 0)
				ret = t_ret;
			DB_ENV_TEST_RECYCLE(dbenv, ret);
			REP_SYSTEM_LOCK(dbenv);
			F_CLR(rep, REP_F_READY);
			rep->in_recovery = 0;
			locked = 0;
			REP_SYSTEM_UNLOCK(dbenv);
		}
		/*
		 * Take a transaction checkpoint so that our new generation
		 * number get written to the log.
		 */
		if ((t_ret = __txn_checkpoint(dbenv, 0, 0, DB_FORCE)) != 0 &&
		    ret == 0)
			ret = t_ret;
		if (redo_prepared &&
		    (t_ret = __rep_restore_prepared(dbenv)) != 0 && ret == 0)
			ret = t_ret;
	} else {
		init_db = 0;
		announce = role_chg || rep->master_id == DB_EID_INVALID;

		/*
		 * If we're changing roles we need to init the db.
		 */
		if (role_chg) {
			rep->master_id = DB_EID_INVALID;
			init_db = 1;
		}
		/* Zero out everything except recovery and tally flags. */
		repflags = F_ISSET(rep, REP_F_NOARCHIVE |
		    REP_F_RECOVER_MASK | REP_F_TALLY);
		FLD_SET(repflags, REP_F_CLIENT);
		if ((ret = __log_get_oldversion(dbenv, &oldvers)) != 0)
			goto errunlock;
		RPRINT(dbenv, (dbenv, &mb,
		    "rep_start: Found old version log %d", oldvers));
		if (oldvers >= DB_LOGVERSION_42) {
			__log_set_version(dbenv, oldvers);
			oldvers = __rep_conv_vers(dbenv, oldvers);
			DB_ASSERT(dbenv, oldvers != DB_REPVERSION_INVALID);
			rep->version = oldvers;
		}
		rep->flags = repflags;
		REP_SYSTEM_UNLOCK(dbenv);

		/*
		 * Abort any prepared transactions that were restored
		 * by recovery.  We won't be able to create any txns of
		 * our own until they're resolved, but we can't resolve
		 * them ourselves;  the master has to.  If any get
		 * resolved as commits, we'll redo them when commit
		 * records come in.  Aborts will simply be ignored.
		 */
		if ((ret = __rep_abort_prepared(dbenv)) != 0)
			goto errlock;

		MUTEX_LOCK(dbenv, rep->mtx_clientdb);
		ret = __rep_client_dbinit(dbenv, init_db, REP_DB);
		MUTEX_UNLOCK(dbenv, rep->mtx_clientdb);
		if (ret != 0)
			goto errlock;
		if (role_chg)
			pending_event = DB_EVENT_REP_CLIENT;
		REP_SYSTEM_LOCK(dbenv);
		rep->lockout_th = 0;
		if (locked) {
			F_CLR(rep, REP_F_READY);
			rep->in_recovery = 0;
			locked = 0;
		}
		REP_SYSTEM_UNLOCK(dbenv);

		/*
		 * If this client created a newly replicated environment,
		 * then announce the existence of this client.  The master
		 * should respond with a message that will tell this client
		 * the current generation number and the current LSN.  This
		 * will allow the client to either perform recovery or
		 * simply join in.
		 */
		if (announce)
			(void)__rep_send_message(dbenv,
			    DB_EID_BROADCAST, REP_NEWCLIENT, NULL, dbt, 0, 0);
		else
			(void)__rep_send_message(dbenv,
			    DB_EID_BROADCAST, REP_ALIVE_REQ, NULL, NULL, 0, 0);
	}

	if (0) {
		/*
		 * We have separate labels for errors.  If we're returning an
		 * error before we've set lockout_th, we use 'err'.  If
		 * we are erroring while holding the region mutex, then we use
		 * 'errunlock' label.  If we're erroring without holding the rep
		 * mutex we must use 'errlock'.
		 */
DB_TEST_RECOVERY_LABEL
errlock:	REP_SYSTEM_LOCK(dbenv);
errunlock:	rep->lockout_th = 0;
		if (locked) {
			F_CLR(rep, REP_F_READY);
			rep->in_recovery = 0;
		}
err:		REP_SYSTEM_UNLOCK(dbenv);
	}
	if (pending_event != DB_EVENT_NO_SUCH_EVENT)
		DB_EVENT(dbenv, pending_event, NULL);
	return (ret);
}

/*
 * __rep_client_dbinit --
 *
 * Initialize the LSN database on the client side.  This is called from the
 * client initialization code.  The startup flag value indicates if
 * this is the first thread/process starting up and therefore should create
 * the LSN database.  This routine must be called once by each process acting
 * as a client.
 *
 * Assumes caller holds appropriate mutex.
 *
 * PUBLIC: int __rep_client_dbinit __P((DB_ENV *, int, repdb_t));
 */
int
__rep_client_dbinit(dbenv, startup, which)
	DB_ENV *dbenv;
	int startup;
	repdb_t which;
{
	DB_REP *db_rep;
	DB *dbp, **rdbpp;
	REP *rep;
	int ret, t_ret;
	u_int32_t flags;
	const char *name;

	PANIC_CHECK(dbenv);
	db_rep = dbenv->rep_handle;
	rep = db_rep->region;
	dbp = NULL;

	if (which == REP_DB) {
		name = REPDBNAME;
		rdbpp = &db_rep->rep_db;
	} else {
		name = REPPAGENAME;
		rdbpp = &rep->file_dbp;
	}
	/* Check if this has already been called on this environment. */
	if (*rdbpp != NULL)
		return (0);

	if (startup) {
		if ((ret = db_create(&dbp, dbenv, 0)) != 0)
			goto err;
		/*
		 * Ignore errors, because if the file doesn't exist, this
		 * is perfectly OK.
		 */
		(void)__db_remove(dbp, NULL, name, NULL, DB_FORCE);
	}

	if ((ret = db_create(&dbp, dbenv, 0)) != 0)
		goto err;
	if (which == REP_DB &&
	    (ret = __bam_set_bt_compare(dbp, __rep_bt_cmp)) != 0)
		goto err;

	/* Allow writes to this database on a client. */
	F_SET(dbp, DB_AM_CL_WRITER);

	flags = DB_NO_AUTO_COMMIT |
	    (startup ? DB_CREATE : 0) |
	    (F_ISSET(dbenv, DB_ENV_THREAD) ? DB_THREAD : 0);

	if ((ret = __db_open(dbp, NULL, name, NULL,
	    (which == REP_DB ? DB_BTREE : DB_RECNO),
	    flags, 0, PGNO_BASE_MD)) != 0)
		goto err;

	*rdbpp= dbp;

	if (0) {
err:		if (dbp != NULL &&
		    (t_ret = __db_close(dbp, NULL, DB_NOSYNC)) != 0 && ret == 0)
			ret = t_ret;
		*rdbpp = NULL;
	}

	return (ret);
}

/*
 * __rep_bt_cmp --
 *
 * Comparison function for the LSN table.  We use the entire control
 * structure as a key (for simplicity, so we don't have to merge the
 * other fields in the control with the data field), but really only
 * care about the LSNs.
 */
static int
__rep_bt_cmp(dbp, dbt1, dbt2)
	DB *dbp;
	const DBT *dbt1, *dbt2;
{
	DB_LSN lsn1, lsn2;
	REP_CONTROL *rp1, *rp2;

	COMPQUIET(dbp, NULL);

	rp1 = dbt1->data;
	rp2 = dbt2->data;

	(void)__ua_memcpy(&lsn1, &rp1->lsn, sizeof(DB_LSN));
	(void)__ua_memcpy(&lsn2, &rp2->lsn, sizeof(DB_LSN));

	if (lsn1.file > lsn2.file)
		return (1);

	if (lsn1.file < lsn2.file)
		return (-1);

	if (lsn1.offset > lsn2.offset)
		return (1);

	if (lsn1.offset < lsn2.offset)
		return (-1);

	return (0);
}

/*
 * __rep_abort_prepared --
 *	Abort any prepared transactions that recovery restored.
 *
 *	This is used by clients that have just run recovery, since
 * they cannot/should not call txn_recover and handle prepared transactions
 * themselves.
 */
static int
__rep_abort_prepared(dbenv)
	DB_ENV *dbenv;
{
#define	PREPLISTSIZE	50
	DB_PREPLIST prep[PREPLISTSIZE], *p;
	DB_TXNMGR *mgr;
	DB_TXNREGION *region;
	int do_aborts, ret;
	long count, i;
	u_int32_t op;

	mgr = dbenv->tx_handle;
	region = mgr->reginfo.primary;

	do_aborts = 0;
	TXN_SYSTEM_LOCK(dbenv);
	if (region->stat.st_nrestores != 0)
		do_aborts = 1;
	TXN_SYSTEM_UNLOCK(dbenv);

	if (do_aborts) {
		op = DB_FIRST;
		do {
			if ((ret = __txn_recover(dbenv,
			    prep, PREPLISTSIZE, &count, op)) != 0)
				return (ret);
			for (i = 0; i < count; i++) {
				p = &prep[i];
				if ((ret = __txn_abort(p->txn)) != 0)
					return (ret);
			}
			op = DB_NEXT;
		} while (count == PREPLISTSIZE);
	}

	return (0);
}

/*
 * __rep_restore_prepared --
 *	Restore to a prepared state any prepared but not yet committed
 * transactions.
 *
 *	This performs, in effect, a "mini-recovery";  it is called from
 * __rep_start by newly upgraded masters.  There may be transactions that an
 * old master prepared but did not resolve, which we need to restore to an
 * active state.
 */
static int
__rep_restore_prepared(dbenv)
	DB_ENV *dbenv;
{
	DB_LOGC *logc;
	DB_LSN ckp_lsn, lsn;
	DB_REP *db_rep;
	DB_TXNHEAD *txninfo;
	DBT rec;
	REP *rep;
	__txn_ckp_args *ckp_args;
	__txn_ckp_42_args *ckp42_args;
	__txn_regop_args *regop_args;
	__txn_regop_42_args *regop42_args;
	__txn_xa_regop_args *prep_args;
	int ret, t_ret;
	u_int32_t hi_txn, low_txn, rectype, status, txnid, txnop;

	db_rep = dbenv->rep_handle;
	rep = db_rep->region;
	txninfo = NULL;
	ckp_args = NULL;
	ckp42_args = NULL;
	prep_args = NULL;
	regop_args = NULL;
	regop42_args = NULL;
	ZERO_LSN(ckp_lsn);
	ZERO_LSN(lsn);

	if ((ret = __log_cursor(dbenv, &logc)) != 0)
		return (ret);

	/*
	 * We need to consider the set of records between the most recent
	 * checkpoint LSN and the end of the log;  any txn in that
	 * range, and only txns in that range, could still have been
	 * active, and thus prepared but not yet committed (PBNYC),
	 * when the old master died.
	 *
	 * Find the most recent checkpoint LSN, and get the record there.
	 * If there is no checkpoint in the log, start off by getting
	 * the very first record in the log instead.
	 */
	memset(&rec, 0, sizeof(DBT));
	if ((ret = __txn_getckp(dbenv, &lsn)) == 0) {
		if ((ret = __log_c_get(logc, &lsn, &rec, DB_SET)) != 0)  {
			__db_errx(dbenv,
			    "Checkpoint record at LSN [%lu][%lu] not found",
			    (u_long)lsn.file, (u_long)lsn.offset);
			goto err;
		}

		if (rep->version >= DB_REPVERSION_43) {
			if ((ret = __txn_ckp_read(dbenv, rec.data,
			    &ckp_args)) == 0) {
				ckp_lsn = ckp_args->ckp_lsn;
				__os_free(dbenv, ckp_args);
			}
		} else {
			if ((ret = __txn_ckp_42_read(dbenv, rec.data,
			    &ckp42_args)) == 0) {
				ckp_lsn = ckp42_args->ckp_lsn;
				__os_free(dbenv, ckp42_args);
			}
		}
		if (ret != 0) {
			__db_errx(dbenv,
			    "Invalid checkpoint record at [%lu][%lu]",
			    (u_long)lsn.file, (u_long)lsn.offset);
			goto err;
		}

		if ((ret = __log_c_get(logc, &ckp_lsn, &rec, DB_SET)) != 0) {
			__db_errx(dbenv,
			    "Checkpoint LSN record [%lu][%lu] not found",
			    (u_long)ckp_lsn.file, (u_long)ckp_lsn.offset);
			goto err;
		}
	} else if ((ret = __log_c_get(logc, &lsn, &rec, DB_FIRST)) != 0) {
		if (ret == DB_NOTFOUND) {
			/* An empty log means no PBNYC txns. */
			ret = 0;
			goto done;
		}
		__db_errx(dbenv, "Attempt to get first log record failed");
		goto err;
	}

	/*
	 * We use the same txnlist infrastructure that recovery does;
	 * it demands an estimate of the high and low txnids for
	 * initialization.
	 *
	 * First, the low txnid.
	 */
	do {
		/* txnid is after rectype, which is a u_int32. */
		memcpy(&low_txn,
		    (u_int8_t *)rec.data + sizeof(u_int32_t), sizeof(low_txn));
		if (low_txn != 0)
			break;
	} while ((ret = __log_c_get(logc, &lsn, &rec, DB_NEXT)) == 0);

	/* If there are no txns, there are no PBNYC txns. */
	if (ret == DB_NOTFOUND) {
		ret = 0;
		goto done;
	} else if (ret != 0)
		goto err;

	/* Now, the high txnid. */
	if ((ret = __log_c_get(logc, &lsn, &rec, DB_LAST)) != 0) {
		/*
		 * Note that DB_NOTFOUND is unacceptable here because we
		 * had to have looked at some log record to get this far.
		 */
		__db_errx(dbenv, "Final log record not found");
		goto err;
	}
	do {
		/* txnid is after rectype, which is a u_int32. */
		memcpy(&hi_txn,
		    (u_int8_t *)rec.data + sizeof(u_int32_t), sizeof(hi_txn));
		if (hi_txn != 0)
			break;
	} while ((ret = __log_c_get(logc, &lsn, &rec, DB_PREV)) == 0);
	if (ret == DB_NOTFOUND) {
		ret = 0;
		goto done;
	} else if (ret != 0)
		goto err;

	/* We have a high and low txnid.  Initialise the txn list. */
	if ((ret =
	    __db_txnlist_init(dbenv, low_txn, hi_txn, NULL, &txninfo)) != 0)
		goto err;

	/*
	 * Now, walk backward from the end of the log to ckp_lsn.  Any
	 * prepares that we hit without first hitting a commit or
	 * abort belong to PBNYC txns, and we need to apply them and
	 * restore them to a prepared state.
	 *
	 * Note that we wind up applying transactions out of order.
	 * Since all PBNYC txns still held locks on the old master and
	 * were isolated, this should be safe.
	 */
	for (ret = __log_c_get(logc, &lsn, &rec, DB_LAST);
	    ret == 0 && LOG_COMPARE(&lsn, &ckp_lsn) > 0;
	    ret = __log_c_get(logc, &lsn, &rec, DB_PREV)) {
		memcpy(&rectype, rec.data, sizeof(rectype));
		switch (rectype) {
		case DB___txn_regop:
			/*
			 * It's a commit or abort--but we don't care
			 * which!  Just add it to the list of txns
			 * that are resolved.
			 */
			if (rep->version >= DB_REPVERSION_44) {
				if ((ret = __txn_regop_read(dbenv, rec.data,
				    &regop_args)) != 0)
					goto err;
				txnid = regop_args->txnp->txnid;
				txnop = regop_args->opcode;
				__os_free(dbenv, regop_args);
			} else {
				if ((ret = __txn_regop_42_read(dbenv, rec.data,
				    &regop42_args)) != 0)
					goto err;
				txnid = regop42_args->txnp->txnid;
				txnop = regop42_args->opcode;
				__os_free(dbenv, regop42_args);
			}

			ret = __db_txnlist_find(dbenv,
			    txninfo, txnid, &status);
			if (ret == DB_NOTFOUND)
				ret = __db_txnlist_add(dbenv, txninfo,
				    txnid, txnop, &lsn);
			else if (ret != 0)
				goto err;
			break;
		case DB___txn_xa_regop:
			/*
			 * It's a prepare.  If its not aborted and
			 * we haven't put the txn on our list yet, it
			 * hasn't been resolved, so apply and restore it.
			 */
			if ((ret = __txn_xa_regop_read(dbenv, rec.data,
			    &prep_args)) != 0)
				goto err;
			ret = __db_txnlist_find(dbenv, txninfo,
			    prep_args->txnp->txnid, &status);
			if (ret == DB_NOTFOUND) {
				if (prep_args->opcode == TXN_ABORT)
					ret = __db_txnlist_add(dbenv, txninfo,
					    prep_args->txnp->txnid,
					    prep_args->opcode, &lsn);
				else if ((ret =
				    __rep_process_txn(dbenv, &rec)) == 0)
					ret = __txn_restore_txn(dbenv,
					    &lsn, prep_args);
			} else if (ret != 0)
				goto err;
			__os_free(dbenv, prep_args);
			break;
		default:
			continue;
		}
	}

	/* It's not an error to have hit the beginning of the log. */
	if (ret == DB_NOTFOUND)
		ret = 0;

done:
err:	t_ret = __log_c_close(logc);

	if (txninfo != NULL)
		__db_txnlist_end(dbenv, txninfo);

	return (ret == 0 ? t_ret : ret);
}

/*
 * __rep_get_limit --
 *	Get the limit on the amount of data that will be sent during a single
 * invocation of __rep_process_message.
 *
 * PUBLIC: int __rep_get_limit __P((DB_ENV *, u_int32_t *, u_int32_t *));
 */
int
__rep_get_limit(dbenv, gbytesp, bytesp)
	DB_ENV *dbenv;
	u_int32_t *gbytesp, *bytesp;
{
	DB_REP *db_rep;
	REP *rep;

	db_rep = dbenv->rep_handle;
	ENV_NOT_CONFIGURED(
	    dbenv, db_rep->region, "DB_ENV->rep_get_limit", DB_INIT_REP);

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		REP_SYSTEM_LOCK(dbenv);
		if (gbytesp != NULL)
			*gbytesp = rep->gbytes;
		if (bytesp != NULL)
			*bytesp = rep->bytes;
		REP_SYSTEM_UNLOCK(dbenv);
	} else {
		if (gbytesp != NULL)
			*gbytesp = db_rep->gbytes;
		if (bytesp != NULL)
			*bytesp = db_rep->bytes;
	}

	return (0);
}

/*
 * __rep_set_limit --
 *	Set a limit on the amount of data that will be sent during a single
 * invocation of __rep_process_message.
 *
 * PUBLIC: int __rep_set_limit __P((DB_ENV *, u_int32_t, u_int32_t));
 */
int
__rep_set_limit(dbenv, gbytes, bytes)
	DB_ENV *dbenv;
	u_int32_t gbytes, bytes;
{
	DB_REP *db_rep;
	REP *rep;

	db_rep = dbenv->rep_handle;
	ENV_NOT_CONFIGURED(
	    dbenv, db_rep->region, "DB_ENV->rep_set_limit", DB_INIT_REP);

	if (bytes > GIGABYTE) {
		gbytes += bytes / GIGABYTE;
		bytes = bytes % GIGABYTE;
	}

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		REP_SYSTEM_LOCK(dbenv);
		rep->gbytes = gbytes;
		rep->bytes = bytes;
		REP_SYSTEM_UNLOCK(dbenv);
	} else {
		db_rep->gbytes = gbytes;
		db_rep->bytes = bytes;
	}

	return (0);
}

/*
 * PUBLIC: int __rep_set_nsites __P((DB_ENV *, int));
 */
int
__rep_set_nsites(dbenv, n)
	DB_ENV *dbenv;
	int n;
{
	DB_REP *db_rep;
	REP *rep;

	if (n <= 0) {
		__db_errx(dbenv,
		    "DB_ENV->rep_set_nsites: nsites must be a positive number");
		return (EINVAL);
	}

	db_rep = dbenv->rep_handle;

	/* TODO: ENV_REQUIRES_CONFIG(... ) and/or ENV_NOT_CONFIGURED (?) */

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		rep->config_nsites = n;
	} else
		db_rep->config_nsites = n;
	return (0);
}

/*
 * PUBLIC: int __rep_get_nsites __P((DB_ENV *, int *));
 */
int
__rep_get_nsites(dbenv, n)
	DB_ENV *dbenv;
	int *n;
{
	DB_REP *db_rep;
	REP *rep;

	db_rep = dbenv->rep_handle;

	/* TODO: ENV_REQUIRES_CONFIG(... ) and/or ENV_NOT_CONFIGURED (?) */

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		*n = rep->config_nsites;
	} else
		*n = db_rep->config_nsites;

	return (0);
}

/*
 * PUBLIC: int __rep_set_priority __P((DB_ENV *, int));
 */
int
__rep_set_priority(dbenv, priority)
	DB_ENV *dbenv;
	int priority;
{
	DB_REP *db_rep;
	REP *rep;

	if (priority < 0) {
		__db_errx(dbenv, "priority may not be negative");
		return (EINVAL);
	}
	db_rep = dbenv->rep_handle;
	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		rep->priority = priority;
	} else
		db_rep->my_priority = priority;
	return (0);
}

/*
 * PUBLIC: int __rep_get_priority __P((DB_ENV *, int *));
 */
int
__rep_get_priority(dbenv, priority)
	DB_ENV *dbenv;
	int *priority;
{
	DB_REP *db_rep;
	REP *rep;

	db_rep = dbenv->rep_handle;
	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		*priority = rep->priority;
	} else
		*priority = db_rep->my_priority;
	return (0);
}

/*
 * PUBLIC: int __rep_set_timeout __P((DB_ENV *, int, db_timeout_t));
 */
int
__rep_set_timeout(dbenv, which, timeout)
	DB_ENV *dbenv;
	int which;
	db_timeout_t timeout;
{
	DB_REP *db_rep;
	REP *rep;
	int ret;

	db_rep = dbenv->rep_handle;
	rep = db_rep->region;
	ret = 0;

	switch (which) {
	case DB_REP_ELECTION_TIMEOUT:
		if (REP_ON(dbenv))
			rep->elect_timeout = timeout;
		else
			db_rep->elect_timeout = timeout;
		break;
#ifdef HAVE_REPLICATION_THREADS
	case DB_REP_ACK_TIMEOUT:
		db_rep->ack_timeout = timeout;
		break;
	case DB_REP_ELECTION_RETRY:
		db_rep->election_retry_wait = timeout;
		break;
	case DB_REP_CONNECTION_RETRY:
		db_rep->connection_retry_wait = timeout;
		break;
#endif
	default:
		__db_errx(dbenv,
		    "Unknown timeout type argument to DB_ENV->rep_set_timeout");
		ret = EINVAL;
	}

	return (ret);
}

/*
 * PUBLIC: int __rep_get_timeout __P((DB_ENV *, int, db_timeout_t *));
 */
int
__rep_get_timeout(dbenv, which, timeout)
	DB_ENV *dbenv;
	int which;
	db_timeout_t *timeout;
{
	DB_REP *db_rep;
	REP *rep;
	int ret;

	db_rep = dbenv->rep_handle;
	rep = db_rep->region;
	ret = 0;

	switch (which) {
	case DB_REP_ELECTION_TIMEOUT:
		if (REP_ON(dbenv))
			*timeout = rep->elect_timeout;
		else
			*timeout = db_rep->elect_timeout;
		break;
#ifdef HAVE_REPLICATION_THREADS
	case DB_REP_ACK_TIMEOUT:
		*timeout = db_rep->ack_timeout;
		break;
	case DB_REP_ELECTION_RETRY:
		*timeout = db_rep->election_retry_wait;
		break;
	case DB_REP_CONNECTION_RETRY:
		*timeout = db_rep->connection_retry_wait;
		break;
#endif
	default:
		__db_errx(dbenv,
		    "Unknown timeout type argument to DB_ENV->rep_get_timeout");
		ret = EINVAL;
	}

	return (ret);
}

/*
 * __rep_get_request --
 *	Get the minimum and maximum number of log records that we wait
 *	before retransmitting.
 *
 * !!!
 * UNDOCUMENTED.
 *
 * PUBLIC: int __rep_get_request __P((DB_ENV *, u_int32_t *, u_int32_t *));
 */
int
__rep_get_request(dbenv, minp, maxp)
	DB_ENV *dbenv;
	u_int32_t *minp, *maxp;
{
	DB_REP *db_rep;
	REP *rep;

	db_rep = dbenv->rep_handle;
	ENV_NOT_CONFIGURED(
	    dbenv, db_rep->region, "DB_ENV->rep_get_request", DB_INIT_REP);

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		/*
		 * We acquire the mtx_region or mtx_clientdb mutexes as needed.
		 */
		REP_SYSTEM_LOCK(dbenv);
		if (minp != NULL)
			*minp = rep->request_gap;
		if (maxp != NULL)
			*maxp = rep->max_gap;
		REP_SYSTEM_UNLOCK(dbenv);
	} else {
		if (minp != NULL)
			*minp = db_rep->request_gap;
		if (maxp != NULL)
			*maxp = db_rep->max_gap;
	}

	return (0);
}

/*
 * __rep_set_request --
 *	Set the minimum and maximum number of log records that we wait
 *	before retransmitting.
 *
 * !!!
 * UNDOCUMENTED.
 *
 * PUBLIC: int __rep_set_request __P((DB_ENV *, u_int32_t, u_int32_t));
 */
int
__rep_set_request(dbenv, min, max)
	DB_ENV *dbenv;
	u_int32_t min, max;
{
	LOG *lp;
	DB_LOG *dblp;
	DB_REP *db_rep;
	REP *rep;

	db_rep = dbenv->rep_handle;
	ENV_NOT_CONFIGURED(
	    dbenv, db_rep->region, "DB_ENV->rep_set_request", DB_INIT_REP);

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		/*
		 * We acquire the mtx_region or mtx_clientdb mutexes as needed.
		 */
		REP_SYSTEM_LOCK(dbenv);
		rep->request_gap = min;
		rep->max_gap = max;
		REP_SYSTEM_UNLOCK(dbenv);

		MUTEX_LOCK(dbenv, rep->mtx_clientdb);
		dblp = dbenv->lg_handle;
		if (dblp != NULL && (lp = dblp->reginfo.primary) != NULL) {
			lp->wait_recs = 0;
			lp->rcvd_recs = 0;
		}
		MUTEX_UNLOCK(dbenv, rep->mtx_clientdb);
	} else {
		db_rep->request_gap = min;
		db_rep->max_gap = max;
	}

	return (0);
}

/*
 * __rep_set_transport --
 *	Set the transport function for replication.
 *
 * PUBLIC: int __rep_set_transport __P((DB_ENV *, int,
 * PUBLIC:     int (*)(DB_ENV *, const DBT *, const DBT *, const DB_LSN *,
 * PUBLIC:     int, u_int32_t)));
 */
int
__rep_set_transport(dbenv, eid, f_send)
	DB_ENV *dbenv;
	int eid;
	int (*f_send) __P((DB_ENV *, const DBT *, const DBT *, const DB_LSN *,
	    int, u_int32_t));
{
	DB_REP *db_rep;
	REP *rep;

	if (f_send == NULL) {
		__db_errx(dbenv,
		    "DB_ENV->rep_set_transport: no send function specified");
		return (EINVAL);
	}

	if (eid < 0) {
		__db_errx(dbenv,
	"DB_ENV->rep_set_transport: eid must be greater than or equal to 0");
		return (EINVAL);
	}

	db_rep = dbenv->rep_handle;
	db_rep->send = f_send;

	if (REP_ON(dbenv)) {
		rep = db_rep->region;
		REP_SYSTEM_LOCK(dbenv);
		rep->eid = eid;
		REP_SYSTEM_UNLOCK(dbenv);
	} else
		db_rep->eid = eid;
	return (0);
}

/*
 * __rep_flush --
 *	Re-push the last log record to all clients, in case they've lost
 *	messages and don't know it.
 *
 * PUBLIC: int __rep_flush __P((DB_ENV *));
 */
int
__rep_flush(dbenv)
	DB_ENV *dbenv;
{
	DBT rec;
	DB_LOGC *logc;
	DB_LSN lsn;
	int ret, t_ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG_XX(
	    dbenv, rep_handle, "DB_ENV->rep_flush", DB_INIT_REP);

	if ((ret = __log_cursor(dbenv, &logc)) != 0)
		return (ret);

	memset(&rec, 0, sizeof(rec));
	memset(&lsn, 0, sizeof(lsn));

	if ((ret = __log_c_get(logc, &lsn, &rec, DB_LAST)) != 0)
		goto err;

	(void)__rep_send_message(dbenv,
	    DB_EID_BROADCAST, REP_LOG, &lsn, &rec, 0, 0);

err:	if ((t_ret = __log_c_close(logc)) != 0 && ret == 0)
		ret = t_ret;
	return (ret);
}

/*
 * __rep_sync --
 *	Force a synchronization to occur between this client and the master.
 *	This is the other half of configuring DELAYCLIENT.
 *
 * PUBLIC: int __rep_sync __P((DB_ENV *, u_int32_t));
 */
int
__rep_sync(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_LSN lsn;
	DB_REP *db_rep;
	LOG *lp;
	REP *rep;
	int master;
	u_int32_t type;

	COMPQUIET(flags, 0);

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG_XX(
	    dbenv, rep_handle, "DB_ENV->rep_sync", DB_INIT_REP);

	dblp = dbenv->lg_handle;
	lp = dblp->reginfo.primary;
	db_rep = dbenv->rep_handle;
	rep = db_rep->region;

	/*
	 * Simple cases.  If we're not in the DELAY state we have nothing
	 * to do.  If we don't know who the master is, send a MASTER_REQ.
	 */
	MUTEX_LOCK(dbenv, rep->mtx_clientdb);
	lsn = lp->verify_lsn;
	MUTEX_UNLOCK(dbenv, rep->mtx_clientdb);
	REP_SYSTEM_LOCK(dbenv);
	master = rep->master_id;
	if (master == DB_EID_INVALID) {
		REP_SYSTEM_UNLOCK(dbenv);
		(void)__rep_send_message(dbenv, DB_EID_BROADCAST,
		    REP_MASTER_REQ, NULL, NULL, 0, 0);
		return (0);
	}
	/*
	 * We want to hold the rep mutex to test and then clear the
	 * DELAY flag.  Racing threads in here could otherwise result
	 * in dual data streams.
	 */
	if (!F_ISSET(rep, REP_F_DELAY)) {
		REP_SYSTEM_UNLOCK(dbenv);
		return (0);
	}

	/*
	 * If we get here, we clear the delay flag and kick off a
	 * synchronization.  From this point forward, we will
	 * synchronize until the next time the master changes.
	 */
	F_CLR(rep, REP_F_DELAY);
	REP_SYSTEM_UNLOCK(dbenv);
	/*
	 * When we set REP_F_DELAY, we set verify_lsn to the real verify
	 * lsn if we need to verify, or we zeroed it out if this is a client
	 * that needs to sync up from the beginning.  So, send the type
	 * of message now that __rep_new_master delayed sending.
	 */
	if (IS_ZERO_LSN(lsn))
		type = REP_ALL_REQ;
	else
		type = REP_VERIFY_REQ;
	(void)__rep_send_message(dbenv, master, type, &lsn, NULL, 0,
	    DB_REP_ANYWHERE);
	return (0);
}

/*
 * __rep_conv_vers --
 *	Convert from a log version to the replication message version
 *	that release used.
 */
static u_int32_t
__rep_conv_vers(dbenv, log_ver)
	DB_ENV *dbenv;
	u_int32_t log_ver;
{
	COMPQUIET(dbenv, NULL);
	if (log_ver == DB_LOGVERSION_42)
		return (DB_REPVERSION_42);
	if (log_ver == DB_LOGVERSION_43)
		return (DB_REPVERSION_43);
	if (log_ver == DB_LOGVERSION_44)
		return (DB_REPVERSION_44);
	if (log_ver == DB_LOGVERSION_45)
		return (DB_REPVERSION_45);
	return (DB_REPVERSION_INVALID);
}
