/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: mp_sync.c,v 12.24 2006/08/24 14:46:15 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/log.h"
#include "dbinc/mp.h"

typedef struct {
	DB_MPOOL_HASH *track_hp;	/* Hash bucket. */

	roff_t	  track_off;		/* Page file offset. */
	db_pgno_t track_pgno;		/* Page number. */
} BH_TRACK;

static int __bhcmp __P((const void *, const void *));
static int __memp_close_flush_files __P((DB_ENV *, DB_MPOOL *, int));
static int __memp_sync_files __P((DB_ENV *, DB_MPOOL *));
static int __memp_sync_file __P((DB_ENV *,
		MPOOLFILE *, void *, u_int32_t *, u_int32_t));

/*
 * __memp_walk_files --
 * PUBLIC: int __memp_walk_files __P((DB_ENV *, MPOOL *,
 * PUBLIC:	int (*) __P((DB_ENV *, MPOOLFILE *, void *,
 * PUBLIC:	u_int32_t *, u_int32_t)), void *, u_int32_t *, u_int32_t));
 */
int
__memp_walk_files(dbenv, mp, func, arg, countp, flags)
	DB_ENV *dbenv;
	MPOOL *mp;
	int (*func)__P((DB_ENV *, MPOOLFILE *, void *, u_int32_t *, u_int32_t));
	void *arg;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOLFILE *mfp;
	int i, ret, t_ret;

	dbmp = dbenv->mp_handle;
	ret = 0;

	hp = R_ADDR(dbmp->reginfo, mp->ftab);
	for (i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(dbenv, hp->mtx_hash);
		SH_TAILQ_FOREACH(mfp, &hp->hash_bucket, q, __mpoolfile) {
			if ((t_ret = func(dbenv,
			    mfp, arg, countp, flags)) != 0 && ret == 0)
				ret = t_ret;
			if (ret != 0 && !LF_ISSET(DB_STAT_NOERROR))
				break;
		}
		MUTEX_UNLOCK(dbenv, hp->mtx_hash);
		if (ret != 0 && !LF_ISSET(DB_STAT_NOERROR))
			break;
	}
	return (ret);
}

/*
 * __memp_sync_pp --
 *	DB_ENV->memp_sync pre/post processing.
 *
 * PUBLIC: int __memp_sync_pp __P((DB_ENV *, DB_LSN *));
 */
