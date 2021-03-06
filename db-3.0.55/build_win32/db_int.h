/* DO NOT EDIT: automatically built by dist/distrib. */
/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999
 *	Sleepycat Software.  All rights reserved.
 *
 *	@(#)db_int.src	11.6 (Sleepycat) 10/1/99
 */

#ifndef _DB_INTERNAL_H_
#define	_DB_INTERNAL_H_

/*******************************************************
 * General includes.
 *******************************************************/
#include "db.h"

#ifndef NO_SYSTEM_INCLUDES
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#endif

#include "queue.h"
#include "shqueue.h"

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
 * Aligning items to particular sizes or in pages or memory.  ALIGNP is a
 * separate macro, as we've had to cast the pointer to different integral
 * types on different architectures.
 *
 * We cast pointers into unsigned longs when manipulating them because C89
 * guarantees that u_long is the largest available integral type and further,
 * to never generate overflows.  However, neither C89 or C9X  requires that
 * any integer type be large enough to hold a pointer, although C9X created
 * the intptr_t type, which is guaranteed to hold a pointer but may or may
 * not exist.  At some point in the future, we should test for intptr_t and
 * use it where available.
 */
#undef	ALIGNTYPE
#define	ALIGNTYPE		u_long
#undef	ALIGNP
#define	ALIGNP(value, bound)	ALIGN((ALIGNTYPE)value, bound)
#undef	ALIGN
#define	ALIGN(value, bound)	(((value) + (bound) - 1) & ~(((u_int)bound) - 1))

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
#define SSZ(name, field)	((int)&(((name *)0)->field))

#undef	SSZA
#define SSZA(name, field)	((int)&(((name *)0)->field[0]))

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

/*
 * Purify and similar run-time tools complain about unitialized reads/writes
 * for structure fields whose only purpose is padding.
 */
#define	UMRW(v)		(v) = 0

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
#define	ZERO_LSN(LSN) {							\
	(LSN).file = 0;							\
	(LSN).offset = 0;						\
}

/* Return 1 if LSN is a 'zero' lsn, otherwise return 0. */
#define	IS_ZERO_LSN(LSN)	((LSN).file == 0)

/* Test if we need to log a change. */
#define	DB_LOGGING(dbc)							\
	(F_ISSET((dbc)->dbp->dbenv, DB_ENV_LOGGING) &&			\
	    !F_ISSET(dbc, DBC_RECOVER))

/*******************************************************
 * Txn.
 *******************************************************/
#define	DB_NONBLOCK(C)	((C)->txn != NULL && F_ISSET((C)->txn, TXN_NOWAIT))

/*******************************************************
 * Global variables.
 *******************************************************/
/*
 * !!!
 * Initialized in env/env_method.c, don't change this without changing that.
 */
typedef struct __db_globals {
	u_int32_t db_mutexlocks;	/* db_set_mutexlocks */
	u_int32_t db_pageyield;		/* db_set_pageyield */
	u_int32_t db_panic;		/* db_set_panic */
	u_int32_t db_region_init;	/* db_set_region_init */
	u_int32_t db_tas_spins;		/* db_set_tas_spins */
					/* XA: list of opened environments. */
	TAILQ_HEAD(__db_envq, __db_env) db_envq;
} DB_GLOBALS;

extern	DB_GLOBALS	__db_global_values;
#define	DB_GLOBAL(v)	__db_global_values.v

/* Forward structure declarations. */
struct __db_reginfo_t;	typedef struct __db_reginfo_t REGINFO;
struct __mutex_t;	typedef struct __mutex_t MUTEX;

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
