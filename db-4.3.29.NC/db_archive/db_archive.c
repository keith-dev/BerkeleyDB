/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2004
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: db_archive.c,v 11.46 2004/06/10 01:00:08 bostic Exp $
 */

#include "db_config.h"

#ifndef lint
static const char copyright[] =
    "Copyright (c) 1996-2004\nSleepycat Software Inc.  All rights reserved.\n";
#endif

#ifndef NO_SYSTEM_INCLUDES
#include <sys/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#endif

#include "db_int.h"

int main __P((int, char *[]));
int usage __P((void));
int version_check __P((const char *));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	extern char *optarg;
	extern int optind;
	const char *progname = "db_archive";
	DB_ENV	*dbenv;
	u_int32_t flags;
	int ch, exitval, ret, verbose;
	char **file, *home, **list, *passwd;

	if ((ret = version_check(progname)) != 0)
		return (ret);

	dbenv = NULL;
	flags = 0;
	exitval = verbose = 0;
	home = passwd = NULL;
	file = list = NULL;
	while ((ch = getopt(argc, argv, "adh:lP:sVv")) != EOF)
		switch (ch) {
		case 'a':
			LF_SET(DB_ARCH_ABS);
			break;
		case 'd':
			LF_SET(DB_ARCH_REMOVE);
			break;
		case 'h':
			home = optarg;
			break;
		case 'l':
			LF_SET(DB_ARCH_LOG);
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
		case 's':
			LF_SET(DB_ARCH_DATA);
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

	/* Handle possible interruptions. */
	__db_util_siginit();

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
	if ((ret = dbenv->open(dbenv,
	    home, DB_JOINENV | DB_USE_ENVIRON, 0)) != 0 &&
	    (ret == DB_VERSION_MISMATCH ||
	    (ret = dbenv->open(dbenv, home, DB_CREATE |
	    DB_INIT_LOG | DB_PRIVATE | DB_USE_ENVIRON, 0)) != 0)) {
		dbenv->err(dbenv, ret, "DB_ENV->open");
		goto shutdown;
	}

	/* Get the list of names. */
	if ((ret = dbenv->log_archive(dbenv, &list, flags)) != 0) {
		dbenv->err(dbenv, ret, "DB_ENV->log_archive");
		goto shutdown;
	}

	/* Print the list of names. */
	if (list != NULL) {
		for (file = list; *file != NULL; ++file)
			printf("%s\n", *file);
		free(list);
	}

	if (0) {
shutdown:	exitval = 1;
	}
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
	(void)fprintf(stderr,
	    "usage: db_archive [-adlsVv] [-h home] [-P password]\n");
	return (EXIT_FAILURE);
}

int
version_check(progname)
	const char *progname;
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