int
__memp_sync_pp(dbenv, lsnp)
	DB_ENV *dbenv;
	DB_LSN *lsnp;
{
	DB_THREAD_INFO *ip;
	int ret;

	PANIC_CHECK(dbenv);
	ENV_REQUIRES_CONFIG(dbenv,
	    dbenv->mp_handle, "memp_sync", DB_INIT_MPOOL);

	/*
	 * If no LSN is provided, flush the entire cache (reasonable usage
	 * even if there's no log subsystem configured).
	 */
	if (lsnp != NULL)
		ENV_REQUIRES_CONFIG(dbenv,
		    dbenv->lg_handle, "memp_sync", DB_INIT_LOG);

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__memp_sync(dbenv, lsnp)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __memp_sync --
 *	DB_ENV->memp_sync.
 *
 * PUBLIC: int __memp_sync __P((DB_ENV *, DB_LSN *));
 */
int
__memp_sync(dbenv, lsnp)
	DB_ENV *dbenv;
	DB_LSN *lsnp;
{
	DB_MPOOL *dbmp;
	MPOOL *mp;
	int ret;

	dbmp = dbenv->mp_handle;
	mp = dbmp->reginfo[0].primary;

	/* If we've flushed to the requested LSN, return that information. */
	if (lsnp != NULL) {
		MPOOL_SYSTEM_LOCK(dbenv);
		if (LOG_COMPARE(lsnp, &mp->lsn) <= 0) {
			*lsnp = mp->lsn;

			MPOOL_SYSTEM_UNLOCK(dbenv);
			return (0);
		}
		MPOOL_SYSTEM_UNLOCK(dbenv);
	}

	if ((ret = __memp_sync_int(dbenv, NULL, 0, DB_SYNC_CACHE, NULL)) != 0)
		return (ret);

	if (lsnp != NULL) {
		MPOOL_SYSTEM_LOCK(dbenv);
		if (LOG_COMPARE(lsnp, &mp->lsn) > 0)
			mp->lsn = *lsnp;
		MPOOL_SYSTEM_UNLOCK(dbenv);
	}

	return (0);
}

/*
 * __memp_fsync_pp --
 *	DB_MPOOLFILE->sync pre/post processing.
 *
 * PUBLIC: int __memp_fsync_pp __P((DB_MPOOLFILE *));
 */
int
__memp_fsync_pp(dbmfp)
	DB_MPOOLFILE *dbmfp;
{
	DB_ENV *dbenv;
	DB_THREAD_INFO *ip;
	int ret;

	dbenv = dbmfp->dbenv;

	PANIC_CHECK(dbenv);
	MPF_ILLEGAL_BEFORE_OPEN(dbmfp, "DB_MPOOLFILE->sync");

	ENV_ENTER(dbenv, ip);
	REPLICATION_WRAP(dbenv, (__memp_fsync(dbmfp)), ret);
	ENV_LEAVE(dbenv, ip);
	return (ret);
}

/*
 * __memp_fsync --
 *	DB_MPOOLFILE->sync.
 *
 * PUBLIC: int __memp_fsync __P((DB_MPOOLFILE *));
 */
int
__memp_fsync(dbmfp)
	DB_MPOOLFILE *dbmfp;
{
	MPOOLFILE *mfp;

	mfp = dbmfp->mfp;

	/*
	 * If this handle doesn't have a file descriptor that's open for
	 * writing, or if the file is a temporary, or if the file hasn't
	 * been written since it was flushed, there's no reason to proceed
	 * further.
	 */
	if (F_ISSET(dbmfp, MP_READONLY))
		return (0);

	if (F_ISSET(dbmfp->mfp, MP_TEMP) || dbmfp->mfp->no_backing_file)
		return (0);

	if (mfp->file_written == 0)
		return (0);

	return (__memp_sync_int(dbmfp->dbenv, dbmfp, 0, DB_SYNC_FILE, NULL));
}

/*
 * __mp_xxx_fh --
 *	Return a file descriptor for DB 1.85 compatibility locking.
 *
 * PUBLIC: int __mp_xxx_fh __P((DB_MPOOLFILE *, DB_FH **));
 */
int
__mp_xxx_fh(dbmfp, fhp)
	DB_MPOOLFILE *dbmfp;
	DB_FH **fhp;
{
	/*
	 * This is a truly spectacular layering violation, intended ONLY to
	 * support compatibility for the DB 1.85 DB->fd call.
	 *
	 * Sync the database file to disk, creating the file as necessary.
	 *
	 * We skip the MP_READONLY and MP_TEMP tests done by memp_fsync(3).
	 * The MP_READONLY test isn't interesting because we will either
	 * already have a file descriptor (we opened the database file for
	 * reading) or we aren't readonly (we created the database which
	 * requires write privileges).  The MP_TEMP test isn't interesting
	 * because we want to write to the backing file regardless so that
	 * we get a file descriptor to return.
	 */
	if ((*fhp = dbmfp->fhp) != NULL)
		return (0);

	return (__memp_sync_int(dbmfp->dbenv, dbmfp, 0, DB_SYNC_FILE, NULL));
}

/*
 * __memp_sync_int --
 *	Mpool sync internal function.
 *
 * PUBLIC: int __memp_sync_int __P((DB_ENV *,
 * PUBLIC:     DB_MPOOLFILE *, u_int32_t, db_sync_op, u_int32_t *));
 */
int
__memp_sync_int(dbenv, dbmfp, trickle_max, op, wrotep)
	DB_ENV *dbenv;
	DB_MPOOLFILE *dbmfp;
	u_int32_t trickle_max, *wrotep;
	db_sync_op op;
{
	BH *bhp;
	BH_TRACK *bharray;
	DB_MPOOL *dbmp;
	DB_MPOOL_HASH *hp;
	MPOOL *c_mp, *mp;
	MPOOLFILE *mfp;
	db_mutex_t mutex;
	roff_t last_mf_offset;
	u_int32_t ar_cnt, ar_max, i, n_cache, remaining, wrote;
	int filecnt, hb_lock, maxopenfd, maxwrite, maxwrite_sleep;
	int pass, ret, t_ret, wait_cnt, write_cnt;

	dbmp = dbenv->mp_handle;
	mp = dbmp->reginfo[0].primary;
	last_mf_offset = INVALID_ROFF;
	filecnt = pass = wrote = 0;

	/* Get shared configuration information. */
	MPOOL_SYSTEM_LOCK(dbenv);
	maxopenfd = mp->mp_maxopenfd;
	maxwrite = mp->mp_maxwrite;
	maxwrite_sleep = mp->mp_maxwrite_sleep;
	MPOOL_SYSTEM_UNLOCK(dbenv);

	/* Assume one dirty page per bucket. */
	ar_max = mp->nreg * mp->htab_buckets;
	if ((ret =
	    __os_malloc(dbenv, ar_max * sizeof(BH_TRACK), &bharray)) != 0)
		return (ret);

	/*
	 * Walk each cache's list of buffers and mark all dirty buffers to be
	 * written and all dirty buffers to be potentially written, depending
	 * on our flags.
	 */
	for (ar_cnt = 0, n_cache = 0; n_cache < mp->nreg; ++n_cache) {
		c_mp = dbmp->reginfo[n_cache].primary;

		hp = R_ADDR(&dbmp->reginfo[n_cache], c_mp->htab);
		for (i = 0; i < c_mp->htab_buckets; i++, hp++) {
			/*
			 * We can check for empty buckets before locking as we
			 * only care if the pointer is zero or non-zero.  We
			 * can ignore empty buckets because we only need write
			 * buffers that were dirty before we started.
			 */
			if (SH_TAILQ_FIRST(&hp->hash_bucket, __bh) == NULL)
				continue;

			MUTEX_LOCK(dbenv, hp->mtx_hash);
			SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh) {
				/* Always ignore clean pages. */
				if (!F_ISSET(bhp, BH_DIRTY))
					continue;

				mfp = R_ADDR(dbmp->reginfo, bhp->mf_offset);

				/*
				 * Ignore in-memory files, even if they are
				 * temp files to whom a backing file has been
				 * allocated.
				 */
				if (mfp->no_backing_file ||
				    F_ISSET(mfp, MP_TEMP))
					continue;

				/*
				 * If we're flushing a specific file, see if
				 * this page is from that file.
				 */
				if (dbmfp != NULL && mfp != dbmfp->mfp)
					continue;

				/*
				 * Ignore files that aren't involved in DB's
				 * transactional operations during checkpoints.
				 */
				if (dbmfp == NULL && mfp->lsn_off == -1)
					continue;

				/* Track the buffer, we want it. */
				bharray[ar_cnt].track_hp = hp;
				bharray[ar_cnt].track_pgno = bhp->pgno;
				bharray[ar_cnt].track_off = bhp->mf_offset;
				ar_cnt++;

				/*
				 * If we run out of space, double and continue.
				 * Don't stop at trickle_max, we want to sort
				 * as large a sample set as possible in order
				 * to minimize disk seeks.
				 */
				if (ar_cnt >= ar_max) {
					if ((ret = __os_realloc(dbenv,
					    (ar_max * 2) * sizeof(BH_TRACK),
					    &bharray)) != 0)
						break;
					ar_max *= 2;
				}
			}
			MUTEX_UNLOCK(dbenv, hp->mtx_hash);

			if (ret != 0)
				goto err;
		}
	}

	/* If there no buffers to write, we're done. */
	if (ar_cnt == 0)
		goto done;

	/*
	 * Write the buffers in file/page order, trying to reduce seeks by the
	 * filesystem and, when pages are smaller than filesystem block sizes,
	 * reduce the actual number of writes.
	 */
	if (ar_cnt > 1)
		qsort(bharray, ar_cnt, sizeof(BH_TRACK), __bhcmp);

	/*
	 * If we're trickling buffers, only write enough to reach the correct
	 * percentage.
	 */
	if (op == DB_SYNC_TRICKLE && ar_cnt > trickle_max)
		ar_cnt = trickle_max;

	/*
	 * Flush the log.  We have to ensure the log records reflecting the
	 * changes on the database pages we're writing have already made it
	 * to disk.  We still have to check the log each time we write a page
	 * (because pages we are about to write may be modified after we have
	 * flushed the log), but in general this will at least avoid any I/O
	 * on the log's part.
	 */
	if (LOGGING_ON(dbenv) && (ret = __log_flush(dbenv, NULL)) != 0)
		goto err;

	/*
	 * Walk the array, writing buffers.  When we write a buffer, we NULL
	 * out its hash bucket pointer so we don't process a slot more than
	 * once.
	 */
	for (i = pass = write_cnt = 0, remaining = ar_cnt; remaining > 0; ++i) {
		if (i >= ar_cnt) {
			i = 0;
			++pass;
			__os_sleep(dbenv, 1, 0);
		}
		if ((hp = bharray[i].track_hp) == NULL)
			continue;

		/* Lock the hash bucket and find the buffer. */
		mutex = hp->mtx_hash;
		MUTEX_LOCK(dbenv, mutex);
		SH_TAILQ_FOREACH(bhp, &hp->hash_bucket, hq, __bh)
			if (bhp->pgno == bharray[i].track_pgno &&
			    bhp->mf_offset == bharray[i].track_off)
				break;

		/*
		 * If we can't find the buffer we're done, somebody else had
		 * to have written it.
		 *
		 * If the buffer isn't dirty, we're done, there's no work
		 * needed.
		 */
		if (bhp == NULL || !F_ISSET(bhp, BH_DIRTY)) {
			MUTEX_UNLOCK(dbenv, mutex);
			--remaining;
			bharray[i].track_hp = NULL;
			continue;
		}

		/*
		 * If the buffer is locked by another thread, ignore it, we'll
		 * come back to it.
		 *
		 * If the buffer is pinned and it's only the first or second
		 * time we have looked at it, ignore it, we'll come back to
		 * it.
		 *
		 * In either case, skip the buffer if we're not required to
		 * write it.
		 */
		if (F_ISSET(bhp, BH_LOCKED) || (bhp->ref != 0 && pass < 2)) {
			MUTEX_UNLOCK(dbenv, mutex);
			if (op != DB_SYNC_CACHE && op != DB_SYNC_FILE) {
				--remaining;
				bharray[i].track_hp = NULL;
			}
			continue;
		}

		/*
		 * The buffer is dirty and may also be pinned.
		 *
		 * Set the sync wait-for count, used to count down outstanding
		 * references to this buffer as they are returned to the cache.
		 */
		bhp->ref_sync = bhp->ref;

		/* Pin the buffer into memory and lock it. */
		++bhp->ref;
		F_SET(bhp, BH_LOCKED);

		/*
		 * Unlock the hash bucket and wait for the wait-for count to
		 * go to 0.   No new thread can acquire the buffer because we
		 * have it locked.
		 *
		 * If a thread attempts to re-pin a page, the wait-for count
		 * will never go to 0 (the thread spins on our buffer lock,
		 * while we spin on the thread's ref count).  Give up if we
		 * don't get the buffer in 3 seconds, we can try again later.
		 *
		 * If, when the wait-for count goes to 0, the buffer is found
		 * to be dirty, write it.
		 */
		MUTEX_UNLOCK(dbenv, mutex);
		for (wait_cnt = 1;
		    bhp->ref_sync != 0 && wait_cnt < 4; ++wait_cnt)
			__os_sleep(dbenv, 1, 0);
		MUTEX_LOCK(dbenv, mutex);
		hb_lock = 1;

		/*
		 * If we've switched files, check to see if we're configured
		 * to close file descriptors.
		 */
		if (maxopenfd != 0 && bhp->mf_offset != last_mf_offset) {
			if (++filecnt >= maxopenfd) {
				filecnt = 0;
				if ((t_ret = __memp_close_flush_files(
				    dbenv, dbmp, 1)) != 0 && ret == 0)
					ret = t_ret;
			}
			last_mf_offset = bhp->mf_offset;
		}

		/*
		 * If the ref_sync count has gone to 0, we're going to be done
		 * with this buffer no matter what happens.
		 */
		if (bhp->ref_sync == 0) {
			--remaining;
			bharray[i].track_hp = NULL;
		}

		/*
		 * If the ref_sync count has gone to 0 and the buffer is still
		 * dirty, we write it.  We only try to write the buffer once.
		 */
		if (bhp->ref_sync == 0 && F_ISSET(bhp, BH_DIRTY)) {
			MUTEX_UNLOCK(dbenv, mutex);
			hb_lock = 0;

			mfp = R_ADDR(dbmp->reginfo, bhp->mf_offset);
			if ((t_ret =
			    __memp_bhwrite(dbmp, hp, mfp, bhp, 1)) == 0)
				++wrote;
			else {
				if (ret == 0)
					ret = t_ret;
				__db_errx
				    (dbenv, "%s: unable to flush page: %lu",
				    __memp_fns(dbmp, mfp), (u_long)bhp->pgno);
			}

			/*
			 * Avoid saturating the disk, sleep once we've done
			 * some number of writes.
			 */
			if (maxwrite != 0 && ++write_cnt >= maxwrite) {
				write_cnt = 0;
				__os_sleep(dbenv, 0, (u_long)maxwrite_sleep);
			}
		}

		/*
		 * If ref_sync count never went to 0, the buffer was written
		 * by another thread, or the write failed, we still have the
		 * buffer locked.
		 *
		 * We may or may not currently hold the hash bucket mutex.  If
		 * the __memp_bhwrite -> __memp_pgwrite call was successful,
		 * __memp_pgwrite will have acquired the hash bucket lock; all
		 * other call paths will leave us without the hash bucket lock.
		 */
		if (F_ISSET(bhp, BH_LOCKED)) {
			if (!hb_lock)
				MUTEX_LOCK(dbenv, mutex);

			F_CLR(bhp, BH_LOCKED);
		}

		/*
		 * Reset the ref_sync count regardless of our success, we're
		 * done with this buffer for now.
		 */
		bhp->ref_sync = 0;

		/* Discard our buffer reference. */
		--bhp->ref;

		/*
		 * If a thread of control is waiting on this buffer, wake it up.
		 */
		if (F_ISSET(hp, IO_WAITER)) {
			F_CLR(hp, IO_WAITER);
			MUTEX_UNLOCK(dbenv, hp->mtx_io);
		}

		/* Release the hash bucket mutex. */
		MUTEX_UNLOCK(dbenv, mutex);
	}

done:	/*
	 * If doing a checkpoint or flushing a file for the application, we
	 * have to force the pages to disk.  We don't do this as we go along
	 * because we want to give the OS as much time as possible to lazily
	 * flush, and because we have to flush files that might not even have
	 * had dirty buffers in the cache, so we have to walk the files list.
	 */
	if (ret == 0 && (op == DB_SYNC_CACHE || op == DB_SYNC_FILE)) {
		if (dbmfp == NULL)
			ret = __memp_sync_files(dbenv, dbmp);
		else
			ret = __os_fsync(dbenv, dbmfp->fhp);
	}

	/* If we've opened files to flush pages, close them. */
	if ((t_ret = __memp_close_flush_files(dbenv, dbmp, 0)) != 0 && ret == 0)
		ret = t_ret;

err:	__os_free(dbenv, bharray);
	if (wrotep != NULL)
		*wrotep = wrote;

	return (ret);
}

