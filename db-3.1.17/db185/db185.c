/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996, 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 */

#include "db_config.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996-2000\nSleepycat Software Inc.  All rights reserved.\n";
static const char revid[] =
    "$Id: db185.c,v 11.9 2000/03/29 20:50:46 ubell Exp $";
#endif

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#endif

#include "db_int.h"
#include "db185_int.h"

static int  db185_close __P((DB185 *));
static int  db185_del __P((const DB185 *, const DBT185 *, u_int));
static int  db185_fd __P((const DB185 *));
static int  db185_get __P((const DB185 *, const DBT185 *, DBT185 *, u_int));
static void db185_openstderr __P((DB_FH *));
static int  db185_put __P((const DB185 *, DBT185 *, const DBT185 *, u_int));
static int  db185_seq __P((const DB185 *, DBT185 *, DBT185 *, u_int));
static int  db185_sync __P((const DB185 *, u_int));

DB185 *
__db185_open(file, oflags, mode, type, openinfo)
	const char *file;
	int oflags, mode;
	DBTYPE type;
	const void *openinfo;
{
	const BTREEINFO *bi;
	const HASHINFO *hi;
	const RECNOINFO *ri;
	DB *dbp;
	DB185 *db185p;
	DB_FH fh;
	size_t nw;
	int ret;

	dbp = NULL;
	db185p = NULL;

	if ((ret = db_create(&dbp, NULL, 0)) != 0)
		goto err;

	if ((ret = __os_calloc(NULL, 1, sizeof(DB185), &db185p)) != 0)
		goto err;

	/*
	 * !!!
	 * The DBTYPE enum wasn't initialized in DB 185, so it's off-by-one
	 * from DB 2.0.
	 */
	switch (type) {
	case 0:					/* DB_BTREE */
		type = DB_BTREE;
		if ((bi = openinfo) != NULL) {
			if (bi->flags & ~R_DUP)
				goto einval;
			if (bi->flags & R_DUP)
				(void)dbp->set_flags(dbp, DB_DUP);
			if (bi->cachesize != 0)
				(void)dbp->set_cachesize
				    (dbp, 0, bi->cachesize, 0);
			if (bi->minkeypage != 0)
				(void)dbp->set_bt_minkey(dbp, bi->minkeypage);
			if (bi->psize != 0)
				(void)dbp->set_pagesize(dbp, bi->psize);
			/*
			 * !!!
			 * Comparisons and prefix calls work because the DBT
			 * structures in 1.85 and 2.0 have the same initial
			 * fields.
			 */
			if (bi->prefix != NULL)
				dbp->set_bt_prefix(dbp, bi->prefix);
			if (bi->compare != NULL)
				dbp->set_bt_compare(dbp, bi->compare);
			if (bi->lorder != 0)
				dbp->set_lorder(dbp, bi->lorder);
		}
		break;
	case 1:					/* DB_HASH */
		type = DB_HASH;
		if ((hi = openinfo) != NULL) {
			if (hi->bsize != 0)
				(void)dbp->set_pagesize(dbp, hi->bsize);
			if (hi->ffactor != 0)
				(void)dbp->set_h_ffactor(dbp, hi->ffactor);
			if (hi->nelem != 0)
				(void)dbp->set_h_nelem(dbp, hi->nelem);
			if (hi->cachesize != 0)
				(void)dbp->set_cachesize
				    (dbp, 0, hi->cachesize, 0);
			if (hi->hash != NULL)
				(void)dbp->set_h_hash(dbp,
			(u_int32_t (*)__P((const void *, u_int32_t)))hi->hash);
			if (hi->lorder != 0)
				dbp->set_lorder(dbp, hi->lorder);
		}

		break;
	case 2:					/* DB_RECNO */
		type = DB_RECNO;

		/* DB 1.85 did renumbering by default. */
		(void)dbp->set_flags(dbp, DB_RENUMBER);

		/*
		 * !!!
		 * The file name given to DB 1.85 recno is the name of the DB
		 * 2.0 backing file.  If the file doesn't exist, create it if
		 * the user has the O_CREAT flag set, DB 1.85 did it for you,
		 * and DB 2.0 doesn't.
		 *
		 * !!!
		 * Setting the file name to NULL specifies that we're creating
		 * a temporary backing file, in DB 2.X.  If we're opening the
		 * DB file read-only, change the flags to read-write, because
		 * temporary backing files cannot be opened read-only, and DB
		 * 2.X will return an error.  We are cheating here -- if the
		 * application does a put on the database, it will succeed --
		 * although that would be a stupid thing for the application
		 * to do.
		 *
		 * !!!
		 * Note, the file name in DB 1.85 was a const -- we don't do
		 * that in DB 2.0, so do that cast.
		 */
		if (file != NULL) {
			if (oflags & O_CREAT && __os_exists(file, NULL) != 0)
				if (__os_openhandle(NULL, file,
				    oflags, mode, &fh) == 0)
					(void)__os_closehandle(&fh);
			(void)dbp->set_re_source(dbp, file);

			if (O_RDONLY)
				oflags &= ~O_RDONLY;
			oflags |= O_RDWR;
			file = NULL;
		}

		if ((ri = openinfo) != NULL) {
			/*
			 * !!!
			 * We can't support the bfname field.
			 */
#define	BFMSG	"DB: DB 1.85's recno bfname field is not supported.\n"
			if (ri->bfname != NULL) {
				db185_openstderr(&fh);
				(void)__os_write(NULL, &fh,
				    BFMSG, sizeof(BFMSG) - 1, &nw);
				goto einval;
			}

			if (ri->flags & ~(R_FIXEDLEN | R_NOKEY | R_SNAPSHOT))
				goto einval;
			if (ri->flags & R_FIXEDLEN) {
				if (ri->bval != 0)
					(void)dbp->set_re_pad(dbp, ri->bval);
				if (ri->reclen != 0)
					(void)dbp->set_re_len(dbp, ri->reclen);
			} else
				if (ri->bval != 0)
					(void)dbp->set_re_delim(dbp, ri->bval);

			/*
			 * !!!
			 * We ignore the R_NOKEY flag, but that's okay, it was
			 * only an optimization that was never implemented.
			 */
			if (ri->flags & R_SNAPSHOT)
				(void)dbp->set_flags(dbp, DB_SNAPSHOT);

			if (ri->cachesize != 0)
				(void)dbp->set_cachesize
				    (dbp, 0, ri->cachesize, 0);
			if (ri->psize != 0)
				(void)dbp->set_pagesize(dbp, ri->psize);
			if (ri->lorder != 0)
				dbp->set_lorder(dbp, ri->lorder);
		}
		break;
	default:
		goto einval;
	}

	db185p->close = db185_close;
	db185p->del = db185_del;
	db185p->fd = db185_fd;
	db185p->get = db185_get;
	db185p->put = db185_put;
	db185p->seq = db185_seq;
	db185p->sync = db185_sync;

	/*
	 * !!!
	 * Store the returned pointer to the real DB 2.0 structure in the
	 * internal pointer.  Ugly, but we're not going for pretty, here.
	 */
	if ((ret = dbp->open(dbp,
	    file, NULL, type, __db_oflags(oflags), mode)) != 0)
		goto err;

	/* Create the cursor used for sequential ops. */
	if ((ret = dbp->cursor(dbp, NULL, &((DB185 *)db185p)->dbc, 0)) != 0)
		goto err;

	db185p->internal = dbp;
	return (db185p);

einval:	ret = EINVAL;

err:	if (db185p != NULL)
		__os_free(db185p, sizeof(DB185));
	if (dbp != NULL)
		(void)dbp->close(dbp, 0);

	__os_set_errno(ret);
	return (NULL);
}

