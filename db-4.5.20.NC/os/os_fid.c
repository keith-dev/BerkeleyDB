/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: os_fid.c,v 12.11 2006/08/24 14:46:17 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

/*
 * __os_fileid --
 *	Return a unique identifier for a file.
 *
 * PUBLIC: int __os_fileid __P((DB_ENV *, const char *, int, u_int8_t *));
 */
int
__os_fileid(dbenv, fname, unique_okay, fidp)
	DB_ENV *dbenv;
	const char *fname;
	int unique_okay;
	u_int8_t *fidp;
{
	pid_t pid;
	size_t i;
	u_int32_t tmp;
	u_int8_t *p;

#ifdef HAVE_STAT
	struct stat sb;
	int ret;

	/*
	 * The structure of a fileid on a POSIX/UNIX system is: ino(4) dev(4)
	 * time(4) pid(4) extra(4).
	 *
	 * For real files, which have a backing inode and device, the first
	 * 16 bytes are filled in and the extra bytes are left 0.  For
	 * temporary files, the inode and device fields are left blank and
	 * the extra four bytes are filled in with a random value.
	 *
	 * Clear the buffer.
	 */
	memset(fidp, 0, DB_FILE_ID_LEN);
#ifdef HAVE_VXWORKS
	RETRY_CHK((stat((char *)fname, &sb)), ret);
#else
	RETRY_CHK((stat(fname, &sb)), ret);
#endif
	if (ret != 0) {
		__db_syserr(dbenv, ret, "stat: %s", fname);
		return (__os_posix_err(ret));
	}

	/*
	 * !!!
	 * Nothing is ever big enough -- on Sparc V9, st_ino, st_dev and the
	 * time_t types are all 8 bytes.  As DB_FILE_ID_LEN is only 20 bytes,
	 * we convert to a (potentially) smaller fixed-size type and use it.
	 *
	 * We don't worry about byte sexing or the actual variable sizes.
	 *
	 * When this routine is called from the DB access methods, it's only
	 * called once -- whatever ID is generated when a database is created
	 * is stored in the database file's metadata, and that is what is
	 * saved in the mpool region's information to uniquely identify the
	 * file.
	 *
	 * When called from the mpool layer this routine will be called each
	 * time a new thread of control wants to share the file, which makes
	 * things tougher.  As far as byte sexing goes, since the mpool region
	 * lives on a single host, there's no issue of that -- the entire
	 * region is byte sex dependent.  As far as variable sizes go, we make
	 * the simplifying assumption that 32-bit and 64-bit processes will
	 * get the same 32-bit values if we truncate any returned 64-bit value
	 * to a 32-bit value.  When we're called from the mpool layer, though,
	 * we need to be careful not to include anything that isn't
	 * reproducible for a given file, such as the timestamp or serial
	 * number.
	 */
	tmp = (u_int32_t)sb.st_ino;
	for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
		*fidp++ = *p++;

	tmp = (u_int32_t)sb.st_dev;
	for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
		*fidp++ = *p++;
#else
	 /* Use the file name. */
	 (void)strncpy(fidp, fname, DB_FILE_ID_LEN);
#endif /* HAVE_STAT */

	if (unique_okay) {
		static u_int32_t fid_serial = 0;

		/* Add in 32-bits of (hopefully) unique number. */
		__os_unique_id(dbenv, &tmp);
		for (p = (u_int8_t *)&tmp, i = sizeof(u_int32_t); i > 0; --i)
			*fidp++ = *p++;

		/*
		 * Initialize/increment the serial number we use to help
		 * avoid fileid collisions.  Note we don't bother with
		 * locking; it's unpleasant to do from down in here, and
		 * if we race on this no real harm will be done, since the
		 * finished fileid has so many other components.
		 *
		 * We use the bottom 32-bits of the process ID, hoping they
		 * are more random than the top 32-bits (should we be on a
		 * machine with 64-bit process IDs).
		 *
		 * We increment by 100000 on each call as a simple way of
		 * randomizing; simply incrementing seems potentially less
		 * useful if pids are also simply incremented, since this
		 * is process-local and we may be one of a set of processes
		 * starting up.  100000 pushes us out of pid space on most
		 * 32-bit platforms, and has few interesting properties in
		 * base 2.
		 */
		if (fid_serial == 0) {
			dbenv->thread_id(dbenv, &pid, NULL);
			fid_serial = (u_int32_t)pid;
		} else
			fid_serial += 100000;

		for (p =
		    (u_int8_t *)&fid_serial, i = sizeof(u_int32_t); i > 0; --i)
			*fidp++ = *p++;
	}

	return (0);
}