static int
__memp_sync_file(dbenv, mfp, argp, countp, flags)
	DB_ENV *dbenv;
	MPOOLFILE *mfp;
	void *argp;
	u_int32_t *countp;
	u_int32_t flags;
{
	DB_MPOOL *dbmp;
	DB_MPOOLFILE *dbmfp;
	int ret, t_ret;

	COMPQUIET(countp, NULL);
	COMPQUIET(flags, 0);

	if (!mfp->file_written || mfp->no_backing_file ||
	    mfp->deadfile || F_ISSET(mfp, MP_TEMP))
		return (0);
	/*
	 * Pin the MPOOLFILE structure into memory, and release the
	 * region mutex allowing us to walk the linked list.  We'll
	 * re-acquire that mutex to move to the next entry in the list.
	 *
	 * This works because we only need to flush current entries,
	 * we don't care about new entries being added, and the linked
	 * list is never re-ordered, a single pass is sufficient.  It
	 * requires MPOOLFILE structures removed before we get to them
	 * be flushed to disk, but that's nothing new, they could have
	 * been removed while checkpoint was running, too.
	 *
	 * Once we have the MPOOLFILE lock, re-check the MPOOLFILE is
	 * not being discarded.  (A thread removing the MPOOLFILE
	 * will: hold the MPOOLFILE mutex, set deadfile, drop the
	 * MPOOLFILE mutex and then acquire the region MUTEX to walk
	 * the linked list and remove the MPOOLFILE structure.  Make
	 * sure the MPOOLFILE wasn't marked dead while we waited for
	 * the mutex.
	 */
	MUTEX_LOCK(dbenv, mfp->mutex);
	if (!mfp->file_written || mfp->deadfile) {
		MUTEX_UNLOCK(dbenv, mfp->mutex);
		return (0);
	}
	++mfp->mpf_cnt;
	MUTEX_UNLOCK(dbenv, mfp->mutex);

	/*
	 * Look for an already open, writeable handle (fsync doesn't
	 * work on read-only Windows handles).
	 */
	dbmp = dbenv->mp_handle;
	MUTEX_LOCK(dbenv, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q) {
		if (dbmfp->mfp != mfp || F_ISSET(dbmfp, MP_READONLY))
			continue;
		/*
		 * We don't want to hold the mutex while calling sync.
		 * Increment the DB_MPOOLFILE handle ref count to pin
		 * it into memory.
		 */
		++dbmfp->ref;
		break;
	}
	MUTEX_UNLOCK(dbenv, dbmp->mutex);

	/* If we don't find a handle we can use, open one. */
	if (dbmfp == NULL) {
		if ((ret = __memp_mf_sync(dbmp, mfp, 0)) != 0) {
			__db_err(dbenv, ret,
			    "%s: unable to flush", (char *)
			    R_ADDR(dbmp->reginfo, mfp->path_off));
		}
	} else {
		ret = __os_fsync(dbenv, dbmfp->fhp);

		if ((t_ret = __memp_fclose(dbmfp, 0)) != 0 && ret == 0)
			ret = t_ret;
	}

	/*
	 * Re-acquire the MPOOLFILE mutex, we need it to modify the
	 * reference count.
	 */
	MUTEX_LOCK(dbenv, mfp->mutex);
	--mfp->mpf_cnt;

	/*
	 * If we wrote the file and there are no open handles (or there
	 * is a single open handle, and it's the one we opened to write
	 * buffers during checkpoint), clear the file_written flag.  We
	 * do this so that applications opening thousands of files don't
	 * loop here opening and flushing those files during checkpoint.
	 *
	 * The danger here is if a buffer were to be written as part of
	 * a checkpoint, and then not be flushed to disk.  This cannot
	 * happen because we only clear file_written when there are no
	 * other users of the MPOOLFILE in the system, and, as we hold
	 * the region lock, no possibility of another thread of control
	 * racing with us to open a MPOOLFILE.
	 */
	if (mfp->mpf_cnt == 0 || (mfp->mpf_cnt == 1 &&
	    dbmfp != NULL && F_ISSET(dbmfp, MP_FLUSH))) {
		mfp->file_written = 0;

		/*
		 * We may be the last reference for a MPOOLFILE, as we
		 * weren't holding the MPOOLFILE mutex when flushing
		 * it's buffers to disk.  If we can discard it, set
		 * a flag to schedule a clean-out pass.   (Not likely,
		 * I mean, what are the chances that there aren't any
		 * buffers in the pool?  Regardless, it might happen.)
		 */
		if (mfp->mpf_cnt == 0 && mfp->block_cnt == 0)
			*(int *)argp = 1;
	}

	/* Unlock the MPOOLFILE, and move to the next entry. */
	MUTEX_UNLOCK(dbenv, mfp->mutex);
	return (0);
}

