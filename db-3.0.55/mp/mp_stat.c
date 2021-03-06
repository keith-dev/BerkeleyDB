/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999
 *	Sleepycat Software.  All rights reserved.
 */
#include "db_config.h"

#ifndef lint
static const char sccsid[] = "@(#)mp_stat.c	11.4 (Sleepycat) 9/18/99";
#endif /* not lint */

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#endif

#include "db_int.h"
#include "db_page.h"
#include "db_shash.h"
#include "db_am.h"
#include "mp.h"

static void __memp_dumpcache
	        __P((DB_MPOOL *, REGINFO *, size_t *, FILE *, u_int32_t));
static void __memp_pbh __P((DB_MPOOL *, BH *, size_t *, FILE *));

/*
 * memp_stat --
 *	Display MPOOL statistics.
 */
int
memp_stat(dbenv, gspp, fspp, db_malloc)
	DB_ENV *dbenv;
	DB_MPOOL_STAT **gspp;
	DB_MPOOL_FSTAT ***fspp;
	void *(*db_malloc) __P((size_t));
{
	DB_MPOOL *dbmp;
	DB_MPOOL_FSTAT **tfsp;
	DB_MPOOL_STAT *sp;
	MCACHE *mc;
	MPOOL *mp;
	MPOOLFILE *mfp;
	size_t len, nlen;
	u_int32_t i;
	int ret;
	char *name;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv, dbenv->mp_handle, DB_INIT_MPOOL);

	dbmp = dbenv->mp_handle;
	sp = NULL;

	/* Global statistics. */
	mp = dbmp->reginfo.primary;
	if (gspp != NULL) {
		*gspp = NULL;

		if ((ret = __os_calloc(1, sizeof(**gspp), gspp)) != 0)
			return (ret);
		sp = *gspp;

		/*
		 * Initialization and information that is not maintained on
		 * a per-cache basis.
		 */
		sp->st_hash_longest = 0;
		sp->st_region_wait = dbmp->reginfo.rp->mutex.mutex_set_wait;
		sp->st_region_nowait = dbmp->reginfo.rp->mutex.mutex_set_nowait;
		sp->st_regsize = dbmp->reginfo.rp->size;
		sp->st_gbytes = dbenv->mp_gbytes;
		sp->st_bytes = dbenv->mp_bytes;

		R_LOCK(dbenv, &dbmp->reginfo);

		/* Walk the cache list and accumulate the global information. */
		for (i = 0; i < mp->nc_reg; ++i) {
			mc = dbmp->c_reginfo[i].primary;
			sp->st_cache_hit += mc->stat.st_cache_hit;
			sp->st_cache_miss += mc->stat.st_cache_miss;
			sp->st_map += mc->stat.st_map;
			sp->st_page_create += mc->stat.st_page_create;
			sp->st_page_in += mc->stat.st_page_in;
			sp->st_page_out += mc->stat.st_page_out;
			sp->st_ro_evict += mc->stat.st_ro_evict;
			sp->st_rw_evict += mc->stat.st_rw_evict;
			sp->st_hash_buckets += mc->stat.st_hash_buckets;
			sp->st_hash_searches += mc->stat.st_hash_searches;
			if (mc->stat.st_hash_longest > sp->st_hash_longest)
				sp->st_hash_longest = mc->stat.st_hash_longest;
			sp->st_hash_examined += mc->stat.st_hash_examined;
			sp->st_page_clean += mc->stat.st_page_clean;
			sp->st_page_dirty += mc->stat.st_page_dirty;
			sp->st_page_trickle += mc->stat.st_page_trickle;
			sp->st_region_wait += mc->stat.st_region_wait;
			sp->st_region_nowait += mc->stat.st_region_nowait;
		}

		R_UNLOCK(dbenv, &dbmp->reginfo);
	}

	/* Per-file statistics. */
	if (fspp != NULL) {
		*fspp = NULL;

		R_LOCK(dbenv, &dbmp->reginfo);

		/* Count the MPOOLFILE structures. */
		for (len = 0,
		    mfp = SH_TAILQ_FIRST(&mp->mpfq, __mpoolfile);
		    mfp != NULL;
		    ++len, mfp = SH_TAILQ_NEXT(mfp, q, __mpoolfile))
			;

		R_UNLOCK(dbenv, &dbmp->reginfo);

		if (len == 0)
			return (0);

		/* Allocate space for the pointers. */
		len = (len + 1) * sizeof(DB_MPOOL_FSTAT *);
		if ((ret = __os_malloc(len, db_malloc, fspp)) != 0)
			return (ret);

		R_LOCK(dbenv, &dbmp->reginfo);

		/* Build each individual entry. */
		for (tfsp = *fspp,
		    mfp = SH_TAILQ_FIRST(&mp->mpfq, __mpoolfile);
		    mfp != NULL;
		    ++tfsp, mfp = SH_TAILQ_NEXT(mfp, q, __mpoolfile)) {
			name = __memp_fns(dbmp, mfp);
			nlen = strlen(name);
			len = sizeof(DB_MPOOL_FSTAT) + nlen + 1;
			if ((ret = __os_malloc(len, db_malloc, tfsp)) != 0)
				return (ret);
			**tfsp = mfp->stat;
			(*tfsp)->file_name = (char *)
			    (u_int8_t *)*tfsp + sizeof(DB_MPOOL_FSTAT);
			memcpy((*tfsp)->file_name, name, nlen + 1);

			/*
			 * We have duplicate statistics fields in the cache
			 * and per-file structures.  The counters are only
			 * incremented in the per-file structures, though.
			 * The intent is that if we ever flush files from
			 * the pool we can save their last known totals in
			 * the cache structure.
			 */
			if (sp != NULL) {
				sp->st_cache_hit += mfp->stat.st_cache_hit;
				sp->st_cache_miss += mfp->stat.st_cache_miss;
				sp->st_map += mfp->stat.st_map;
				sp->st_page_create += mfp->stat.st_page_create;
				sp->st_page_in += mfp->stat.st_page_in;
				sp->st_page_out += mfp->stat.st_page_out;
			}
		}
		*tfsp = NULL;

		R_UNLOCK(dbenv, &dbmp->reginfo);
	}
	return (0);
}

