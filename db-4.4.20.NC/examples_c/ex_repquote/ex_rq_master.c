/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2001-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: ex_rq_master.c,v 12.5 2005/11/02 22:14:24 alanb Exp $
 */

#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <db.h>

#include "ex_repquote.h"

static void *master_loop __P((void *));

#define	BUFSIZE 1024

int
domaster(dbenv, progname)
	DB_ENV *dbenv;
	const char *progname;
{
	int ret;
	thread_t interface_thr;
#ifndef _WIN32
	int t_ret;
	pthread_attr_t attr;

	/* Spawn off a thread to handle the basic master interface. */
	if ((ret = pthread_attr_init(&attr)) != 0 ||
	    (ret = pthread_attr_setdetachstate(&attr,
	    PTHREAD_CREATE_DETACHED)) != 0) {
		dbenv->err(dbenv, ret,
		    "can't set up pthread DETACHED attribute");
		goto err;
	}
#endif

	if ((ret = thread_create(&interface_thr,
	    &attr, master_loop, (void *)dbenv)) != 0) {
		dbenv->err(dbenv, ret, "can't create master thread");
		goto err;
	}

err:
#ifndef _WIN32
	if ((t_ret = pthread_attr_destroy(&attr)) != 0) {
		dbenv->err(dbenv, t_ret, "can't destroy thread attribute");
		if (ret == 0)
			ret = t_ret;
	}
#endif
	COMPQUIET(progname, NULL);

	return (ret);
}

static void *
master_loop(dbenvv)
	void *dbenvv;
{
	DB *dbp;
	DBT key, data;
	DB_ENV *dbenv;
	DB_TXN *txn;
	int ret;
	char buf[BUFSIZE], *rbuf;

	dbp = NULL;
	txn = NULL;

	dbenv = (DB_ENV *)dbenvv;
	/*
	 * Check if the database exists and if it verifies cleanly.
	 * If it does, run with it; else recreate it and go.  Note
	 * that we have to verify outside of the environment.
	 */
#ifdef NOTDEF
	if ((ret = db_create(&dbp, NULL, 0)) != 0)
		goto err;
	if ((ret = dbp->verify(dbp, DATABASE, NULL, NULL, 0)) != 0) {
		if ((ret = dbp->remove(dbp, DATABASE, NULL, 0)) != 0 &&
		    ret != DB_NOTFOUND && ret != ENOENT)
			goto err;
#endif
		if ((ret = db_create(&dbp, dbenv, 0)) != 0)
			goto err;
		/* Set page size small so we can easily do page allocation. */
		if ((ret = dbp->set_pagesize(dbp, 512)) != 0)
			goto err;

		if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0)
			goto err;
		if ((ret = dbp->open(dbp, txn, DATABASE,
		    NULL, DB_BTREE, DB_CREATE /* | DB_THREAD */, 0)) != 0)
			goto err;
		ret = txn->commit(txn, 0);
		txn = NULL;
		if (ret != 0) {
			dbp = NULL;
			goto err;
		}

#ifdef NOTDEF
	} else {
		/* Reopen in the environment. */
		if ((ret = dbp->close(dbp, 0)) != 0)
			goto err;
		if ((ret = db_create(&dbp, dbenv, 0)) != 0)
			goto err;
		if ((ret = dbp->open(dbp,
		    DATABASE, NULL, DB_UNKNOWN, DB_THREAD, 0)) != 0)
			goto err;
	}
#endif
	/*
	 * XXX
	 * It would probably be kind of cool to do this in Tcl and
	 * have a nice GUI.  It would also be cool to be independently
	 * wealthy.
	 */
	memset(&key, 0, sizeof(key));
	memset(&data, 0, sizeof(data));
	for (;;) {
		printf("QUOTESERVER> ");
		fflush(stdout);

		if (fgets(buf, sizeof(buf), stdin) == NULL)
			break;
		(void)strtok(&buf[0], " \t\n");
		rbuf = strtok(NULL, " \t\n");
		if (rbuf == NULL || rbuf[0] == '\0') {
			if (strncmp(buf, "exit", 4) == 0 ||
			    strncmp(buf, "quit", 4) == 0)
				break;
			dbenv->errx(dbenv, "Format: TICKER VALUE");
			continue;
		}

		key.data = buf;
		key.size = (u_int32_t)strlen(buf);

		data.data = rbuf;
		data.size = (u_int32_t)strlen(rbuf);

		if ((ret = dbenv->txn_begin(dbenv, NULL, &txn, 0)) != 0)
			goto err;
		switch (ret =
		    dbp->put(dbp, txn, &key, &data, 0)) {
		case 0:
			break;
		default:
			dbp->err(dbp, ret, "DB->put");
			if (ret != DB_KEYEXIST)
				goto err;
			break;
		}
		ret = txn->commit(txn, 0);
		txn = NULL;
		if (ret != 0)
			goto err;
	}

err:	if (txn != NULL)
		(void)txn->abort(txn);

	if (dbp != NULL)
		(void)dbp->close(dbp, DB_NOSYNC);

	return ((void *)(uintptr_t)ret);
}
