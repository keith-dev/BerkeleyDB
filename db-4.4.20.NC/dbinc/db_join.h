/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1998-2005
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: db_join.h,v 12.2 2005/06/16 20:21:47 bostic Exp $
 */

#ifndef _DB_JOIN_H_
#define	_DB_JOIN_H_
/*
 * Joins use a join cursor that is similar to a regular DB cursor except
 * that it only supports c_get and c_close functionality.  Also, it does
 * not support the full range of flags for get.
 */
typedef struct __join_cursor {
	u_int8_t *j_exhausted;	/* Array of flags; is cursor i exhausted? */
	DBC	**j_curslist;	/* Array of cursors in the join: constant. */
	DBC	**j_fdupcurs;	/* Cursors w/ first instances of current dup. */
	DBC	**j_workcurs;	/* Scratch cursor copies to muck with. */
	DB	 *j_primary;	/* Primary dbp. */
	DBT	  j_key;	/* Used to do lookups. */
	DBT	  j_rdata;	/* Memory used for data return. */
	u_int32_t j_ncurs;	/* How many cursors do we have? */
#define	JOIN_RETRY	0x01	/* Error on primary get; re-return same key. */
	u_int32_t flags;
} JOIN_CURSOR;

#endif /* !_DB_JOIN_H_ */