#define	FMAP_ENTRIES	200			/* Files we map. */

#define	MPOOL_DUMP_HASH	0x01			/* Debug hash chains. */
#define	MPOOL_DUMP_LRU	0x02			/* Debug LRU chains. */
#define	MPOOL_DUMP_MEM	0x04			/* Debug region memory. */
#define	MPOOL_DUMP_ALL	0x07			/* Debug all. */

/*
 * __memp_dump_region --
 *	Display MPOOL structures.
 *
 * PUBLIC: void __memp_dump_region __P((DB_ENV *, char *, FILE *));
 */
void
__memp_dump_region(dbenv, area, fp)
	DB_ENV *dbenv;
	char *area;
	FILE *fp;
{
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	MPOOL *mp;
	MPOOLFILE *mfp;
	size_t fmap[FMAP_ENTRIES + 1];
	u_int32_t i, flags;
	int cnt;
	u_int8_t *p;

	dbmp = dbenv->mp_handle;

	/* Make it easy to call from the debugger. */
	if (fp == NULL)
		fp = stderr;

	for (flags = 0; *area != '\0'; ++area)
		switch (*area) {
		case 'A':
			LF_SET(MPOOL_DUMP_ALL);
			break;
		case 'h':
			LF_SET(MPOOL_DUMP_HASH);
			break;
		case 'l':
			LF_SET(MPOOL_DUMP_LRU);
			break;
		case 'm':
			LF_SET(MPOOL_DUMP_MEM);
			break;
		}

	R_LOCK(dbenv, &dbmp->reginfo);

	mp = dbmp->reginfo.primary;

	/* Display MPOOL structures. */
	(void)fprintf(fp, "%s\nPool (region addr 0x%lx)\n",
	    DB_LINE, (u_long)dbmp->reginfo.addr);

	/* Display the MPOOLFILE structures. */
	cnt = 0;
	for (mfp = SH_TAILQ_FIRST(&mp->mpfq, __mpoolfile);
	    mfp != NULL; mfp = SH_TAILQ_NEXT(mfp, q, __mpoolfile), ++cnt) {
		(void)fprintf(fp, "File #%d: %s: type %ld, %s\n\t [UID: ",
		    cnt + 1, __memp_fns(dbmp, mfp), (long)mfp->ftype,
		    F_ISSET(mfp, MP_CAN_MMAP) ? "mmap" : "read/write");
		p = R_ADDR(&dbmp->reginfo, mfp->fileid_off);
		for (i = 0; i < DB_FILE_ID_LEN; ++i) {
			(void)fprintf(fp, "%x", *p++);
			if (i < DB_FILE_ID_LEN - 1)
				(void)fprintf(fp, " ");
		}
		(void)fprintf(fp, "]\n");
		if (cnt < FMAP_ENTRIES)
			fmap[cnt] = R_OFFSET(&dbmp->reginfo, mfp);
	}

	for (dbmfp = TAILQ_FIRST(&dbmp->dbmfq);
	    dbmfp != NULL; dbmfp = TAILQ_NEXT(dbmfp, q), ++cnt) {
		(void)fprintf(fp, "File #%d: %s: per-process, %s\n",
		    cnt + 1, __memp_fn(dbmfp),
		    F_ISSET(dbmfp, MP_READONLY) ? "readonly" : "read/write");
		    if (cnt < FMAP_ENTRIES)
			fmap[cnt] = R_OFFSET(&dbmp->reginfo, mfp);
	}
	if (cnt < FMAP_ENTRIES)
		fmap[cnt] = INVALID_ROFF;
	else
		fmap[FMAP_ENTRIES] = INVALID_ROFF;

	/* Dump each cache. */
	for (i = 0; i < mp->nc_reg; ++i) {
		(void)fprintf(fp, "%s\nCache #%d:\n", DB_LINE, i + 1);
		__memp_dumpcache(dbmp, &dbmp->c_reginfo[i], fmap, fp, flags);
	}

	if (LF_ISSET(MPOOL_DUMP_MEM))
		__db_shalloc_dump(dbmp->reginfo.addr, fp);

	R_UNLOCK(dbenv, &dbmp->reginfo);

	/* Flush in case we're debugging. */
	(void)fflush(fp);
}

