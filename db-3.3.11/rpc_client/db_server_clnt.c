#include "db_config.h"
#ifdef HAVE_RPC
/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "db_server.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

__env_cachesize_reply *
__db_env_cachesize_3003(argp, clnt)
	__env_cachesize_msg *argp;
	CLIENT *clnt;
{
	static __env_cachesize_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_env_cachesize,
		(xdrproc_t) xdr___env_cachesize_msg, (caddr_t) argp,
		(xdrproc_t) xdr___env_cachesize_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__env_close_reply *
__db_env_close_3003(argp, clnt)
	__env_close_msg *argp;
	CLIENT *clnt;
{
	static __env_close_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_env_close,
		(xdrproc_t) xdr___env_close_msg, (caddr_t) argp,
		(xdrproc_t) xdr___env_close_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__env_create_reply *
__db_env_create_3003(argp, clnt)
	__env_create_msg *argp;
	CLIENT *clnt;
{
	static __env_create_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_env_create,
		(xdrproc_t) xdr___env_create_msg, (caddr_t) argp,
		(xdrproc_t) xdr___env_create_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__env_flags_reply *
__db_env_flags_3003(argp, clnt)
	__env_flags_msg *argp;
	CLIENT *clnt;
{
	static __env_flags_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_env_flags,
		(xdrproc_t) xdr___env_flags_msg, (caddr_t) argp,
		(xdrproc_t) xdr___env_flags_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__env_open_reply *
__db_env_open_3003(argp, clnt)
	__env_open_msg *argp;
	CLIENT *clnt;
{
	static __env_open_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_env_open,
		(xdrproc_t) xdr___env_open_msg, (caddr_t) argp,
		(xdrproc_t) xdr___env_open_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__env_remove_reply *
__db_env_remove_3003(argp, clnt)
	__env_remove_msg *argp;
	CLIENT *clnt;
{
	static __env_remove_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_env_remove,
		(xdrproc_t) xdr___env_remove_msg, (caddr_t) argp,
		(xdrproc_t) xdr___env_remove_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__txn_abort_reply *
__db_txn_abort_3003(argp, clnt)
	__txn_abort_msg *argp;
	CLIENT *clnt;
{
	static __txn_abort_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_txn_abort,
		(xdrproc_t) xdr___txn_abort_msg, (caddr_t) argp,
		(xdrproc_t) xdr___txn_abort_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__txn_begin_reply *
__db_txn_begin_3003(argp, clnt)
	__txn_begin_msg *argp;
	CLIENT *clnt;
{
	static __txn_begin_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_txn_begin,
		(xdrproc_t) xdr___txn_begin_msg, (caddr_t) argp,
		(xdrproc_t) xdr___txn_begin_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__txn_commit_reply *
__db_txn_commit_3003(argp, clnt)
	__txn_commit_msg *argp;
	CLIENT *clnt;
{
	static __txn_commit_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_txn_commit,
		(xdrproc_t) xdr___txn_commit_msg, (caddr_t) argp,
		(xdrproc_t) xdr___txn_commit_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__txn_discard_reply *
__db_txn_discard_3003(argp, clnt)
	__txn_discard_msg *argp;
	CLIENT *clnt;
{
	static __txn_discard_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_txn_discard,
		(xdrproc_t) xdr___txn_discard_msg, (caddr_t) argp,
		(xdrproc_t) xdr___txn_discard_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__txn_prepare_reply *
__db_txn_prepare_3003(argp, clnt)
	__txn_prepare_msg *argp;
	CLIENT *clnt;
{
	static __txn_prepare_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_txn_prepare,
		(xdrproc_t) xdr___txn_prepare_msg, (caddr_t) argp,
		(xdrproc_t) xdr___txn_prepare_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__txn_recover_reply *
__db_txn_recover_3003(argp, clnt)
	__txn_recover_msg *argp;
	CLIENT *clnt;
{
	static __txn_recover_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_txn_recover,
		(xdrproc_t) xdr___txn_recover_msg, (caddr_t) argp,
		(xdrproc_t) xdr___txn_recover_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_associate_reply *
__db_db_associate_3003(argp, clnt)
	__db_associate_msg *argp;
	CLIENT *clnt;
{
	static __db_associate_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_associate,
		(xdrproc_t) xdr___db_associate_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_associate_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_bt_maxkey_reply *
__db_db_bt_maxkey_3003(argp, clnt)
	__db_bt_maxkey_msg *argp;
	CLIENT *clnt;
{
	static __db_bt_maxkey_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_bt_maxkey,
		(xdrproc_t) xdr___db_bt_maxkey_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_bt_maxkey_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_bt_minkey_reply *
__db_db_bt_minkey_3003(argp, clnt)
	__db_bt_minkey_msg *argp;
	CLIENT *clnt;
{
	static __db_bt_minkey_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_bt_minkey,
		(xdrproc_t) xdr___db_bt_minkey_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_bt_minkey_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_close_reply *
__db_db_close_3003(argp, clnt)
	__db_close_msg *argp;
	CLIENT *clnt;
{
	static __db_close_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_close,
		(xdrproc_t) xdr___db_close_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_close_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_create_reply *
__db_db_create_3003(argp, clnt)
	__db_create_msg *argp;
	CLIENT *clnt;
{
	static __db_create_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_create,
		(xdrproc_t) xdr___db_create_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_create_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_del_reply *
__db_db_del_3003(argp, clnt)
	__db_del_msg *argp;
	CLIENT *clnt;
{
	static __db_del_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_del,
		(xdrproc_t) xdr___db_del_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_del_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_extentsize_reply *
__db_db_extentsize_3003(argp, clnt)
	__db_extentsize_msg *argp;
	CLIENT *clnt;
{
	static __db_extentsize_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_extentsize,
		(xdrproc_t) xdr___db_extentsize_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_extentsize_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_flags_reply *
__db_db_flags_3003(argp, clnt)
	__db_flags_msg *argp;
	CLIENT *clnt;
{
	static __db_flags_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_flags,
		(xdrproc_t) xdr___db_flags_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_flags_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_get_reply *
__db_db_get_3003(argp, clnt)
	__db_get_msg *argp;
	CLIENT *clnt;
{
	static __db_get_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_get,
		(xdrproc_t) xdr___db_get_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_get_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_h_ffactor_reply *
__db_db_h_ffactor_3003(argp, clnt)
	__db_h_ffactor_msg *argp;
	CLIENT *clnt;
{
	static __db_h_ffactor_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_h_ffactor,
		(xdrproc_t) xdr___db_h_ffactor_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_h_ffactor_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_h_nelem_reply *
__db_db_h_nelem_3003(argp, clnt)
	__db_h_nelem_msg *argp;
	CLIENT *clnt;
{
	static __db_h_nelem_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_h_nelem,
		(xdrproc_t) xdr___db_h_nelem_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_h_nelem_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_key_range_reply *
__db_db_key_range_3003(argp, clnt)
	__db_key_range_msg *argp;
	CLIENT *clnt;
{
	static __db_key_range_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_key_range,
		(xdrproc_t) xdr___db_key_range_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_key_range_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_lorder_reply *
__db_db_lorder_3003(argp, clnt)
	__db_lorder_msg *argp;
	CLIENT *clnt;
{
	static __db_lorder_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_lorder,
		(xdrproc_t) xdr___db_lorder_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_lorder_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_open_reply *
__db_db_open_3003(argp, clnt)
	__db_open_msg *argp;
	CLIENT *clnt;
{
	static __db_open_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_open,
		(xdrproc_t) xdr___db_open_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_open_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_pagesize_reply *
__db_db_pagesize_3003(argp, clnt)
	__db_pagesize_msg *argp;
	CLIENT *clnt;
{
	static __db_pagesize_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_pagesize,
		(xdrproc_t) xdr___db_pagesize_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_pagesize_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_pget_reply *
__db_db_pget_3003(argp, clnt)
	__db_pget_msg *argp;
	CLIENT *clnt;
{
	static __db_pget_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_pget,
		(xdrproc_t) xdr___db_pget_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_pget_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_put_reply *
__db_db_put_3003(argp, clnt)
	__db_put_msg *argp;
	CLIENT *clnt;
{
	static __db_put_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_put,
		(xdrproc_t) xdr___db_put_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_put_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_re_delim_reply *
__db_db_re_delim_3003(argp, clnt)
	__db_re_delim_msg *argp;
	CLIENT *clnt;
{
	static __db_re_delim_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_re_delim,
		(xdrproc_t) xdr___db_re_delim_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_re_delim_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_re_len_reply *
__db_db_re_len_3003(argp, clnt)
	__db_re_len_msg *argp;
	CLIENT *clnt;
{
	static __db_re_len_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_re_len,
		(xdrproc_t) xdr___db_re_len_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_re_len_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_re_pad_reply *
__db_db_re_pad_3003(argp, clnt)
	__db_re_pad_msg *argp;
	CLIENT *clnt;
{
	static __db_re_pad_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_re_pad,
		(xdrproc_t) xdr___db_re_pad_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_re_pad_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_remove_reply *
__db_db_remove_3003(argp, clnt)
	__db_remove_msg *argp;
	CLIENT *clnt;
{
	static __db_remove_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_remove,
		(xdrproc_t) xdr___db_remove_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_remove_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_rename_reply *
__db_db_rename_3003(argp, clnt)
	__db_rename_msg *argp;
	CLIENT *clnt;
{
	static __db_rename_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_rename,
		(xdrproc_t) xdr___db_rename_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_rename_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_stat_reply *
__db_db_stat_3003(argp, clnt)
	__db_stat_msg *argp;
	CLIENT *clnt;
{
	static __db_stat_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_stat,
		(xdrproc_t) xdr___db_stat_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_stat_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_sync_reply *
__db_db_sync_3003(argp, clnt)
	__db_sync_msg *argp;
	CLIENT *clnt;
{
	static __db_sync_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_sync,
		(xdrproc_t) xdr___db_sync_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_sync_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_truncate_reply *
__db_db_truncate_3003(argp, clnt)
	__db_truncate_msg *argp;
	CLIENT *clnt;
{
	static __db_truncate_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_truncate,
		(xdrproc_t) xdr___db_truncate_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_truncate_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_cursor_reply *
__db_db_cursor_3003(argp, clnt)
	__db_cursor_msg *argp;
	CLIENT *clnt;
{
	static __db_cursor_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_cursor,
		(xdrproc_t) xdr___db_cursor_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_cursor_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__db_join_reply *
__db_db_join_3003(argp, clnt)
	__db_join_msg *argp;
	CLIENT *clnt;
{
	static __db_join_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_db_join,
		(xdrproc_t) xdr___db_join_msg, (caddr_t) argp,
		(xdrproc_t) xdr___db_join_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_close_reply *
__db_dbc_close_3003(argp, clnt)
	__dbc_close_msg *argp;
	CLIENT *clnt;
{
	static __dbc_close_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_close,
		(xdrproc_t) xdr___dbc_close_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_close_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_count_reply *
__db_dbc_count_3003(argp, clnt)
	__dbc_count_msg *argp;
	CLIENT *clnt;
{
	static __dbc_count_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_count,
		(xdrproc_t) xdr___dbc_count_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_count_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_del_reply *
__db_dbc_del_3003(argp, clnt)
	__dbc_del_msg *argp;
	CLIENT *clnt;
{
	static __dbc_del_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_del,
		(xdrproc_t) xdr___dbc_del_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_del_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_dup_reply *
__db_dbc_dup_3003(argp, clnt)
	__dbc_dup_msg *argp;
	CLIENT *clnt;
{
	static __dbc_dup_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_dup,
		(xdrproc_t) xdr___dbc_dup_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_dup_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_get_reply *
__db_dbc_get_3003(argp, clnt)
	__dbc_get_msg *argp;
	CLIENT *clnt;
{
	static __dbc_get_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_get,
		(xdrproc_t) xdr___dbc_get_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_get_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_pget_reply *
__db_dbc_pget_3003(argp, clnt)
	__dbc_pget_msg *argp;
	CLIENT *clnt;
{
	static __dbc_pget_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_pget,
		(xdrproc_t) xdr___dbc_pget_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_pget_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}

__dbc_put_reply *
__db_dbc_put_3003(argp, clnt)
	__dbc_put_msg *argp;
	CLIENT *clnt;
{
	static __dbc_put_reply clnt_res;

	memset((char *)&clnt_res, 0, sizeof (clnt_res));
	if (clnt_call(clnt, __DB_dbc_put,
		(xdrproc_t) xdr___dbc_put_msg, (caddr_t) argp,
		(xdrproc_t) xdr___dbc_put_reply, (caddr_t) &clnt_res,
		TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&clnt_res);
}
#endif /* HAVE_RPC */
