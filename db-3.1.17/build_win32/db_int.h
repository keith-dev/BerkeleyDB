/* DO NOT EDIT: automatically built by dist/distrib. */
/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 *
 *	$Id: db_int.src,v 11.31 2000/05/18 19:26:07 bostic Exp $
 */

#ifndef _DB_INTERNAL_H_
#define	_DB_INTERNAL_H_

/*******************************************************
 * General includes.
 *******************************************************/
#include "db.h"

#ifndef NO_SYSTEM_INCLUDES
#if defined(__STDC__) || defined(__cplusplus)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#endif

/*
 * XXX
 * We need to #undef the following LIST_XXX macros because VxWorks copied
 * some of them into UnixLib.h -- not all of them though, so we can't use
 * their versions.
 */
#undef LIST_INIT
#undef LIST_INSERT_HEAD
#undef LIST_REMOVE
#include "queue.h"
#include "shqueue.h"

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************************
 * General purpose constants and macros.
 *******************************************************/
#define	UINT16_T_MAX	    0xffff	/* Maximum 16 bit unsigned. */
#define	UINT32_T_MAX	0xffffffff	/* Maximum 32 bit unsigned. */

#define	MEGABYTE	1048576
#define	GIGABYTE	1073741824

#define	MS_PER_SEC	1000		/* Milliseconds in a second. */
#define	USEC_PER_MS	1000		/* Microseconds in a millisecond. */

#define	DB_MIN_PGSIZE	0x000200	/* Minimum page size (512). */
#define	DB_MAX_PGSIZE	0x010000	/* Maximum page size (65536). */

#define	RECNO_OOB	0		/* Illegal record number. */

/*
 * If we are unable to determine the underlying filesystem block size, use
 * 8K on the grounds that most OS's use less than 8K for a VM page size.
 */
#define	DB_DEF_IOSIZE	(8 * 1024)

/*
 * Aligning items to particular sizes or in pages or memory.
 *
 * db_align_t --
 * Largest integral type, used to align structures in memory.  We don't store
 * floating point types in structures, so integral types should be sufficient
 * (and we don't have to worry about systems that store floats in other than
 * power-of-2 numbers of bytes).  Additionally this fixes compiler that rewrite
 * structure assignments and ANSI C memcpy calls to be in-line instructions
 * that happen to require alignment.  Note: this alignment isn't sufficient for
 * mutexes, which depend on things like cache line alignment.  Mutex alignment
 * is handled separately, in mutex.h.
 *
 * db_alignp_t --
 * Integral type that's the same size as a pointer.  There are places where
 * DB modifies pointers by discarding the bottom bits to guarantee alignment.
 * We can't use db_align_t, it may be larger than the pointer, and compilers
 * get upset about that.  So far we haven't run on any machine where there
 * isn't an integral type the same size as a pointer -- here's hoping.
 */
typedef unsigned long db_align_t;
typedef unsigned long db_alignp_t;

/* Align an integer to a specific boundary. */
#undef	ALIGN
#define	ALIGN(value, bound) \
    (((value) + (bound) - 1) & ~(((u_int)bound) - 1))

/* Align a pointer to a specific boundary. */
#undef	ALIGNP
#define	ALIGNP(value, bound)	ALIGN((db_alignp_t)value, bound)

/*
 * There are several on-page structures that are declared to have a number of
 * fields followed by a variable length array of items.  The structure size
 * without including the variable length array or the address of the first of
 * those elements can be found using SSZ.
 *
 * This macro can also be used to find the offset of a structure element in a
 * structure.  This is used in various places to copy structure elements from
 * unaligned memory references, e.g., pointers into a packed page.
 *
 * There are two versions because compilers object if you take the address of
 * an array.
 */
#undef	SSZ
#define	SSZ(name, field)	((int)&(((name *)0)->field))

#undef	SSZA
#define	SSZA(name, field)	((int)&(((name *)0)->field[0]))

/*
 * Print an address as a u_long (a u_long is the largest type we can print
 * portably).  Most 64-bit systems have made longs 64-bits, so this should
 * work.
 */
#define	P_TO_ULONG(p)	((u_long)(db_alignp_t)(p))

/* Structure used to print flag values. */
typedef struct __fn {
	u_int32_t mask;			/* Flag value. */
	const char *name;		/* Flag name. */
} FN;

/* Set, clear and test flags. */
#define	FLD_CLR(fld, f)		(fld) &= ~(f)
#define	FLD_ISSET(fld, f)	((fld) & (f))
#define	FLD_SET(fld, f)		(fld) |= (f)
#define	F_CLR(p, f)		(p)->flags &= ~(f)
#define	F_ISSET(p, f)		((p)->flags & (f))
#define	F_SET(p, f)		(p)->flags |= (f)
#define	LF_CLR(f)		(flags &= ~(f))
#define	LF_ISSET(f)		(flags & (f))
#define	LF_SET(f)		(flags |= (f))