/*
 * __memp_dumpcache --
 *	Display statistics for a cache.
 */
static void
__memp_dumpcache(dbmp, reginfo, fmap, fp, flags)
	DB_MPOOL *dbmp;
	REGINFO *reginfo;
	size_t *fmap;
	FILE *fp;
	u_int32_t flags;
{
	BH *bhp;
	DB_HASHTAB *dbht;
	MCACHE *mc;
	int bucket;

	mc = reginfo->primary;

	/* Display the hash table list of BH's. */
	if (LF_ISSET(MPOOL_DUMP_HASH)) {
		(void)fprintf(fp,
	    "%s\nBH hash table (%lu hash slots)\npageno, file, ref, address\n",
		    DB_LINE, (u_long)mc->htab_buckets);
		for (dbht = R_ADDR(reginfo, mc->htab),
		    bucket = 0; bucket < mc->htab_buckets; ++dbht, ++bucket) {
			if (SH_TAILQ_FIRST(dbht, __bh) != NULL)
				(void)fprintf(fp, "%lu:\n", (u_long)bucket);
			for (bhp = SH_TAILQ_FIRST(dbht, __bh);
			    bhp != NULL; bhp = SH_TAILQ_NEXT(bhp, hq, __bh))
				__memp_pbh(dbmp, bhp, fmap, fp);
		}
	}

	/* Display the LRU list of BH's. */
	if (LF_ISSET(MPOOL_DUMP_LRU)) {
		(void)fprintf(fp, "%s\nBH LRU list\n", DB_LINE);
		(void)fprintf(fp, "pageno, file, ref, address\n");
		for (bhp = SH_TAILQ_FIRST(&mc->bhq, __bh);
		    bhp != NULL; bhp = SH_TAILQ_NEXT(bhp, q, __bh))
			__memp_pbh(dbmp, bhp, fmap, fp);
	}
}

/*
 * __memp_pbh --
 *	Display a BH structure.
 */
static void
__memp_pbh(dbmp, bhp, fmap, fp)
	DB_MPOOL *dbmp;
	BH *bhp;
	size_t *fmap;
	FILE *fp;
{
	static const FN fn[] = {
		{ BH_CALLPGIN,	"callpgin" },
		{ BH_DIRTY,	"dirty" },
		{ BH_DISCARD,	"discard" },
		{ BH_LOCKED,	"locked" },
		{ BH_TRASH,	"trash" },
		{ BH_WRITE,	"write" },
		{ 0,		NULL }
	};
	int i;

	for (i = 0; i < FMAP_ENTRIES; ++i)
		if (fmap[i] == INVALID_ROFF || fmap[i] == bhp->mf_offset)
			break;

	if (fmap[i] == INVALID_ROFF)
		(void)fprintf(fp, "  %4lu, %lu, %2lu, %lu",
		    (u_long)bhp->pgno, (u_long)bhp->mf_offset,
		    (u_long)bhp->ref, (u_long)R_OFFSET(&dbmp->reginfo, bhp));
	else
		(void)fprintf(fp, "  %4lu,   #%d,  %2lu, %lu",
		    (u_long)bhp->pgno, i + 1,
		    (u_long)bhp->ref, (u_long)R_OFFSET(&dbmp->reginfo, bhp));

	__db_prflags(bhp->flags, fn, fp);

	(void)fprintf(fp, "\n");
}