/*
 * __memp_sync_files --
 *	Sync all the files in the environment, open or not.
 */
static
int __memp_sync_files(dbenv, dbmp)
	DB_ENV *dbenv;
	DB_MPOOL *dbmp;
{
	DB_MPOOL_HASH *hp;
	MPOOL *mp;
	MPOOLFILE *mfp, *next_mfp;
	int i, need_discard_pass, ret;

	need_discard_pass = ret = 0;
	mp = dbmp->reginfo[0].primary;

	ret = __memp_walk_files(dbenv,
	    mp, __memp_sync_file, &need_discard_pass, 0, DB_STAT_NOERROR);

	/*
	 * We may need to do a last pass through the MPOOLFILE list -- if we
	 * were the last reference to an MPOOLFILE, we need to clean it out.
	 */
	if (!need_discard_pass)
		return (ret);

	hp = R_ADDR(dbmp->reginfo, mp->ftab);
	for (i = 0; i < MPOOL_FILE_BUCKETS; i++, hp++) {
		MUTEX_LOCK(dbenv, hp->mtx_hash);
		for (mfp = SH_TAILQ_FIRST(&hp->hash_bucket,
		    __mpoolfile); mfp != NULL; mfp = next_mfp) {
			next_mfp = SH_TAILQ_NEXT(mfp, q, __mpoolfile);
			/*
			 * Do a fast check -- we can check for zero/non-zero
			 * without a mutex on the MPOOLFILE.  If likely to
			 * succeed, lock the MPOOLFILE down and look for real.
			 */
			if (mfp->block_cnt != 0 || mfp->mpf_cnt != 0)
				continue;

			MUTEX_LOCK(dbenv, mfp->mutex);
			if (mfp->block_cnt == 0 && mfp->mpf_cnt == 0)
				(void)__memp_mf_discard(dbmp, mfp);
			else
				MUTEX_UNLOCK(dbenv, mfp->mutex);
		}
		MUTEX_UNLOCK(dbenv, hp->mtx_hash);
	}
	return (ret);
}

