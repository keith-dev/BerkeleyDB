/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1996-2006
 *	Oracle Corporation.  All rights reserved.
 *
 * $Id: txn_method.c,v 12.6 2006/08/24 14:46:53 bostic Exp $
 */

#include "db_config.h"

#include "db_int.h"
#include "dbinc/txn.h"

/*
 * __txn_dbenv_create --
 *	Transaction specific initialization of the DB_ENV structure.
 *
 * PUBLIC: int __txn_dbenv_create __P((DB_ENV *));
 */
int
__txn_dbenv_create(dbenv)
	DB_ENV *dbenv;
{
	/*
	 * !!!
	 * Our caller has not yet had the opportunity to reset the panic
	 * state or turn off mutex locking, and so we can neither check
	 * the panic state or acquire a mutex in the DB_ENV create path.
	 */
	dbenv->tx_max = DEF_MAX_TXNS;

	return (0);
}

/*
 * __txn_dbenv_destroy --
 *	Transaction specific destruction of the DB_ENV structure.
 *
 * PUBLIC: void __txn_dbenv_destroy __P((DB_ENV *));
 */
void
__txn_dbenv_destroy(dbenv)
	DB_ENV *dbenv;
{
	COMPQUIET(dbenv, NULL);
}

/*
 * PUBLIC: int __txn_get_tx_max __P((DB_ENV *, u_int32_t *));
 */
int
__txn_get_tx_max(dbenv, tx_maxp)
	DB_ENV *dbenv;
	u_int32_t *tx_maxp;
{
	ENV_NOT_CONFIGURED(dbenv,
	    dbenv->tx_handle, "DB_ENV->get_tx_max", DB_INIT_TXN);

	if (TXN_ON(dbenv)) {
		/* Cannot be set after open, no lock required to read. */
		*tx_maxp = ((DB_TXNREGION *)
		    dbenv->tx_handle->reginfo.primary)->maxtxns;
	} else
		*tx_maxp = dbenv->tx_max;
	return (0);
}

/*
 * __txn_set_tx_max --
 *	DB_ENV->set_tx_max.
 *
 * PUBLIC: int __txn_set_tx_max __P((DB_ENV *, u_int32_t));
 */
int
__txn_set_tx_max(dbenv, tx_max)
	DB_ENV *dbenv;
	u_int32_t tx_max;
{
	ENV_ILLEGAL_AFTER_OPEN(dbenv, "DB_ENV->set_tx_max");

	dbenv->tx_max = tx_max;
	return (0);
}

/*
 * PUBLIC: int __txn_get_tx_timestamp __P((DB_ENV *, time_t *));
 */
int
__txn_get_tx_timestamp(dbenv, timestamp)
	DB_ENV *dbenv;
	time_t *timestamp;
{
	*timestamp = dbenv->tx_timestamp;
	return (0);
}

/*
 * __txn_set_tx_timestamp --
 *	Set the transaction recovery timestamp.
 *
 * PUBLIC: int __txn_set_tx_timestamp __P((DB_ENV *, time_t *));
 */
int
__txn_set_tx_timestamp(dbenv, timestamp)
	DB_ENV *dbenv;
	time_t *timestamp;
{
	ENV_ILLEGAL_AFTER_OPEN(dbenv, "DB_ENV->set_tx_timestamp");

	dbenv->tx_timestamp = *timestamp;
	return (0);
}