/* Display separator string. */
#undef	DB_LINE
#define	DB_LINE "=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-="

/* Unused, or not-used-yet variable.  "Shut that bloody compiler up!" */
#define	COMPQUIET(n, v)	(n) = (v)

/*******************************************************
 * Files.
 *******************************************************/
 /*
  * We use 1024 as the maximum path length.  It's too hard to figure out what
  * the real path length is, as it was traditionally stored in <sys/param.h>,
  * and that file isn't always available.
  */
#undef	MAXPATHLEN
#define	MAXPATHLEN	1024

#define	PATH_DOT	"."	/* Current working directory. */
#define	PATH_SEPARATOR	"\\/"	/* Path separator character. */

/*
 * Flags understood by __os_open.
 */
#define	DB_OSO_CREATE	0x001		/* POSIX: O_CREAT */
#define	DB_OSO_EXCL	0x002		/* POSIX: O_EXCL */
#define	DB_OSO_LOG	0x004		/* Opening a log file. */
#define	DB_OSO_RDONLY	0x008		/* POSIX: O_RDONLY */
#define	DB_OSO_SEQ	0x010		/* Expected sequential access. */
#define	DB_OSO_TEMP	0x020		/* Remove after last close. */
#define	DB_OSO_TRUNC	0x040		/* POSIX: O_TRUNC */

/*
 * Seek options understood by __os_seek.
 */
typedef enum {
	DB_OS_SEEK_CUR,			/* POSIX: SEEK_CUR */
	DB_OS_SEEK_END,			/* POSIX: SEEK_END */
	DB_OS_SEEK_SET			/* POSIX: SEEK_SET */
} DB_OS_SEEK;

/*******************************************************
 * Environment.
 *******************************************************/
/* Type passed to __db_appname(). */
typedef enum {
	DB_APP_NONE=0,			/* No type (region). */
	DB_APP_DATA,			/* Data file. */
	DB_APP_LOG,			/* Log file. */
	DB_APP_TMP			/* Temporary file. */
} APPNAME;

/*
 * CDB_LOCKING	CDB product locking.
 * LOCKING_ON	Locking has been configured.
 * LOGGING_ON	Logging has been configured.
 * MPOOL_ON	Memory pool has been configured.
 * TXN_ON	Transactions have been configured.
 */
#define	CDB_LOCKING(dbenv)	F_ISSET(dbenv, DB_ENV_CDB)
#define	LOCKING_ON(dbenv)	((dbenv)->lk_handle != NULL)
#define	LOGGING_ON(dbenv)	((dbenv)->lg_handle != NULL)
#define	MPOOL_ON(dbenv)		((dbenv)->mp_handle != NULL)
#define	TXN_ON(dbenv)		((dbenv)->tx_handle != NULL)

/*
 * STD_LOCKING	Standard locking, that is, locking was configured and CDB
 *		was not.  We do not do locking in off-page duplicate trees,
 *		so we check for that in the cursor first.
 */
#define	STD_LOCKING(dbc)						\
	(!F_ISSET(dbc, DBC_OPD) &&					\
	    !CDB_LOCKING((dbc)->dbp->dbenv) && LOCKING_ON((dbc)->dbp->dbenv))

/*
 * IS_RECOVERING The system is running recovery.
 */
#define	IS_RECOVERING(dbenv)						\
	(LOGGING_ON(dbenv) &&						\
	    F_ISSET((DB_LOG *)(dbenv)->lg_handle, DBLOG_RECOVER))

/* Most initialization methods cannot be called after open is called. */
#define	ENV_ILLEGAL_AFTER_OPEN(dbenv, name)				\
	if (F_ISSET((dbenv), DB_ENV_OPEN_CALLED))			\
		return (__db_mi_open(dbenv, name, 1));

/* We're not actually user hostile, honest. */
#define	ENV_REQUIRES_CONFIG(dbenv, handle, subsystem)			\
	if (handle == NULL)						\
		return (__db_env_config(dbenv, subsystem));

/*******************************************************
 * Database Access Methods.
 *******************************************************/
/*
 * DB_IS_THREADED --
 *	The database handle is free-threaded (was opened with DB_THREAD).
 */
#define	DB_IS_THREADED(dbp)						\
	((dbp)->mutexp != NULL)

/* Initialization methods are often illegal before/after open is called. */
#define	DB_ILLEGAL_AFTER_OPEN(dbp, name)				\
	if (F_ISSET((dbp), DB_OPEN_CALLED))				\
		return (__db_mi_open(dbp->dbenv, name, 1));
#define	DB_ILLEGAL_BEFORE_OPEN(dbp, name)				\
	if (!F_ISSET((dbp), DB_OPEN_CALLED))				\
		return (__db_mi_open(dbp->dbenv, name, 0));
/* Some initialization methods are illegal if environment isn't local. */
#define	DB_ILLEGAL_IN_ENV(dbp, name)					\
	if (!F_ISSET(dbp->dbenv, DB_ENV_DBLOCAL))			\
		return (__db_mi_env(dbp->dbenv, name));
