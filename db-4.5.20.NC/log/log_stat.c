/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: log_stat.c,v 12.13 2006/08/24 14:46:12 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/db_page.h"
#include "dbinc/db_am.h"
#include "dbinc/log.h"

#ifdef HAVE_STATISTICS
static int __log_print_all __P((DB_ENV *, u_int32_t));
static int __log_print_stats __P((DB_ENV *, u_int32_t));
static int __log_stat __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));

/*
 * __log_stat_pp --
 *	DB_ENV->log_stat pre/post processing.
 *
 * PUBLIC: int __log_stat_pp __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));
 */
int
__log_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_LOG_STAT **statp;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->lg_handle, "DB_ENV->log_stat", DB_INIT_LOG);

	if ((ret = __db_fchk(dbenv,
	    "DB_ENV->log_stat", flags, DB_STAT_CLEAR)) != 0)
		return (ret);

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__log_stat(dbenv, statp, flags)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __log_stat --
 *	DB_ENV->log_stat.
 */
static int
__log_stat(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_LOG_STAT **statp;
	u_int32_t flags;
{
	DB_LOG *dblp;
	DB_LOG_STAT *stats;
	LOG *lp;
	int ret;

	*statp = NULL;

	dblp = dbenv->lg_handle;
	lp = dblp->reginfo.primary;

	if ((ret = __os_umalloc(dbenv, sizeof(DB_LOG_STAT), &stats)) != 0)
		return (ret);

	/* Copy out the global statistics. */
	LOG_SYSTEM_LOCK(dbenv);
	*stats = lp->stat;
	if (LF_ISSET(DB_STAT_CLEAR))
		memset(&lp->stat, 0, sizeof(lp->stat));

	stats->st_magic = lp->persist.magic;
	stats->st_version = lp->persist.version;
	stats->st_mode = lp->filemode;
	stats->st_lg_bsize = lp->buffer_size;
	stats->st_lg_size = lp->log_nsize;

	__mutex_set_wait_info(dbenv, lp->mtx_region,
	    &stats->st_region_wait, &stats->st_region_nowait);
	if (LF_ISSET(DB_STAT_CLEAR))
		__mutex_clear(dbenv, lp->mtx_region);
	stats->st_regsize = dblp->reginfo.rp->size;

	stats->st_cur_file = lp->lsn.file;
	stats->st_cur_offset = lp->lsn.offset;
	stats->st_disk_file = lp->s_lsn.file;
	stats->st_disk_offset = lp->s_lsn.offset;

	LOG_SYSTEM_UNLOCK(dbenv);

	*statp = stats;
	return (0);
}

/*
 * __log_stat_print_pp --
 *	DB_ENV->log_stat_print pre/post processing.
 *
 * PUBLIC: int __log_stat_print_pp __P((DB_ENV *, u_int32_t));
 */
int
__log_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_THREAD_INFO *ip;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->lg_handle, "DB_ENV->log_stat_print", DB_INIT_LOG);

	if ((ret = __db_fchk(dbenv, "DB_ENV->log_stat_print",
	    flags, DB_STAT_ALL | DB_STAT_CLEAR)) != 0)
		return (ret);

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__log_stat_print(dbenv, flags)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __log_stat_print --
 *	DB_ENV->log_stat_print method.
 *
 * PUBLIC: int __log_stat_print __P((DB_ENV *, u_int32_t));
 */
int
__log_stat_print(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	u_int32_t orig_flags;
	int ret;

	orig_flags = flags;
	LF_CLR(DB_STAT_CLEAR);
	if (flags == 0 || LF_ISSET(DB_STAT_ALL)) {
		ret = __log_print_stats(dbenv, orig_flags);
		if (flags == 0 || ret != 0)
			return (ret);
	}

	if (LF_ISSET(DB_STAT_ALL) &&
	    (ret = __log_print_all(dbenv, orig_flags)) != 0)
		return (ret);

	return (0);
}

/*
 * __log_print_stats --
 *	Display default log region statistics.
 */