static int
db185_close(db185p)
	DB185 *db185p;
{
	DB *dbp;
	int ret;

	dbp = (DB *)db185p->internal;

	ret = dbp->close(dbp, 0);

	__os_free(db185p, sizeof(DB185));

	if (ret == 0)
		return (0);

	__os_set_errno(ret);
	return (-1);
}

static int
db185_del(db185p, key185, flags)
	const DB185 *db185p;
	const DBT185 *key185;
	u_int flags;
{
	DB *dbp;
	DBT key;
	int ret;

	dbp = (DB *)db185p->internal;

	memset(&key, 0, sizeof(key));
	key.data = key185->data;
	key.size = key185->size;

	if (flags & ~R_CURSOR)
		goto einval;
	if (flags & R_CURSOR)
		ret = db185p->dbc->c_del(db185p->dbc, 0);
	else
		ret = dbp->del(dbp, NULL, &key, 0);

	switch (ret) {
	case 0:
		return (0);
	case DB_NOTFOUND:
		return (1);
	}

	__os_set_errno(ret);
	return (-1);

einval:	__os_set_errno(EINVAL);
	return (-1);
}

static int
db185_fd(db185p)
	const DB185 *db185p;
{
	DB *dbp;
	int fd, ret;

	dbp = (DB *)db185p->internal;

	if ((ret = dbp->fd(dbp, &fd)) == 0)
		return (fd);

	__os_set_errno(ret);
	return (-1);
}

static int
db185_get(db185p, key185, data185, flags)
	const DB185 *db185p;
	const DBT185 *key185;
	DBT185 *data185;
	u_int flags;
{
	DB *dbp;
	DBT key, data;
	int ret;

	dbp = (DB *)db185p->internal;

	memset(&key, 0, sizeof(key));
	key.data = key185->data;
	key.size = key185->size;
	memset(&data, 0, sizeof(data));
	data.data = data185->data;
	data.size = data185->size;

	if (flags)
		goto einval;

	switch (ret = dbp->get(dbp, NULL, &key, &data, 0)) {
	case 0:
		data185->data = data.data;
		data185->size = data.size;
		return (0);
	case DB_NOTFOUND:
		return (1);
	}

	__os_set_errno(ret);
	return (-1);

einval:	__os_set_errno(EINVAL);
	return (-1);
}

