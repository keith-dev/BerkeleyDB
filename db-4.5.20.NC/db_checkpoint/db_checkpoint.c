/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: db_checkpoint.c,v 12.15 2006/08/26 09:23:25 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996-2006\nOracle Corporation.  All rights reserved.\n";
#endif

int	 main __P((int, char *[]));
int	 usage __P((void));
int	 version_check __P((void));

const char *progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	DB_ENV	*dbenv;
	time_t now;
	long argval;
	u_int32_t flags, kbytes, minutes, seconds;
	int ch, exitval, once, ret, verbose;
	char *home, *logfile, *passwd, time_buf[CTIME_BUFLEN];

	if ((progname = strrchr(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		++progname;

	if ((ret = version_check()) != 0)
		return (ret);

	/*
	 * !!!
	 * Don't allow a fully unsigned 32-bit number, some compilers get
	 * upset and require it to be specified in hexadecimal and so on.
	 */
#define	MAX_UINT32_T	2147483647

	dbenv = NULL;
	kbytes = minutes = 0;
	exitval = once = verbose = 0;
	flags = 0;
	home = logfile = passwd = NULL;
	while ((ch = getopt(argc, argv, "1h:k:L:P:p:Vv")) != EOF)
		switch (ch) {
		case '1':
			once = 1;
			flags = DB_FORCE;
			break;
		case 'h':
			home = optarg;
			break;
		case 'k':
			if (__db_getlong(NULL, progname,
			    optarg, 1, (long)MAX_UINT32_T, &argval))
				return (EXIT_FAILURE);
			kbytes = (u_int32_t)argval;
			break;
		case 'L':
			logfile = optarg;
			break;
		case 'P':
			passwd = strdup(optarg);
			memset(optarg, 0, strlen(optarg));
			if (passwd == NULL) {
				fprintf(stderr, "%s: strdup: %s\n",
				    progname, strerror(errno));
				return (EXIT_FAILURE);
			}
			break;
		case 'p':
			if (__db_getlong(NULL, progname,
			    optarg, 1, (long)MAX_UINT32_T, &argval))
				return (EXIT_FAILURE);
			minutes = (u_int32_t)argval;
			break;
		case 'V':
			printf("%s\n", db_version(NULL, NULL, NULL));
			return (EXIT_SUCCESS);
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			return (usage());
		}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		return (usage());

	if (once == 0 && kbytes == 0 && minutes == 0) {
		(void)fprintf(stderr,
		    "%s: at least one of -1, -k and -p must be specified\n",
		    progname);
		return (usage());
	}

	/* Handle possible interruptions. */
	__db_util_siginit();

	/* Log our process ID. */
	if (logfile != NULL && __db_util_logset(progname, logfile))
		goto shutdown;

	/*
	 * Create an environment object and initialize it for error
	 * reporting.
	 */
	if ((ret = db_env_create(&dbenv, 0)) != 0) {
		fprintf(stderr,
		    "%s: db_env_create: %s\n", progname, db_strerror(ret));
		goto shutdown;
	}

	dbenv->set_errfile(dbenv, stderr);
	dbenv->set_errpfx(dbenv, progname);

	if (passwd != NULL && (ret = dbenv->set_encrypt(dbenv,
	    passwd, DB_ENCRYPT_AES)) != 0) {
		dbenv->err(dbenv, ret, "set_passwd");
		goto shutdown;
	}

	/*
	 * If attaching to a pre-existing environment fails, create a
	 * private one and try again.
	 */
	if ((ret = dbenv->open(dbenv, home, DB_USE_ENVIRON, 0)) != 0 &&
	    (!once || ret == DB_VERSION_MISMATCH ||
	    (ret = dbenv->open(dbenv, home,
	    DB_CREATE | DB_INIT_TXN | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0)) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto shutdown;
	}

	/*
	 * If we have only a time delay, then we'll sleep the right amount
	 * to wake up when a checkpoint is necessary.  If we have a "kbytes"
	 * field set, then we'll check every 30 seconds.
	 */
	seconds = kbytes != 0 ? 30 : minutes * 60;
	while (!__db_util_interrupted()) {
		if (verbose) {
			(void)time(&now);
			dbenv->errx(dbenv,
		    "checkpoint begin: %s", __db_ctime(&now, time_buf));
		}

		if ((ret = dbenv->txn_checkpoint(dbenv,
		    kbytes, minutes, flags)) != 0) {
			dbenv->err(dbenv, ret, "txn_checkpoint");
			goto shutdown;
		}

		if (verbose) {
			(void)time(&now);
			dbenv->errx(dbenv,
		    "checkpoint complete: %s", __db_ctime(&now, time_buf));
		}

		if (once)
			break;

		__os_sleep(dbenv, seconds, 0);
	}

	if (0) {
shutdown:	exitval = 1;
	}

	/* Clean up the logfile. */
	if (logfile != NULL)
		(void)remove(logfile);

	/* Clean up the environment. */
	if (dbenv != NULL && (ret = dbenv->close(dbenv, 0)) != 0) {
		exitval = 1;
		fprintf(stderr,
		    "%s: dbenv->close: %s\n", progname, db_strerror(ret));
	}

	if (passwd != NULL)
		free(passwd);

	/* Resend any caught signal. */
	__db_util_sigresend();

	return (exitval == 0 ? EXIT_SUCCESS : EXIT_FAILURE);
}

int
usage()
{
	(void)fprintf(stderr, "usage: %s [-1Vv]\n\t%s\n", progname,
	    "[-h home] [-k kbytes] [-L file] [-P password] [-p min]");
	return (EXIT_FAILURE);
}

int
version_check()
{
	int v_major, v_minor, v_patch;

	/* Make sure we're loaded with the right version of the DB library. */
	(void)db_version(&v_major, &v_minor, &v_patch);
	if (v_major != DB_VERSION_MAJOR || v_minor != DB_VERSION_MINOR) {
		fprintf(stderr,
	"%s: version %d.%d doesn't match library version %d.%d\n",
		    progname, DB_VERSION_MAJOR, DB_VERSION_MINOR,
		    v_major, v_minor);
		return (EXIT_FAILURE);
	}
	return (0);
}
