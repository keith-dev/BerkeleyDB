/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: env_open.c,v 12.36 2005/10/31 02:22:28 bostic Exp $
 */

#include "db_config.h"

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "db_int.h"
#include "dbinc/crypto.h"
#include "dbinc/db_page.h"
#include "dbinc/db_shash.h"
#include "dbinc/btree.h"
#include "dbinc/hash.h"
#include "dbinc/fop.h"
#include "dbinc/lock.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"
#include "dbinc/qam.h"
#include "dbinc/txn.h"

static int __db_parse __P((DB_ENV *, char *));
static int __db_tmp_open __P((DB_ENV *, u_int32_t, char *, DB_FH **));
static int __env_config __P((DB_ENV *, const char *, u_int32_t));
static int __env_refresh __P((DB_ENV *, u_int32_t, int));
static int __env_remove_int __P((DB_ENV *, const char *, u_int32_t));

/*
 * db_version --
 *	Return version information.
 *
 * EXTERN: char *db_version __P((int *, int *, int *));
 */
char *
db_version(majverp, minverp, patchp)
	int *majverp, *minverp, *patchp;
{
	if (majverp != NULL)
		*majverp = DB_VERSION_MAJOR;
	if (minverp != NULL)
		*minverp = DB_VERSION_MINOR;
	if (patchp != NULL)
		*patchp = DB_VERSION_PATCH;
	return ((char *)DB_VERSION_STRING);
}

/*
 * __env_open_pp --
 *	DB_ENV->open pre/post processing.
 *
 * PUBLIC: int __env_open_pp __P((DB_ENV *, const char *, u_int32_t, int));
 */
int
__env_open_pp(dbenv, db_home, flags, mode)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
	int mode;
{
	DB_THREAD_INFO *ip;
	u_int32_t orig_flags;
	int need_recovery, ret, t_ret;

	need_recovery = 0;

#undef	OKFLAGS
#define	OKFLAGS								\
	(DB_CREATE | DB_INIT_CDB | DB_INIT_LOCK | DB_INIT_LOG |		\
	DB_INIT_MPOOL | DB_INIT_REP | DB_INIT_TXN | DB_LOCKDOWN |	\
	DB_PRIVATE | DB_RECOVER | DB_RECOVER_FATAL | DB_REGISTER |	\
	DB_SYSTEM_MEM | DB_THREAD | DB_USE_ENVIRON | DB_USE_ENVIRON_ROOT)
#undef	OKFLAGS_CDB
#define	OKFLAGS_CDB							\
	(DB_CREATE | DB_INIT_CDB | DB_INIT_MPOOL | DB_LOCKDOWN |	\
	DB_PRIVATE | DB_SYSTEM_MEM | DB_THREAD |			\
	DB_USE_ENVIRON | DB_USE_ENVIRON_ROOT)

	if ((ret = __db_fchk(dbenv, "DB_ENV->open", flags, OKFLAGS)) != 0)
		return (ret);
	if ((ret = __db_fcchk(
	    dbenv, "DB_ENV->open", flags, DB_INIT_CDB, ~OKFLAGS_CDB)) != 0)
		return (ret);
	if ((ret = __db_fcchk(dbenv, "DB_ENV->open", flags,
	    DB_PRIVATE, DB_REGISTER | DB_SYSTEM_MEM)) != 0)
		return (ret);
	if (LF_ISSET(DB_INIT_REP)) {
		if (!LF_ISSET(DB_INIT_LOCK)) {
			__db_err(dbenv, "replication requires locking support");
			return (EINVAL);
		}
		if (!LF_ISSET(DB_INIT_TXN)) {
			__db_err(
			    dbenv, "replication requires transaction support");
			return (EINVAL);
		}
	}
	if (LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL)) {
		if ((ret = __db_fcchk(dbenv,
		    "DB_ENV->open", flags, DB_RECOVER, DB_RECOVER_FATAL)) != 0)
			return (ret);
		if (!LF_ISSET(DB_CREATE)) {
			__db_err(dbenv, "recovery requires the create flag");
			return (EINVAL);
		}
		if (!LF_ISSET(DB_INIT_TXN)) {
			__db_err(
			    dbenv, "recovery requires transaction support");
			return (EINVAL);
		}
	}

	/*
	 * Currently we support one kind of mutex that is intra-process only,
	 * POSIX 1003.1 pthreads, because a variety of systems don't support
	 * the full pthreads API, and our only alternative is test-and-set.
	 */
#ifdef HAVE_MUTEX_THREAD_ONLY
	if (!LF_ISSET(DB_PRIVATE)) {
		__db_err(dbenv,
	 "Berkeley DB library configured to support only private environments");
		return (EINVAL);
	}
#endif

#if defined(HAVE_MUTEX_FCNTL)
	/*
	 * !!!
	 * We need a file descriptor for fcntl(2) locking.  We use the file
	 * handle from the REGENV file for this purpose.
	 *
	 * Since we may be using shared memory regions, e.g., shmget(2), and
	 * not a mapped-in regular file, the backing file may be only a few
	 * bytes in length.  So, this depends on the ability to call fcntl to
	 * lock file offsets much larger than the actual physical file.  I
	 * think that's safe -- besides, very few systems actually need this
	 * kind of support, SunOS is the only one still in wide use of which
	 * I'm aware.
	 *
	 * The error case is if an application lacks spinlocks and wants to be
	 * threaded.  That doesn't work because fcntl will lock the underlying
	 * process, including all its threads.
	 */
	if (F_ISSET(dbenv, DB_ENV_THREAD)) {
		__db_err(dbenv,
	    "architecture lacks fast mutexes: applications cannot be threaded");
		return (EINVAL);
	}