/*
 * __memp_mf_sync --
 *	Flush an MPOOLFILE, when no currently open handle is available.
 *
 * PUBLIC: int __memp_mf_sync __P((DB_MPOOL *, MPOOLFILE *, int));
 */
int
__memp_mf_sync(dbmp, mfp, region_locked)
	DB_MPOOL *dbmp;
	MPOOLFILE *mfp;
	int region_locked;
{
	DB_ENV *dbenv;
	DB_FH *fhp;
	int ret, t_ret;
	char *rpath;

	dbenv = dbmp->dbenv;

	/*
	 * We need to be holding the region lock: we're using the path name
	 * and __memp_nameop might try and rename the file.
	 */
	if (!region_locked)
		MPOOL_SYSTEM_LOCK(dbenv);

	if ((ret = __db_appname(dbenv, DB_APP_DATA,
	    R_ADDR(dbmp->reginfo, mfp->path_off), 0, NULL, &rpath)) == 0) {
		if ((ret = __os_open(dbenv, rpath, 0, 0, &fhp)) == 0) {
			ret = __os_fsync(dbenv, fhp);
			if ((t_ret =
			    __os_closehandle(dbenv, fhp)) != 0 && ret == 0)
				ret = t_ret;
		}
		__os_free(dbenv, rpath);
	}

	if (!region_locked)
		MPOOL_SYSTEM_UNLOCK(dbenv);

	return (ret);
}