#define	DB_ILLEGAL_METHOD(dbp, flags) {					\
	int __ret;							\
	if ((__ret = __dbh_am_chk(dbp, flags)) != 0)			\
		return (__ret);						\
}

/*
 * Common DBC->internal fields.  Each access method adds additional fields
 * to this list, but the initial fields are common.
 */
#define	__DBC_INTERNAL							\
	DBC	 *opd;			/* Off-page duplicate cursor. */\
									\
	void	 *page;			/* Referenced page. */		\
	db_pgno_t root;			/* Tree root. */		\
	db_pgno_t pgno;			/* Referenced page number. */	\
	db_indx_t indx;			/* Referenced key item index. */\
									\
	DB_LOCK		lock;		/* Cursor lock. */		\
	db_lockmode_t	lock_mode;	/* Lock mode. */

struct __dbc_internal {
	__DBC_INTERNAL
};

/*******************************************************
 * Mpool.
 *******************************************************/
/*
 * File types for DB access methods.  Negative numbers are reserved to DB.
 */
#define	DB_FTYPE_SET		-1	/* Call pgin/pgout functions. */
#define	DB_FTYPE_NOTSET		 0	/* Don't call... */

/* Structure used as the DB pgin/pgout pgcookie. */
typedef struct __dbpginfo {
	size_t	db_pagesize;		/* Underlying page size. */
	int	needswap;		/* If swapping required. */
} DB_PGINFO;

/*******************************************************
 * Log.
 *******************************************************/
/* Initialize an LSN to 'zero'. */
#define	ZERO_LSN(LSN) do {						\
	(LSN).file = 0;							\
	(LSN).offset = 0;						\
} while (0)

/* Return 1 if LSN is a 'zero' lsn, otherwise return 0. */
#define	IS_ZERO_LSN(LSN)	((LSN).file == 0)

/* Test if we need to log a change. */
#define	DB_LOGGING(dbc)							\
	(LOGGING_ON((dbc)->dbp->dbenv) && !F_ISSET(dbc, DBC_RECOVER))

/* Internal flag for use with internal __log_unregister. */
#define	DB_LOGONLY	0x01
/*******************************************************
 * Txn.
 *******************************************************/
#define	DB_NONBLOCK(C)	((C)->txn != NULL && F_ISSET((C)->txn, TXN_NOWAIT))

/*******************************************************
 * Global variables.
 *******************************************************/
#ifdef HAVE_VXWORKS
#include "semLib.h"
#endif

/*
 * DB global variables.  Done in a single structure to minimize the name-space
 * pollution.
 */
typedef struct __db_globals {
	u_int32_t db_mutexlocks;	/* db_set_mutexlocks */
	u_int32_t db_pageyield;		/* db_set_pageyield */
	u_int32_t db_panic;		/* db_set_panic */
	u_int32_t db_region_init;	/* db_set_region_init */
	u_int32_t db_tas_spins;		/* db_set_tas_spins */
#ifdef HAVE_VXWORKS
	u_int32_t db_global_init;	/* VxWorks: inited */
	SEM_ID db_global_lock;		/* VxWorks: global semaphore */
#endif
					/* XA: list of opened environments. */
	TAILQ_HEAD(__db_envq, __db_env) db_envq;
} DB_GLOBALS;

#ifdef DB_INITIALIZE_DB_GLOBALS
DB_GLOBALS __db_global_values = {
	1,					/* db_set_mutexlocks */
	0,					/* db_set_pageyield */
	1,					/* db_set_panic */
	0,					/* db_set_region_init */
	0,					/* db_set_tas_spins */
#ifdef HAVE_VXWORKS
	0,					/* db_global_init */
	NULL,					/* db_global_lock */
#endif
						/* XA environment queue */
	{NULL, &__db_global_values.db_envq.tqh_first}
};
#else
extern	DB_GLOBALS	__db_global_values;
#endif
#define	DB_GLOBAL(v)	__db_global_values.v

/* Forward structure declarations. */
struct __db_reginfo_t;	typedef struct __db_reginfo_t REGINFO;
struct __mutex_t;	typedef struct __mutex_t MUTEX;
struct __vrfy_childinfo; typedef struct __vrfy_childinfo VRFY_CHILDINFO;
struct __vrfy_dbinfo;   typedef struct __vrfy_dbinfo VRFY_DBINFO;
struct __vrfy_pageinfo; typedef struct __vrfy_pageinfo VRFY_PAGEINFO;

#if defined(__cplusplus)
}
#endif

/*******************************************************
 * More general includes.
 *******************************************************/
#include "debug.h"
#include "mutex.h"
#include "mutex_ext.h"
#include "region.h"
#include "env_ext.h"
#include "os.h"
#include "os_ext.h"
#include "common_ext.h"

#endif /* !_DB_INTERNAL_H_ */