#endif

	if (LF_ISSET(DB_INIT_REP) && !__os_support_replication()) {
		__db_err(dbenv,
	     "Berkeley DB library does not support replication on this system");
		return (EINVAL);
	}

	/*
	 * If we're going to register with the environment, that's the first
	 * thing we do.
	 */
	if (LF_ISSET(DB_REGISTER)) {
		if (!__os_support_db_register()) {
			__db_err(dbenv,
	     "Berkeley DB library does not support DB_REGISTER on this system");
			return (EINVAL);
		}

		if ((ret =
		    __envreg_register(dbenv, db_home, &need_recovery)) != 0)
			return (ret);
		if (need_recovery) {
			if (!LF_ISSET(DB_RECOVER)) {
				__db_err(dbenv,
		    "No recovery flag was specified, and recovery is needed");
				ret = DB_RUNRECOVERY;
				goto err;
			}
		} else
			LF_CLR(DB_RECOVER | DB_RECOVER_FATAL);
	}

	/*
	 * If we're doing recovery, destroy the environment so that we create
	 * all the regions from scratch.  The major concern I have is if the
	 * application stomps the environment with a rogue pointer.  We have
	 * no way of detecting that, and we could be forced into a situation
	 * where we start up and then crash, repeatedly.
	 *
	 * Note that we do not check any flags like DB_PRIVATE before calling
	 * remove.  We don't care if the current environment was private or
	 * not, we just want to nail any files that are left-over for whatever
	 * reason, from whatever session.
	 */
	if (LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL)) {
		orig_flags = dbenv->flags;
		if ((ret = __env_remove_int(dbenv, db_home, DB_FORCE)) != 0 ||
		    (ret = __env_refresh(dbenv, orig_flags, 0)) != 0)
			goto err;
	}

	ret = __env_open(dbenv, db_home, flags, mode);
	if (ret == 0 && dbenv->thr_hashtab != NULL &&
	    (t_ret = __env_set_state(dbenv, &ip, THREAD_OUT)) != 0)
		ret = t_ret;

err:	if (need_recovery) {
		/*
		 * If recovery succeeded, release our exclusive lock, other
		 * processes can now proceed.
		 *
		 * If recovery failed, unregister now.
		 */
		if (ret == 0 && (t_ret = __envreg_xunlock(dbenv)) != 0)
			ret = t_ret;
		if (ret != 0)
			(void)__envreg_unregister(dbenv, 1);
	}

	return (ret);
}

/*
 * __env_open --
 *	DB_ENV->open.
 *
 * PUBLIC: int __env_open __P((DB_ENV *, const char *, u_int32_t, int));
 */