static int
__log_print_stats(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	DB_LOG_STAT *sp;
	int ret;

	if ((ret = __log_stat(dbenv, &sp, flags)) != 0)
		return (ret);

	if (LF_ISSET(DB_STAT_ALL))
		__db_msg(dbenv, "Default logging region information:");
	STAT_HEX("Log magic number", sp->st_magic);
	STAT_ULONG("Log version number", sp->st_version);
	__db_dlbytes(dbenv, "Log record cache size",
	    (u_long)0, (u_long)0, (u_long)sp->st_lg_bsize);
	__db_msg(dbenv, "%#o\tLog file mode", sp->st_mode);
	if (sp->st_lg_size % MEGABYTE == 0)
		__db_msg(dbenv, "%luMb\tCurrent log file size",
		    (u_long)sp->st_lg_size / MEGABYTE);
	else if (sp->st_lg_size % 1024 == 0)
		__db_msg(dbenv, "%luKb\tCurrent log file size",
		    (u_long)sp->st_lg_size / 1024);
	else
		__db_msg(dbenv, "%lu\tCurrent log file size",
		    (u_long)sp->st_lg_size);
	__db_dl(dbenv, "Records entered into the log", (u_long)sp->st_record);
	__db_dlbytes(dbenv, "Log bytes written",
	    (u_long)0, (u_long)sp->st_w_mbytes, (u_long)sp->st_w_bytes);
	__db_dlbytes(dbenv, "Log bytes written since last checkpoint",
	    (u_long)0, (u_long)sp->st_wc_mbytes, (u_long)sp->st_wc_bytes);
	__db_dl(dbenv, "Total log file I/O writes", (u_long)sp->st_wcount);
	__db_dl(dbenv, "Total log file I/O writes due to overflow",
	    (u_long)sp->st_wcount_fill);
	__db_dl(dbenv, "Total log file flushes", (u_long)sp->st_scount);
	__db_dl(dbenv, "Total log file I/O reads", (u_long)sp->st_rcount);
	STAT_ULONG("Current log file number", sp->st_cur_file);
	STAT_ULONG("Current log file offset", sp->st_cur_offset);
	STAT_ULONG("On-disk log file number", sp->st_disk_file);
	STAT_ULONG("On-disk log file offset", sp->st_disk_offset);

	__db_dl(dbenv,
	    "Maximum commits in a log flush", (u_long)sp->st_maxcommitperflush);
	__db_dl(dbenv,
	    "Minimum commits in a log flush", (u_long)sp->st_mincommitperflush);

	__db_dlbytes(dbenv, "Log region size",
	    (u_long)0, (u_long)0, (u_long)sp->st_regsize);
	__db_dl_pct(dbenv,
	    "The number of region locks that required waiting",
	    (u_long)sp->st_region_wait, DB_PCT(sp->st_region_wait,
	    sp->st_region_wait + sp->st_region_nowait), NULL);

	__os_ufree(dbenv, sp);

	return (0);
}

/*
 * __log_print_all --
 *	Display debugging log region statistics.
 */
static int
__log_print_all(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	static const FN fn[] = {
		{ DBLOG_RECOVER,	"DBLOG_RECOVER" },
		{ DBLOG_FORCE_OPEN,	"DBLOG_FORCE_OPEN" },
		{ 0,			NULL }
	};
	DB_LOG *dblp;
	LOG *lp;

	dblp = dbenv->lg_handle;
	lp = (LOG *)dblp->reginfo.primary;

	LOG_SYSTEM_LOCK(dbenv);

	__db_print_reginfo(dbenv, &dblp->reginfo, "Log");

	__db_msg(dbenv, "%s", DB_GLOBAL(db_line));
	__db_msg(dbenv, "DB_LOG handle information:");
	__mutex_print_debug_single(
	    dbenv, "DB_LOG handle mutex", dblp->mtx_dbreg, flags);
	STAT_ULONG("Log file name", dblp->lfname);
	__db_print_fh(dbenv, "Log file handle", dblp->lfhp, flags);
	__db_prflags(dbenv, NULL, dblp->flags, fn, NULL, "\tFlags");

	__db_msg(dbenv, "%s", DB_GLOBAL(db_line));
	__db_msg(dbenv, "LOG handle information:");
	__mutex_print_debug_single(
	    dbenv, "LOG region mutex", lp->mtx_region, flags);
	__mutex_print_debug_single(
	    dbenv, "File name list mutex", lp->mtx_filelist, flags);

	STAT_HEX("persist.magic", lp->persist.magic);
	STAT_ULONG("persist.version", lp->persist.version);
	__db_dlbytes(dbenv,
	    "persist.log_size", (u_long)0, (u_long)0, lp->persist.log_size);
	STAT_FMT("log file permissions mode", "%#lo", u_long, lp->filemode);
	STAT_LSN("current file offset LSN", &lp->lsn);
	STAT_LSN("first buffer byte LSN", &lp->lsn);
	STAT_ULONG("current buffer offset", lp->b_off);
	STAT_ULONG("current file write offset", lp->w_off);
	STAT_ULONG("length of last record", lp->len);
	STAT_LONG("log flush in progress", lp->in_flush);
	__mutex_print_debug_single(
	    dbenv, "Log flush mutex", lp->mtx_flush, flags);

	STAT_LSN("last sync LSN", &lp->s_lsn);

	/*
	 * Don't display the replication fields here, they're displayed as part
	 * of the replication statistics.
	 */

	STAT_LSN("cached checkpoint LSN", &lp->cached_ckp_lsn);

	__db_dlbytes(dbenv,
	    "log buffer size", (u_long)0, (u_long)0, lp->buffer_size);
	__db_dlbytes(dbenv,
	    "log file size", (u_long)0, (u_long)0, lp->log_size);
	__db_dlbytes(dbenv,
	    "next log file size", (u_long)0, (u_long)0, lp->log_nsize);

	STAT_ULONG("transactions waiting to commit", lp->ncommit);
	STAT_LSN("LSN of first commit", &lp->t_lsn);

	LOG_SYSTEM_UNLOCK(dbenv);

	return (0);
}

#else /* !HAVE_STATISTICS */

int
__log_stat_pp(dbenv, statp, flags)
	DB_ENV *dbenv;
	DB_LOG_STAT **statp;
	u_int32_t flags;
{
	COMPQUIET(statp, NULL);
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv));
}

int
__log_stat_print_pp(dbenv, flags)
	DB_ENV *dbenv;
	u_int32_t flags;
{
	COMPQUIET(flags, 0);

	return (__db_stat_not_built(dbenv));
}
#endif