/*
 * __memp_close_flush_files --
 *	Close files opened only to flush buffers.
 */
static int
__memp_close_flush_files(dbenv, dbmp, dosync)
	DB_ENV *dbenv;
	DB_MPOOL *dbmp;
	int dosync;
{
	DB_MPOOLFILE *dbmfp;
	MPOOLFILE *mfp;
	int ret;

	/*
	 * The routine exists because we must close files opened by sync to
	 * flush buffers.  There are two cases: first, extent files have to
	 * be closed so they may be removed when empty.  Second, regular
	 * files have to be closed so we don't run out of descriptors (for
	 * example, an application partitioning its data into databases
	 * based on timestamps, so there's a continually increasing set of
	 * files).
	 *
	 * We mark files opened in the __memp_bhwrite() function with the
	 * MP_FLUSH flag.  Here we walk through our file descriptor list,
	 * and, if a file was opened by __memp_bhwrite(), we close it.
	 */
retry:	MUTEX_LOCK(dbenv, dbmp->mutex);
	TAILQ_FOREACH(dbmfp, &dbmp->dbmfq, q)
		if (F_ISSET(dbmfp, MP_FLUSH)) {
			F_CLR(dbmfp, MP_FLUSH);
			MUTEX_UNLOCK(dbenv, dbmp->mutex);
			if (dosync) {
				/*
				 * If we have the only open handle on the file,
				 * clear the dirty flag so we don't re-open and
				 * sync it again when discarding the MPOOLFILE
				 * structure.  Clear the flag before the sync
				 * so can't race with a thread writing the file.
				 */
				mfp = dbmfp->mfp;
				if (mfp->mpf_cnt == 1) {
					MUTEX_LOCK(dbenv, mfp->mutex);
					if (mfp->mpf_cnt == 1)
						mfp->file_written = 0;
					MUTEX_UNLOCK(dbenv, mfp->mutex);
				}
				if ((ret = __os_fsync(dbenv, dbmfp->fhp)) != 0)
					return (ret);
			}
			if ((ret = __memp_fclose(dbmfp, 0)) != 0)
				return (ret);
			goto retry;
		}
	MUTEX_UNLOCK(dbenv, dbmp->mutex);

	return (0);
}

static int
__bhcmp(p1, p2)
	const void *p1, *p2;
{
	BH_TRACK *bhp1, *bhp2;

	bhp1 = (BH_TRACK *)p1;
	bhp2 = (BH_TRACK *)p2;

	/* Sort by file (shared memory pool offset). */
	if (bhp1->track_off < bhp2->track_off)
		return (-1);
	if (bhp1->track_off > bhp2->track_off)
		return (1);

	/*
	 * !!!
	 * Defend against badly written quicksort code calling the comparison
	 * function with two identical pointers (e.g., WATCOM C++ (Power++)).
	 */
	if (bhp1->track_pgno < bhp2->track_pgno)
		return (-1);
	if (bhp1->track_pgno > bhp2->track_pgno)
		return (1);
	return (0);
}