int
__env_open(dbenv, db_home, flags, mode)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
	int mode;
{
	DB_THREAD_INFO *ip;
	REGINFO *infop;
	u_int32_t init_flags, orig_flags;
	int rep_check, ret;

	orig_flags = dbenv->flags;
	rep_check = 0;
	ip = NULL;

	/* Initialize the DB_ENV structure. */
	if ((ret = __env_config(dbenv, db_home, flags)) != 0)
		goto err;

	/* Convert the DB_ENV->open flags to internal flags. */
	if (LF_ISSET(DB_CREATE))
		F_SET(dbenv, DB_ENV_CREATE);
	if (LF_ISSET(DB_LOCKDOWN))
		F_SET(dbenv, DB_ENV_LOCKDOWN);
	if (LF_ISSET(DB_PRIVATE))
		F_SET(dbenv, DB_ENV_PRIVATE);
	if (LF_ISSET(DB_RECOVER_FATAL))
		F_SET(dbenv, DB_ENV_FATAL);
	if (LF_ISSET(DB_SYSTEM_MEM))
		F_SET(dbenv, DB_ENV_SYSTEM_MEM);
	if (LF_ISSET(DB_THREAD))
		F_SET(dbenv, DB_ENV_THREAD);

	/* Default permissions are read-write for both owner and group. */
	dbenv->db_mode = mode == 0 ? __db_omode("rw-rw----") : mode;

	/*
	 * Flags saved in the init_flags field of the environment, representing
	 * flags to DB_ENV->set_flags and DB_ENV->open that need to be set.
	 */
#define	DB_INITENV_CDB		0x0001	/* DB_INIT_CDB */
#define	DB_INITENV_CDB_ALLDB	0x0002	/* DB_INIT_CDB_ALLDB */
#define	DB_INITENV_LOCK		0x0004	/* DB_INIT_LOCK */
#define	DB_INITENV_LOG		0x0008	/* DB_INIT_LOG */
#define	DB_INITENV_MPOOL	0x0010	/* DB_INIT_MPOOL */
#define	DB_INITENV_REP		0x0020	/* DB_INIT_REP */
#define	DB_INITENV_TXN		0x0040	/* DB_INIT_TXN */

	/*
	 * Create/join the environment.  We pass in the flags of interest to
	 * a thread subsequently joining an environment we create.  If we're
	 * not the ones to create the environment, our flags will be updated
	 * to match the existing environment.
	 */
	init_flags = 0;
	if (LF_ISSET(DB_INIT_CDB))
		FLD_SET(init_flags, DB_INITENV_CDB);
	if (LF_ISSET(DB_INIT_LOCK))
		FLD_SET(init_flags, DB_INITENV_LOCK);
	if (LF_ISSET(DB_INIT_LOG))
		FLD_SET(init_flags, DB_INITENV_LOG);
	if (LF_ISSET(DB_INIT_MPOOL))
		FLD_SET(init_flags, DB_INITENV_MPOOL);
	if (LF_ISSET(DB_INIT_REP))
		FLD_SET(init_flags, DB_INITENV_REP);
	if (LF_ISSET(DB_INIT_TXN))
		FLD_SET(init_flags, DB_INITENV_TXN);
	if (F_ISSET(dbenv, DB_ENV_CDB_ALLDB))
		FLD_SET(init_flags, DB_INITENV_CDB_ALLDB);
	if ((ret = __db_e_attach(dbenv, &init_flags)) != 0)
		goto err;

	/*
	 * __db_e_attach will return the saved init_flags field, which contains
	 * the DB_INIT_* flags used when the environment was created.
	 *
	 * We may be joining an environment -- reset our flags to match the
	 * ones in the environment.
	 */
	if (FLD_ISSET(init_flags, DB_INITENV_CDB))
		LF_SET(DB_INIT_CDB);
	if (FLD_ISSET(init_flags, DB_INITENV_LOCK))
		LF_SET(DB_INIT_LOCK);
	if (FLD_ISSET(init_flags, DB_INITENV_LOG))
		LF_SET(DB_INIT_LOG);
	if (FLD_ISSET(init_flags, DB_INITENV_MPOOL))
		LF_SET(DB_INIT_MPOOL);
	if (FLD_ISSET(init_flags, DB_INITENV_REP))
		LF_SET(DB_INIT_REP);
	if (FLD_ISSET(init_flags, DB_INITENV_TXN))
		LF_SET(DB_INIT_TXN);
	if (FLD_ISSET(init_flags, DB_INITENV_CDB_ALLDB) &&
	    (ret = __env_set_flags(dbenv, DB_CDB_ALLDB, 1)) != 0)
		goto err;

	/*
	 * Save the flags matching the database environment: we've replaced
	 * the argument flags with the flags corresponding to the existing,
	 * underlying set of subsystems.
	 */
	dbenv->open_flags = flags;

	/* Initialize for CDB product. */
	if (LF_ISSET(DB_INIT_CDB)) {
		LF_SET(DB_INIT_LOCK);
		F_SET(dbenv, DB_ENV_CDB);
	}

	/*
	 * The DB_ENV structure has been initialized.  This has to be set
	 * before we start calling into the subsystems, some of them look
	 * for it.
	 */
	F_SET(dbenv, DB_ENV_OPEN_CALLED);

	/*
	 * Initialize the subsystems.
	 *
	 * Initialize the mutex regions first.  There's no ordering requirement,
	 * but it's simpler to get this in place so we don't have to keep track
	 * of mutexes for later allocation, once the mutex region is created we
	 * can go ahead and do the allocation for real.
	 */
	if ((ret = __mutex_open(dbenv)) != 0)
		goto err;

	/* __mutex_open creates the thread info region, enter it now. */
	ENV_ENTER(dbenv, ip);

	/*
	 * Initialize the replication area next, so that we can lock out this
	 * call if we're currently running recovery for replication.
	 */
	if (LF_ISSET(DB_INIT_REP) && (ret = __rep_open(dbenv)) != 0)
		goto err;

	rep_check = IS_ENV_REPLICATED(dbenv) ? 1 : 0;
	if (rep_check && (ret = __env_rep_enter(dbenv, 0)) != 0)
		goto err;

	if (LF_ISSET(DB_INIT_MPOOL))
		if ((ret = __memp_open(dbenv)) != 0)
			goto err;
	/*
	 * Initialize the ciphering area prior to any running of recovery so
	 * that we can initialize the keys, etc. before recovery.
	 *
	 * !!!
	 * This must be after the mpool init, but before the log initialization
	 * because log_open may attempt to run log_recover during its open.
	 */
	if (LF_ISSET(DB_INIT_MPOOL | DB_INIT_LOG | DB_INIT_TXN) &&
	    (ret = __crypto_region_init(dbenv)) != 0)
		goto err;

	/*
	 * Transactions imply logging but do not imply locking.  While almost
	 * all applications want both locking and logging, it would not be
	 * unreasonable for a single threaded process to want transactions for
	 * atomicity guarantees, but not necessarily need concurrency.
	 */
	if (LF_ISSET(DB_INIT_LOG | DB_INIT_TXN))
		if ((ret = __log_open(dbenv)) != 0)
			goto err;
	if (LF_ISSET(DB_INIT_LOCK))
		if ((ret = __lock_open(dbenv)) != 0)
			goto err;

	if (LF_ISSET(DB_INIT_TXN)) {
		if ((ret = __txn_open(dbenv)) != 0)
			goto err;

		/*
		 * If the application is running with transactions, initialize
		 * the function tables.
		 */
		if ((ret = __bam_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __crdel_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __db_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __dbreg_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __fop_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __ham_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __qam_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
		if ((ret = __txn_init_recover(dbenv, &dbenv->recover_dtab,
		    &dbenv->recover_dtab_size)) != 0)
			goto err;
	}

	/*
	 * Initialize the DB list, and its mutex as necessary.  If the env
	 * handle isn't free-threaded we don't need a mutex because there
	 * will never be more than a single DB handle on the list.  If the
	 * mpool wasn't initialized, then we can't ever open a DB handle.
	 *
	 * We also need to initialize the MT mutex as necessary, so do them
	 * both.
	 *
	 * !!!
	 * This must come after the __memp_open call above because if we are
	 * recording mutexes for system resources, we will do it in the mpool
	 * region for environments and db handles.  So, the mpool region must
	 * already be initialized.
	 */
	LIST_INIT(&dbenv->dblist);
	if (LF_ISSET(DB_INIT_MPOOL)) {
		if ((ret = __mutex_alloc(dbenv, MTX_ENV_DBLIST,
		    DB_MUTEX_THREAD, &dbenv->mtx_dblist)) != 0)
			goto err;
		if ((ret = __mutex_alloc(dbenv,
		    MTX_TWISTER, DB_MUTEX_THREAD, &dbenv->mtx_mt)) != 0)
			goto err;

		/* Register DB's pgin/pgout functions.  */
		if ((ret = __memp_register(
		    dbenv, DB_FTYPE_SET, __db_pgin, __db_pgout)) != 0)
			goto err;
	}

	/* Perform recovery for any previous run. */
	if (LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL) &&
	    (ret = __db_apprec(dbenv, NULL, NULL, 1,
	    LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL))) != 0)
		goto err;

	/*
	 * If we've created the regions, are running with transactions, and did
	 * not just run recovery, we need to log the fact that the transaction
	 * IDs got reset.
	 *
	 * If we ran recovery, there may be prepared-but-not-yet-committed
	 * transactions that need to be resolved.  Recovery resets the minimum
	 * transaction ID and logs the reset if that's appropriate, so we
	 * don't need to do anything here in the recover case.
	 */
	infop = dbenv->reginfo;
	if (TXN_ON(dbenv) &&
	    !F_ISSET(dbenv, DB_ENV_LOG_INMEMORY) &&
	    F_ISSET(infop, REGION_CREATE) &&
	    !LF_ISSET(DB_RECOVER | DB_RECOVER_FATAL) &&
	    (ret = __txn_reset(dbenv)) != 0)
		goto err;

	/* The database environment is ready for business. */
	if ((ret = __db_e_golive(dbenv)) != 0)
		goto err;

	if (rep_check && (ret = __env_db_rep_exit(dbenv)) != 0)
		goto err;

	ENV_LEAVE(dbenv, ip);
	return (0);

err:	/*
	 * If we fail after creating the regions, panic and remove them.
	 *
	 * !!!
	 * No need to call __env_db_rep_exit, that work is done by the calls to
	 * __env_refresh.
	 */
	infop = dbenv->reginfo;
	if (infop != NULL && F_ISSET(infop, REGION_CREATE)) {
		ret = __db_panic(dbenv, ret);

		/* Refresh the DB_ENV so we can use it to call remove. */
		(void)__env_refresh(dbenv, orig_flags, rep_check);
		(void)__env_remove_int(dbenv, db_home, DB_FORCE);
		(void)__env_refresh(dbenv, orig_flags, 0);
	} else
		(void)__env_refresh(dbenv, orig_flags, rep_check);

	if (ip != NULL)
		ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __env_remove --
 *	DB_ENV->remove.
 *
 * PUBLIC: int __env_remove __P((DB_ENV *, const char *, u_int32_t));
 */
int
__env_remove(dbenv, db_home, flags)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
{
	int ret, t_ret;

#undef	OKFLAGS
#define	OKFLAGS								\
	(DB_FORCE | DB_USE_ENVIRON | DB_USE_ENVIRON_ROOT)

	/* Validate arguments. */
	if ((ret = __db_fchk(dbenv, "DB_ENV->remove", flags, OKFLAGS)) != 0)
		return (ret);

	ENV_ILLEGAL_AFTER_OPEN(dbenv, "DB_ENV->remove");

	ret = __env_remove_int(dbenv, db_home, flags);

	if ((t_ret = __env_close(dbenv, 0)) != 0 && ret == 0)
		ret = t_ret;

	return (ret);
}

/*
 * __env_remove_int --
 *	Discard an environment, internal version.
 */
static int
__env_remove_int(dbenv, db_home, flags)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
{
	int ret;

	/* Initialize the DB_ENV structure. */
	if ((ret = __env_config(dbenv, db_home, flags)) != 0)
		return (ret);

	/* The DB_ENV structure has been initialized. */
	F_SET(dbenv, DB_ENV_OPEN_CALLED);

	/* Remove the environment. */
	return (__db_e_remove(dbenv, flags));
}

/*
 * __env_config --
 *	Initialization of the DB_ENV structure, read the DB_CONFIG file.
 */
static int
__env_config(dbenv, db_home, flags)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
{
	FILE *fp;
	int ret;
	char *p, buf[256];

	/*
	 * Set the database home.  Do this before calling __db_appname,
	 * it uses the home directory.
	 */
	if ((ret = __db_home(dbenv, db_home, flags)) != 0)
		return (ret);

	/* Parse the config file. */
	p = NULL;
	if ((ret =
	    __db_appname(dbenv, DB_APP_NONE, "DB_CONFIG", 0, NULL, &p)) != 0)
		return (ret);
	if (p == NULL)
		fp = NULL;
	else {
		fp = fopen(p, "r");
		__os_free(dbenv, p);
	}

	if (fp != NULL) {
		while (fgets(buf, sizeof(buf), fp) != NULL) {
			if ((p = strchr(buf, '\n')) != NULL)
				*p = '\0';
			else if (strlen(buf) + 1 == sizeof(buf)) {
				__db_err(dbenv, "DB_CONFIG: line too long");
				(void)fclose(fp);
				return (EINVAL);
			}
			if (buf[0] == '\0' ||
			    buf[0] == '#' || isspace((int)buf[0]))
				continue;

			if ((ret = __db_parse(dbenv, buf)) != 0) {
				(void)fclose(fp);
				return (ret);
			}
		}
		(void)fclose(fp);
	}

	/*
	 * If no temporary directory path was specified in the config file,
	 * choose one.
	 */
	if (dbenv->db_tmp_dir == NULL && (ret = __os_tmpdir(dbenv, flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __env_close_pp --
 *	DB_ENV->close pre/post processor.
 *
 * PUBLIC: int __env_close_pp __P((DB_ENV *, u_int32_t));
 */
int
__env_close_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	int rep_check, ret, t_ret;

	ret = 0;

	PANIC_CHECK(dbenv);

	ENV_ENTER(dbenv, ip);
	/*
	 * Validate arguments, but as a DB_ENV handle destructor, we can't
	 * fail.
	 */
	if (flags != 0 &&
	    (t_ret = __db_ferr(dbenv, "DB_ENV->close", 0)) != 0 && ret == 0)
		ret = t_ret;

	rep_check = IS_ENV_REPLICATED(dbenv) ? 1 : 0;
	if (rep_check && (t_ret = __env_rep_enter(dbenv, 0)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __env_close(dbenv, rep_check)) != 0 && ret == 0)
		ret = t_ret;

	/* Don't ENV_LEAVE as we have already detached from the region. */
	return (ret);
}

/*
 * __env_close --
 *	DB_ENV->close.
 *
 * PUBLIC: int __env_close __P((DB_ENV *, int));
 */
int
__env_close(dbenv, rep_check)
	DB_ENV *dbenv;
	int rep_check;
{
	int ret, t_ret;
	char **p;

	ret = 0;

	/*
	 * Before checking the reference count, we have to see if we were in
	 * the middle of restoring transactions and need to close the open
	 * files.
	 */
	if (TXN_ON(dbenv) && (t_ret = __txn_preclose(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	if (REP_ON(dbenv) &&
	    (t_ret = __rep_preclose(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Detach from the regions and undo the allocations done by
	 * DB_ENV->open.
	 */
	if ((t_ret = __env_refresh(dbenv, 0, rep_check)) != 0 && ret == 0)
		ret = t_ret;

	/* Do per-subsystem close. */
	if ((t_ret = __lock_dbenv_close(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	if ((t_ret = __rep_dbenv_close(dbenv)) != 0 && ret == 0)
		ret = t_ret;

#ifdef HAVE_CRYPTO
	/*
	 * Crypto comes last, because higher level close functions need
	 * cryptography.
	 */
	if ((t_ret = __crypto_dbenv_close(dbenv)) != 0 && ret == 0)
		ret = t_ret;
#endif

	/* Release any string-based configuration parameters we've copied. */
	if (dbenv->db_log_dir != NULL)
		__os_free(dbenv, dbenv->db_log_dir);
	if (dbenv->db_tmp_dir != NULL)
		__os_free(dbenv, dbenv->db_tmp_dir);
	if (dbenv->db_data_dir != NULL) {
		for (p = dbenv->db_data_dir; *p != NULL; ++p)
			__os_free(dbenv, *p);
		__os_free(dbenv, dbenv->db_data_dir);
	}

	/* If we're registered, clean up. */
	if (dbenv->registry != NULL) {
		(void)__envreg_unregister(dbenv, 0);
		dbenv->registry = NULL;
	}

	/* Discard the structure. */
	memset(dbenv, CLEAR_BYTE, sizeof(DB_ENV));
	__os_free(NULL, dbenv);

	return (ret);
}

/*
 * __env_refresh --
 *	Refresh the DB_ENV structure, releasing resources allocated by
 * DB_ENV->open, and returning it to the state it was in just before
 * open was called.  (Note that this means that any state set by
 * pre-open configuration functions must be preserved.)
 */
static int
__env_refresh(dbenv, orig_flags, rep_check)
	DB_ENV *dbenv;
	u_int32_t orig_flags;
	int rep_check;
{
	DB *ldbp;
	DB_THREAD_INFO *ip;
	int ret, t_ret;

	ret = 0;

	/*
	 * Refresh subsystems, in the reverse order they were opened (txn
	 * must be first, it may want to discard locks and flush the log).
	 *
	 * !!!
	 * Note that these functions, like all of __env_refresh, only undo
	 * the effects of __env_open.  Functions that undo work done by
	 * db_env_create or by a configuration function should go in
	 * __env_close.
	 */
	if (TXN_ON(dbenv) &&
	    (t_ret = __txn_dbenv_refresh(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	if (LOGGING_ON(dbenv) &&
	    (t_ret = __log_dbenv_refresh(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Locking should come after logging, because closing log results
	 * in files closing which may require locks being released.
	 */
	if (LOCKING_ON(dbenv)) {
		if (!F_ISSET(dbenv, DB_ENV_THREAD) &&
		    dbenv->env_lref != NULL && (t_ret = __lock_id_free(dbenv,
		    ((DB_LOCKER *)dbenv->env_lref)->id)) != 0 && ret == 0)
			ret = t_ret;
		dbenv->env_lref = NULL;

		if ((t_ret = __lock_dbenv_refresh(dbenv)) != 0 && ret == 0)
			ret = t_ret;
	}

	/*
	 * Discard DB list and its mutex.
	 * Discard the MT mutex.
	 *
	 * !!!
	 * This must be done before we close the mpool region because we
	 * may have allocated the DB handle mutex in the mpool region.
	 * It must be done *after* we close the log region, though, because
	 * we close databases and try to acquire the mutex when we close
	 * log file handles.  Ick.
	 */
	if (dbenv->db_ref != 0) {
		__db_err(dbenv,
		    "Database handles still open at environment close");
		for (ldbp = LIST_FIRST(&dbenv->dblist);
		    ldbp != NULL; ldbp = LIST_NEXT(ldbp, dblistlinks))
			__db_err(dbenv, "Open database handle: %s%s%s",
			    ldbp->fname == NULL ? "unnamed" : ldbp->fname,
			    ldbp->dname == NULL ? "" : "/",
			    ldbp->dname == NULL ? "" : ldbp->dname);
		if (ret == 0)
			ret = EINVAL;
	}
	LIST_INIT(&dbenv->dblist);

	if ((t_ret = __mutex_free(dbenv, &dbenv->mtx_dblist)) != 0 && ret == 0)
		ret = t_ret;
	if ((t_ret = __mutex_free(dbenv, &dbenv->mtx_mt)) != 0 && ret == 0)
		ret = t_ret;

	if (dbenv->mt != NULL) {
		__os_free(dbenv, dbenv->mt);
		dbenv->mt = NULL;
	}

	if (MPOOL_ON(dbenv)) {
		/*
		 * If it's a private environment, flush the contents to disk.
		 * Recovery would have put everything back together, but it's
		 * faster and cleaner to flush instead.
		 */
		if (F_ISSET(dbenv, DB_ENV_PRIVATE) &&
		    (t_ret = __memp_sync(dbenv, NULL)) != 0 && ret == 0)
			ret = t_ret;
		if ((t_ret = __memp_dbenv_refresh(dbenv)) != 0 && ret == 0)
			ret = t_ret;
	}

	/*
	 * If we're included in a shared replication handle count, this
	 * is our last chance to decrement that count.
	 *
	 * !!!
	 * We can't afford to do anything dangerous after we decrement the
	 * handle count, of course, as replication may be proceeding with
	 * client recovery.  However, since we're discarding the regions
	 * as soon as we drop the handle count, there's little opportunity
	 * to do harm.
	 */
	if (rep_check && (t_ret = __env_db_rep_exit(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	/*
	 * Detach from the region.
	 *
	 * Must come after we call __env_db_rep_exit above.
	 */
	__rep_dbenv_refresh(dbenv);

	/*
	 * Mark the thread as out of the env before we get rid
	 * of the handles needed to do so.
	 */
	if (dbenv->thr_hashtab != NULL &&
	    (t_ret = __env_set_state(dbenv, &ip, THREAD_OUT)) != 0 && ret == 0)
		ret = t_ret;

	if (MUTEX_ON(dbenv) &&
	    (t_ret = __mutex_dbenv_refresh(dbenv)) != 0 && ret == 0)
		ret = t_ret;

	if (dbenv->reginfo != NULL) {
		if ((t_ret = __db_e_detach(dbenv, 0)) != 0 && ret == 0)
			ret = t_ret;
		/*
		 * !!!
		 * Don't free dbenv->reginfo or set the reference to NULL,
		 * that was done by __db_e_detach().
		 */
	}

	/* Undo changes and allocations done by __env_open. */
	if (dbenv->db_home != NULL) {
		__os_free(dbenv, dbenv->db_home);
		dbenv->db_home = NULL;
	}
	if (dbenv->db_abshome != NULL) {
		__os_free(dbenv, dbenv->db_abshome);
		dbenv->db_abshome = NULL;
	}
	if (dbenv->mutex_iq != NULL) {
		__os_free(dbenv, dbenv->mutex_iq);
		dbenv->mutex_iq = NULL;
	}

	dbenv->open_flags = 0;
	dbenv->db_mode = 0;

	if (dbenv->recover_dtab != NULL) {
		__os_free(dbenv, dbenv->recover_dtab);
		dbenv->recover_dtab = NULL;
		dbenv->recover_dtab_size = 0;
	}

	dbenv->flags = orig_flags;

	return (ret);
}

#define	DB_ADDSTR(add) {						\
	/*								\
	 * The string might be NULL or zero-length, and the p[-1]	\
	 * might indirect to before the beginning of our buffer.	\
	 */								\
	if ((add) != NULL && (add)[0] != '\0') {			\
		/* If leading slash, start over. */			\
		if (__os_abspath(add)) {				\
			p = str;					\
			slash = 0;					\
		}							\
		/* Append to the current string. */			\
		len = strlen(add);					\
		if (slash)						\
			*p++ = PATH_SEPARATOR[0];			\
		memcpy(p, add, len);					\
		p += len;						\
		slash = strchr(PATH_SEPARATOR, p[-1]) == NULL;		\
	}								\
}

/*
 * __env_get_open_flags
 *	Retrieve the flags passed to DB_ENV->open.
 *
 * PUBLIC: int __env_get_open_flags __P((DB_ENV *, u_int32_t *));
 */
int
__env_get_open_flags(dbenv, flagsp)
	DB_ENV *dbenv;
	u_int32_t *flagsp;
{
	ENV_ILLEGAL_BEFORE_OPEN(dbenv, "DB_ENV->get_open_flags");

	*flagsp = dbenv->open_flags;
	return (0);
}

/*
 * __db_appname --
 *	Given an optional DB environment, directory and file name and type
 *	of call, build a path based on the DB_ENV->open rules, and return
 *	it in allocated space.
 *
 * PUBLIC: int __db_appname __P((DB_ENV *, APPNAME,
 * PUBLIC:    const char *, u_int32_t, DB_FH **, char **));
 */
int
__db_appname(dbenv, appname, file, tmp_oflags, fhpp, namep)
	DB_ENV *dbenv;
	APPNAME appname;
	const char *file;
	u_int32_t tmp_oflags;
	DB_FH **fhpp;
	char **namep;
{
	size_t len, str_len;
	int data_entry, ret, slash, tmp_create;
	const char *a, *b;
	char *p, *str;

	a = b = NULL;
	data_entry = -1;
	tmp_create = 0;

	/*
	 * We don't return a name when creating temporary files, just a file
	 * handle.  Default to an error now.
	 */
	if (fhpp != NULL)
		*fhpp = NULL;
	if (namep != NULL)
		*namep = NULL;

	/*
	 * Absolute path names are never modified.  If the file is an absolute
	 * path, we're done.
	 */
	if (file != NULL && __os_abspath(file))
		return (__os_strdup(dbenv, file, namep));

	/* Everything else is relative to the environment home. */
	if (dbenv != NULL)
		a = dbenv->db_home;

retry:	/*
	 * DB_APP_NONE:
	 *      DB_HOME/file
	 * DB_APP_DATA:
	 *      DB_HOME/DB_DATA_DIR/file
	 * DB_APP_LOG:
	 *      DB_HOME/DB_LOG_DIR/file
	 * DB_APP_TMP:
	 *      DB_HOME/DB_TMP_DIR/<create>
	 */
	switch (appname) {
	case DB_APP_NONE:
		break;
	case DB_APP_DATA:
		if (dbenv != NULL && dbenv->db_data_dir != NULL &&
		    (b = dbenv->db_data_dir[++data_entry]) == NULL) {
			data_entry = -1;
			b = dbenv->db_data_dir[0];
		}
		break;
	case DB_APP_LOG:
		if (dbenv != NULL)
			b = dbenv->db_log_dir;
		break;
	case DB_APP_TMP:
		if (dbenv != NULL)
			b = dbenv->db_tmp_dir;
		tmp_create = 1;
		break;
	}

	len =
	    (a == NULL ? 0 : strlen(a) + 1) +
	    (b == NULL ? 0 : strlen(b) + 1) +
	    (file == NULL ? 0 : strlen(file) + 1);

	/*
	 * Allocate space to hold the current path information, as well as any
	 * temporary space that we're going to need to create a temporary file
	 * name.
	 */
#define	DB_TRAIL	"BDBXXXXX"
	str_len = len + sizeof(DB_TRAIL) + 10;
	if ((ret = __os_malloc(dbenv, str_len, &str)) != 0)
		return (ret);

	slash = 0;
	p = str;
	DB_ADDSTR(a);
	DB_ADDSTR(b);
	DB_ADDSTR(file);
	*p = '\0';

	/*
	 * If we're opening a data file, see if it exists.  If it does,
	 * return it, otherwise, try and find another one to open.
	 */
	if (__os_exists(str, NULL) != 0 && data_entry != -1) {
		__os_free(dbenv, str);
		b = NULL;
		goto retry;
	}

	/* Create the file if so requested. */
	if (tmp_create &&
	    (ret = __db_tmp_open(dbenv, tmp_oflags, str, fhpp)) != 0) {
		__os_free(dbenv, str);
		return (ret);
	}

	if (namep == NULL)
		__os_free(dbenv, str);
	else
		*namep = str;
	return (0);
}

/*
 * __db_home --
 *	Find the database home.
 *
 * PUBLIC:	int __db_home __P((DB_ENV *, const char *, u_int32_t));
 */
int
__db_home(dbenv, db_home, flags)
	DB_ENV *dbenv;
	const char *db_home;
	u_int32_t flags;
{
	int ret;
	const char *p;
	char path[MAXPATHLEN];

	/*
	 * Use db_home by default, this allows utilities to reasonably
	 * override the environment either explicitly or by using a -h
	 * option.  Otherwise, use the environment if it's permitted
	 * and initialized.
	 */
	if ((p = db_home) == NULL &&
	    (LF_ISSET(DB_USE_ENVIRON) ||
	    (LF_ISSET(DB_USE_ENVIRON_ROOT) && __os_isroot())) &&
	    (p = getenv("DB_HOME")) != NULL && p[0] == '\0') {
		__db_err(dbenv, "illegal DB_HOME environment variable");
		return (EINVAL);
	}
	if (p != NULL && (ret = __os_strdup(dbenv, p, &dbenv->db_home)) != 0)
		return (ret);

	/*
	 * Get the absolute pathname of the current directory.  We use this
	 * to build absolute pathnames when removing log files.
	 *
	 * XXX
	 * Can't trust getcwd(3) to set a valid errno, so don't try to display
	 * one unless we know it's good.  It's likely a permissions problem:
	 * use something bland and useless in the default return value, so we
	 * don't send somebody off in the wrong direction.
	 */
	__os_set_errno(0);
	if ((p = getcwd(path, sizeof(path))) == NULL) {
		if ((ret = __os_get_errno()) == 0) {
			__db_err(dbenv,
			    "no absolute path for the current directory");
			ret = EAGAIN;
		} else
			__db_err(dbenv,
			    "no absolute path for the current directory: %s",
			    db_strerror(ret));
		return (ret);
	}
	if (p != NULL && (ret = __os_strdup(dbenv, p, &dbenv->db_abshome)) != 0)
		return (ret);

	return (0);
}

#define	__DB_OVFL(v, max)						\
	if (v > max) {							\
		__v = v;						\
		__max = max;						\
		goto toobig;						\
	}

/*
 * __db_parse --
 *	Parse a single NAME VALUE pair.
 */
static int
__db_parse(dbenv, s)
	DB_ENV *dbenv;
	char *s;
{
	u_long __max, __v, v1, v2, v3;
	u_int32_t flags;
	char *name, *p, *value, v4;

	/*
	 * !!!
	 * The constant 40 is hard-coded into format arguments to sscanf
	 * below, it can't be changed here without changing it there, too.
	 * The additional bytes are for a trailing nul byte and because we
	 * are reading user input -- I don't want to risk any off-by-ones.
	 */
	char arg[40 + 5];

	/*
	 * Name/value pairs are parsed as two white-space separated strings.
	 * Leading and trailing white-space is trimmed from the value, but
	 * it may contain embedded white-space.  Note: we use the isspace(3)
	 * macro because it's more portable, but that means that you can use
	 * characters like form-feed to separate the strings.
	 */
	name = s;
	for (p = name; *p != '\0' && !isspace((int)*p); ++p)
		;
	if (*p == '\0' || p == name)
		goto illegal;
	*p = '\0';
	for (++p; isspace((int)*p); ++p)
		;
	if (*p == '\0')
		goto illegal;
	value = p;
	for (++p; *p != '\0'; ++p)
		;
	for (--p; isspace((int)*p); --p)
		;
	++p;
	if (p == value) {
illegal:	__db_err(dbenv, "mis-formatted name-value pair: %s", s);
		return (EINVAL);
	}
	*p = '\0';

	if (strcasecmp(name, "mutex_set_align") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__mutex_set_align(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "mutex_set_increment") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__mutex_set_increment(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "mutex_set_max") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__mutex_set_max(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "mutex_set_tas_spins") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__mutex_set_tas_spins(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "rep_set_config") == 0) {
		if (sscanf(value, "%40s %c", arg, &v4) != 1)
			goto badarg;

		if (strcasecmp(value, "rep_bulk") == 0)
			return (__rep_set_config(dbenv,
			    DB_REP_CONF_BULK, 1));
		if (strcasecmp(value, "rep_delayclient") == 0)
			return (__rep_set_config(dbenv,
			    DB_REP_CONF_DELAYCLIENT, 1));
		if (strcasecmp(value, "rep_noautoinit") == 0)
			return (__rep_set_config(dbenv,
			    DB_REP_CONF_NOAUTOINIT, 1));
		if (strcasecmp(value, "rep_nowait") == 0)
			return (__rep_set_config(dbenv, DB_REP_CONF_NOWAIT, 1));
		goto badarg;
	}

	if (strcasecmp(name, "set_cachesize") == 0) {
		if (sscanf(value, "%lu %lu %lu %c", &v1, &v2, &v3, &v4) != 3)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		__DB_OVFL(v2, UINT32_MAX);
		__DB_OVFL(v3, 10000);
		return (__memp_set_cachesize(
		    dbenv, (u_int32_t)v1, (u_int32_t)v2, (int)v3));
	}

	if (strcasecmp(name, "set_data_dir") == 0 ||
	    strcasecmp(name, "db_data_dir") == 0)	/* Compatibility. */
		return (__env_set_data_dir(dbenv, value));

							/* Undocumented. */
	if (strcasecmp(name, "set_intermediate_dir") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
#ifdef INT_MAX
		__DB_OVFL(v1, INT_MAX);
#endif
		return (__env_set_intermediate_dir(dbenv, (int)v1, 0));
	}

	if (strcasecmp(name, "set_flags") == 0) {
		if (sscanf(value, "%40s %c", arg, &v4) != 1)
			goto badarg;

		if (strcasecmp(value, "db_auto_commit") == 0)
			return (__env_set_flags(dbenv, DB_AUTO_COMMIT, 1));
		if (strcasecmp(value, "db_cdb_alldb") == 0)
			return (__env_set_flags(dbenv, DB_CDB_ALLDB, 1));
		if (strcasecmp(value, "db_direct_db") == 0)
			return (__env_set_flags(dbenv, DB_DIRECT_DB, 1));
		if (strcasecmp(value, "db_direct_log") == 0)
			return (__env_set_flags(dbenv, DB_DIRECT_LOG, 1));
		if (strcasecmp(value, "db_dsync_db") == 0)
			return (__env_set_flags(dbenv, DB_DSYNC_DB, 1));
		if (strcasecmp(value, "db_dsync_log") == 0)
			return (__env_set_flags(dbenv, DB_DSYNC_LOG, 1));
		if (strcasecmp(value, "db_log_autoremove") == 0)
			return (__env_set_flags(dbenv, DB_LOG_AUTOREMOVE, 1));
		if (strcasecmp(value, "db_log_inmemory") == 0)
			return (__env_set_flags(dbenv, DB_LOG_INMEMORY, 1));
		if (strcasecmp(value, "db_nolocking") == 0)
			return (__env_set_flags(dbenv, DB_NOLOCKING, 1));
		if (strcasecmp(value, "db_nommap") == 0)
			return (__env_set_flags(dbenv, DB_NOMMAP, 1));
		if (strcasecmp(value, "db_nopanic") == 0)
			return (__env_set_flags(dbenv, DB_NOPANIC, 1));
		if (strcasecmp(value, "db_overwrite") == 0)
			return (__env_set_flags(dbenv, DB_OVERWRITE, 1));
		if (strcasecmp(value, "db_region_init") == 0)
			return (__env_set_flags(dbenv, DB_REGION_INIT, 1));
		if (strcasecmp(value, "db_txn_nosync") == 0)
			return (__env_set_flags(dbenv, DB_TXN_NOSYNC, 1));
		if (strcasecmp(value, "db_txn_write_nosync") == 0)
			return (
			    __env_set_flags(dbenv, DB_TXN_WRITE_NOSYNC, 1));
		if (strcasecmp(value, "db_yieldcpu") == 0)
			return (__env_set_flags(dbenv, DB_YIELDCPU, 1));
		goto badarg;
	}

	if (strcasecmp(name, "set_lg_bsize") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__log_set_lg_bsize(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lg_filemode") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, INT_MAX);
		return (__log_set_lg_filemode(dbenv, (int)v1));
	}

	if (strcasecmp(name, "set_lg_max") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__log_set_lg_max(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lg_regionmax") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__log_set_lg_regionmax(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lg_dir") == 0 ||
	    strcasecmp(name, "db_log_dir") == 0)	/* Compatibility. */
		return (__log_set_lg_dir(dbenv, value));

	if (strcasecmp(name, "set_lk_detect") == 0) {
		if (sscanf(value, "%40s %c", arg, &v4) != 1)
			goto badarg;
		if (strcasecmp(value, "db_lock_default") == 0)
			flags = DB_LOCK_DEFAULT;
		else if (strcasecmp(value, "db_lock_expire") == 0)
			flags = DB_LOCK_EXPIRE;
		else if (strcasecmp(value, "db_lock_maxlocks") == 0)
			flags = DB_LOCK_MAXLOCKS;
		else if (strcasecmp(value, "db_lock_maxwrite") == 0)
			flags = DB_LOCK_MAXWRITE;
		else if (strcasecmp(value, "db_lock_minlocks") == 0)
			flags = DB_LOCK_MINLOCKS;
		else if (strcasecmp(value, "db_lock_minwrite") == 0)
			flags = DB_LOCK_MINWRITE;
		else if (strcasecmp(value, "db_lock_oldest") == 0)
			flags = DB_LOCK_OLDEST;
		else if (strcasecmp(value, "db_lock_random") == 0)
			flags = DB_LOCK_RANDOM;
		else if (strcasecmp(value, "db_lock_youngest") == 0)
			flags = DB_LOCK_YOUNGEST;
		else
			goto badarg;
		return (__lock_set_lk_detect(dbenv, flags));
	}

	if (strcasecmp(name, "set_lk_max") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__lock_set_lk_max(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lk_max_locks") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__lock_set_lk_max_locks(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lk_max_lockers") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__lock_set_lk_max_lockers(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lk_max_objects") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__lock_set_lk_max_objects(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_lock_timeout") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__lock_set_env_timeout(
		    dbenv, (u_int32_t)v1, DB_SET_LOCK_TIMEOUT));
	}

	if (strcasecmp(name, "set_mp_max_openfd") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, INT_MAX);
		return (__memp_set_mp_max_openfd(dbenv, (int)v1));
	}

	if (strcasecmp(name, "set_mp_max_write") == 0) {
		if (sscanf(value, "%lu %lu %c", &v1, &v2, &v4) != 2)
			goto badarg;
		__DB_OVFL(v1, INT_MAX);
		__DB_OVFL(v2, INT_MAX);
		return (__memp_set_mp_max_write(dbenv, (int)v1, (int)v2));
	}

	if (strcasecmp(name, "set_mp_mmapsize") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__memp_set_mp_mmapsize(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_region_init") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1 || v1 != 1)
			goto badarg;
		return (__env_set_flags(
		    dbenv, DB_REGION_INIT, v1 == 0 ? 0 : 1));
	}

	if (strcasecmp(name, "set_shm_key") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		return (__env_set_shm_key(dbenv, (long)v1));
	}

	/*
	 * The set_tas_spins method has been replaced by mutex_set_tas_spins.
	 * The set_tas_spins name remains for DB_CONFIG compatibility.
	 */
	if (strcasecmp(name, "set_tas_spins") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__mutex_set_tas_spins(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_tmp_dir") == 0 ||
	    strcasecmp(name, "db_tmp_dir") == 0)	/* Compatibility.*/
		return (__env_set_tmp_dir(dbenv, value));

	if (strcasecmp(name, "set_tx_max") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__txn_set_tx_max(dbenv, (u_int32_t)v1));
	}

	if (strcasecmp(name, "set_txn_timeout") == 0) {
		if (sscanf(value, "%lu %c", &v1, &v4) != 1)
			goto badarg;
		__DB_OVFL(v1, UINT32_MAX);
		return (__lock_set_env_timeout(
		    dbenv, (u_int32_t)v1, DB_SET_TXN_TIMEOUT));
	}

	if (strcasecmp(name, "set_verbose") == 0) {
		if (sscanf(value, "%40s %c", arg, &v4) != 1)
			goto badarg;

		else if (strcasecmp(value, "db_verb_deadlock") == 0)
			flags = DB_VERB_DEADLOCK;
		else if (strcasecmp(value, "db_verb_recovery") == 0)
			flags = DB_VERB_RECOVERY;
		else if (strcasecmp(value, "db_verb_register") == 0)
			flags = DB_VERB_REGISTER;
		else if (strcasecmp(value, "db_verb_replication") == 0)
			flags = DB_VERB_REPLICATION;
		else if (strcasecmp(value, "db_verb_waitsfor") == 0)
			flags = DB_VERB_WAITSFOR;
		else
			goto badarg;
		return (__env_set_verbose(dbenv, flags, 1));
	}

	__db_err(dbenv, "unrecognized name-value pair: %s", s);
	return (EINVAL);

badarg:	__db_err(dbenv, "incorrect arguments for name-value pair: %s", s);
	return (EINVAL);

toobig:	__db_err(dbenv,
	    "%s: %lu larger than maximum value %lu", s, __v, __max);
	return (EINVAL);
}

/*
 * __db_tmp_open --
 *	Create a temporary file.
 */
static int
__db_tmp_open(dbenv, tmp_oflags, path, fhpp)
	DB_ENV *dbenv;
	u_int32_t tmp_oflags;
	char *path;
	DB_FH **fhpp;
{
	pid_t pid;
	db_threadid_t tid;
	int filenum, i, isdir, ret;
	char *firstx, *trv;

	/*
	 * Check the target directory; if you have six X's and it doesn't
	 * exist, this runs for a *very* long time.
	 */
	if ((ret = __os_exists(path, &isdir)) != 0) {
		__db_err(dbenv, "%s: %s", path, db_strerror(ret));
		return (ret);
	}
	if (!isdir) {
		__db_err(dbenv, "%s: %s", path, db_strerror(EINVAL));
		return (EINVAL);
	}

	/* Build the path. */
	(void)strncat(path, PATH_SEPARATOR, 1);
	(void)strcat(path, DB_TRAIL);

	/* Replace the X's with the process ID (in decimal). */
	__os_id(dbenv, &pid, &tid);
	for (trv = path + strlen(path); *--trv == 'X'; pid /= 10)
		*trv = '0' + (u_char)(pid % 10);
	firstx = trv + 1;

	/* Loop, trying to open a file. */
	for (filenum = 1;; filenum++) {
		if ((ret = __os_open(dbenv, path,
		    tmp_oflags | DB_OSO_CREATE | DB_OSO_EXCL | DB_OSO_TEMP,
		    __db_omode(OWNER_RW), fhpp)) == 0)
			return (0);

		/*
		 * !!!:
		 * If we don't get an EEXIST error, then there's something
		 * seriously wrong.  Unfortunately, if the implementation
		 * doesn't return EEXIST for O_CREAT and O_EXCL regardless
		 * of other possible errors, we've lost.
		 */
		if (ret != EEXIST) {
			__db_err(dbenv,
			    "tmp_open: %s: %s", path, db_strerror(ret));
			return (ret);
		}

		/*
		 * Generate temporary file names in a backwards-compatible way.
		 * If pid == 12345, the result is:
		 *   <path>/DB12345 (tried above, the first time through).
		 *   <path>/DBa2345 ...  <path>/DBz2345
		 *   <path>/DBaa345 ...  <path>/DBaz345
		 *   <path>/DBba345, and so on.
		 *
		 * XXX
		 * This algorithm is O(n**2) -- that is, creating 100 temporary
		 * files requires 5,000 opens, creating 1000 files requires
		 * 500,000.  If applications open a lot of temporary files, we
		 * could improve performance by switching to timestamp-based
		 * file names.
		 */
		for (i = filenum, trv = firstx; i > 0; i = (i - 1) / 26)
			if (*trv++ == '\0')
				return (EINVAL);

		for (i = filenum; i > 0; i = (i - 1) / 26)
			*--trv = 'a' + ((i - 1) % 26);
	}
	/* NOTREACHED */
}