static int
db185_put(db185p, key185, data185, flags)
	const DB185 *db185p;
	DBT185 *key185;
	const DBT185 *data185;
	u_int flags;
{
	DB *dbp;
	DBC *dbcp_put;
	DBT key, data;
	int ret;

	dbp = (DB *)db185p->internal;

	memset(&key, 0, sizeof(key));
	key.data = key185->data;
	key.size = key185->size;
	memset(&data, 0, sizeof(data));
	data.data = data185->data;
	data.size = data185->size;

	switch (flags) {
	case 0:
		ret = dbp->put(dbp, NULL, &key, &data, 0);
		break;
	case R_CURSOR:
		ret = db185p->dbc->c_put(db185p->dbc, &key, &data, DB_CURRENT);
		break;
	case R_IAFTER:
	case R_IBEFORE:
		if (dbp->type != DB_RECNO)
			goto einval;

		if ((ret = dbp->cursor(dbp, NULL, &dbcp_put, 0)) != 0) {
			__os_set_errno(ret);
			return (-1);
		}
		if ((ret =
		    dbcp_put->c_get(dbcp_put, &key, &data, DB_SET)) != 0) {
			(void)dbcp_put->c_close(dbcp_put);
			__os_set_errno(ret);
			return (-1);
		}
		memset(&data, 0, sizeof(data));
		data.data = data185->data;
		data.size = data185->size;
		ret = dbcp_put->c_put(dbcp_put,
		    &key, &data, flags == R_IAFTER ? DB_AFTER : DB_BEFORE);
		(void)dbcp_put->c_close(dbcp_put);
		__os_set_errno(ret);
		break;
	case R_NOOVERWRITE:
		ret = dbp->put(dbp, NULL, &key, &data, DB_NOOVERWRITE);
		break;
	case R_SETCURSOR:
		if (dbp->type != DB_BTREE && dbp->type != DB_RECNO)
			goto einval;

		if ((ret = dbp->put(dbp, NULL, &key, &data, 0)) != 0)
			break;
		ret =
		    db185p->dbc->c_get(db185p->dbc, &key, &data, DB_SET_RANGE);
		break;
	default:
		goto einval;
	}

	switch (ret) {
	case 0:
		key185->data = key.data;
		key185->size = key.size;
		return (0);
	case DB_KEYEXIST:
		return (1);
	}
	__os_set_errno(ret);
	return (-1);

einval:	__os_set_errno(EINVAL);
	return (-1);
}

static int
db185_seq(db185p, key185, data185, flags)
	const DB185 *db185p;
	DBT185 *key185, *data185;
	u_int flags;
{
	DB *dbp;
	DBT key, data;
	int ret;

	dbp = (DB *)db185p->internal;

	memset(&key, 0, sizeof(key));
	key.data = key185->data;
	key.size = key185->size;
	memset(&data, 0, sizeof(data));
	data.data = data185->data;
	data.size = data185->size;

	switch (flags) {
	case R_CURSOR:
		flags = DB_SET_RANGE;
		break;
	case R_FIRST:
		flags = DB_FIRST;
		break;
	case R_LAST:
		if (dbp->type != DB_BTREE && dbp->type != DB_RECNO)
			goto einval;
		flags = DB_LAST;
		break;
	case R_NEXT:
		flags = DB_NEXT;
		break;
	case R_PREV:
		if (dbp->type != DB_BTREE && dbp->type != DB_RECNO)
			goto einval;
		flags = DB_PREV;
		break;
	default:
		goto einval;
	}
	switch (ret = db185p->dbc->c_get(db185p->dbc, &key, &data, flags)) {
	case 0:
		key185->data = key.data;
		key185->size = key.size;
		data185->data = data.data;
		data185->size = data.size;
		return (0);
	case DB_NOTFOUND:
		return (1);
	}

	__os_set_errno(ret);
	return (-1);

einval:	__os_set_errno(EINVAL);
	return (-1);
}

static int
db185_sync(db185p, flags)
	const DB185 *db185p;
	u_int flags;
{
	DB *dbp;
	DB_FH fh;
	size_t nw;
	int ret;

	dbp = (DB *)db185p->internal;

	switch (flags) {
	case 0:
		break;
	case R_RECNOSYNC:
		/*
		 * !!!
		 * We can't support the R_RECNOSYNC flag.
		 */
#define	RSMSG	"DB: DB 1.85's R_RECNOSYNC sync flag is not supported.\n"
		db185_openstderr(&fh);
		(void)__os_write(NULL, &fh, RSMSG, sizeof(RSMSG) - 1, &nw);
		goto einval;
	default:
		goto einval;
	}

	if ((ret = dbp->sync(dbp, 0)) == 0)
		return (0);

	__os_set_errno(ret);
	return (-1);

einval:	__os_set_errno(EINVAL);
	return (-1);
}

static void
db185_openstderr(fhp)
	DB_FH *fhp;
{
	/* Dummy up the results of an __os_openhandle() on stderr. */
	memset(fhp, 0, sizeof(*fhp));
	F_SET(fhp, DB_FH_VALID);

#ifndef STDERR_FILENO
#define	STDERR_FILENO	2
#endif
	fhp->fd = STDERR_FILENO;
}
