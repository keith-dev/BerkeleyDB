/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_rpc_client_ext_h_
#define	_rpc_client_ext_h_
#if defined(__cplusplus)
extern "C" {
#endif
int __dbcl_envrpcserver __P((DB_ENV *, void *, const char *, long, long, u_int32_t));
int __dbcl_env_open_wrap __P((DB_ENV *, const char *, u_int32_t, int));
int __dbcl_refresh __P((DB_ENV *));
void __dbcl_txn_end __P((DB_TXN *));
void __dbcl_txn_setup __P((DB_ENV *, DB_TXN *, DB_TXN *, u_int32_t));
void __dbcl_c_refresh __P((DBC *));
int __dbcl_c_setup __P((long, DB *, DBC **));
int __dbcl_retcopy __P((DB_ENV *, DBT *, void *, u_int32_t));
int __dbcl_dbclose_common __P((DB *));
int __dbcl_env_alloc __P((DB_ENV *, void *(*)(size_t), void *(*)(void *, size_t), void (*)(void *)));
int __dbcl_env_cachesize __P((DB_ENV *, u_int32_t, u_int32_t, int));
int __dbcl_env_close __P((DB_ENV *, u_int32_t));
int __dbcl_env_create __P((DB_ENV *, long));
int __dbcl_set_data_dir __P((DB_ENV *, const char *));
int __dbcl_env_set_feedback __P((DB_ENV *, void (*)(DB_ENV *, int, int)));
int __dbcl_env_flags __P((DB_ENV *, u_int32_t, int));
int __dbcl_set_lg_bsize __P((DB_ENV *, u_int32_t));
int __dbcl_set_lg_dir __P((DB_ENV *, const char *));
int __dbcl_set_lg_max __P((DB_ENV *, u_int32_t));
int __dbcl_set_lg_regionmax __P((DB_ENV *, u_int32_t));
int __dbcl_set_lk_conflict __P((DB_ENV *, u_int8_t *, int));
int __dbcl_set_lk_detect __P((DB_ENV *, u_int32_t));
int __dbcl_set_lk_max __P((DB_ENV *, u_int32_t));
int __dbcl_set_lk_max_locks __P((DB_ENV *, u_int32_t));
int __dbcl_set_lk_max_lockers __P((DB_ENV *, u_int32_t));
int __dbcl_set_lk_max_objects __P((DB_ENV *, u_int32_t));
int __dbcl_set_mp_mmapsize __P((DB_ENV *, size_t));
int __dbcl_env_open __P((DB_ENV *, const char *, u_int32_t, int));
int __dbcl_env_paniccall __P((DB_ENV *, void (*)(DB_ENV *, int)));
int __dbcl_set_recovery_init __P((DB_ENV *, int (*)(DB_ENV *)));
int __dbcl_env_remove __P((DB_ENV *, const char *, u_int32_t));
int __dbcl_set_shm_key __P((DB_ENV *, long));
int __dbcl_set_tas_spins __P((DB_ENV *, u_int32_t));
int __dbcl_set_timeout __P((DB_ENV *, u_int32_t, u_int32_t));
int __dbcl_set_tmp_dir __P((DB_ENV *, const char *));
int __dbcl_set_tx_recover __P((DB_ENV *, int (*)(DB_ENV *, DBT *, DB_LSN *, db_recops)));
int __dbcl_set_tx_max __P((DB_ENV *, u_int32_t));
int __dbcl_set_tx_timestamp __P((DB_ENV *, time_t *));
int __dbcl_set_verbose __P((DB_ENV *, u_int32_t, int));
int __dbcl_txn_abort __P((DB_TXN *));
int __dbcl_txn_begin __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t));
int __dbcl_txn_checkpoint __P((DB_ENV *, u_int32_t, u_int32_t, u_int32_t));
int __dbcl_txn_commit __P((DB_TXN *, u_int32_t));
int __dbcl_txn_discard __P((DB_TXN *, u_int32_t));
int __dbcl_txn_prepare __P((DB_TXN *, u_int8_t *));
int __dbcl_txn_recover __P((DB_ENV *, DB_PREPLIST *, long, long *, u_int32_t));
int __dbcl_txn_stat __P((DB_ENV *, DB_TXN_STAT **, u_int32_t));
int __dbcl_txn_timeout __P((DB_TXN *, u_int32_t, u_int32_t));
int __dbcl_rep_elect __P((DB_ENV *, int, int, u_int32_t, int *));
int __dbcl_rep_process_message __P((DB_ENV *, DBT *, DBT *, int *));
int __dbcl_rep_set_rep_transport __P((DB_ENV *, int, int (*)(DB_ENV *, const DBT *, const DBT *, int, u_int32_t)));
int __dbcl_rep_start __P((DB_ENV *, DBT *, u_int32_t));
int __dbcl_db_alloc __P((DB *, void *(*)(size_t), void *(*)(void *, size_t), void (*)(void *)));
int __dbcl_db_associate __P((DB *, DB *, int (*)(DB *, const DBT *, const DBT *, DBT *), u_int32_t));
int __dbcl_db_bt_compare __P((DB *, int (*)(DB *, const DBT *, const DBT *)));
int __dbcl_db_bt_maxkey __P((DB *, u_int32_t));
int __dbcl_db_bt_minkey __P((DB *, u_int32_t));
int __dbcl_db_bt_prefix __P((DB *, size_t(*)(DB *, const DBT *, const DBT *)));
int __dbcl_db_set_append_recno __P((DB *, int (*)(DB *, DBT *, db_recno_t)));
int __dbcl_db_cachesize __P((DB *, u_int32_t, u_int32_t, int));
int __dbcl_db_close __P((DB *, u_int32_t));
int __dbcl_db_create __P((DB *, DB_ENV *, u_int32_t));
int __dbcl_db_del __P((DB *, DB_TXN *, DBT *, u_int32_t));
int __dbcl_db_dup_compare __P((DB *, int (*)(DB *, const DBT *, const DBT *)));
int __dbcl_db_extentsize __P((DB *, u_int32_t));
int __dbcl_db_fd __P((DB *, int *));
int __dbcl_db_feedback __P((DB *, void (*)(DB *, int, int)));
int __dbcl_db_flags __P((DB *, u_int32_t));
int __dbcl_db_get __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
int __dbcl_db_h_ffactor __P((DB *, u_int32_t));
int __dbcl_db_h_hash __P((DB *, u_int32_t(*)(DB *, const void *, u_int32_t)));
int __dbcl_db_h_nelem __P((DB *, u_int32_t));
int __dbcl_db_key_range __P((DB *, DB_TXN *, DBT *, DB_KEY_RANGE *, u_int32_t));
int __dbcl_db_lorder __P((DB *, int));
int __dbcl_db_open __P((DB *, const char *, const char *, DBTYPE, u_int32_t, int));
int __dbcl_db_pagesize __P((DB *, u_int32_t));
int __dbcl_db_panic __P((DB *, void (*)(DB_ENV *, int)));
int __dbcl_db_pget __P((DB *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t));
int __dbcl_db_put __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t));
int __dbcl_db_re_delim __P((DB *, int));
int __dbcl_db_re_len __P((DB *, u_int32_t));
int __dbcl_db_re_pad __P((DB *, int));
int __dbcl_db_re_source __P((DB *, const char *));
int __dbcl_db_remove __P((DB *, const char *, const char *, u_int32_t));
int __dbcl_db_rename __P((DB *, const char *, const char *, const char *, u_int32_t));
int __dbcl_db_stat __P((DB *, void *, u_int32_t));
int __dbcl_db_sync __P((DB *, u_int32_t));
int __dbcl_db_truncate __P((DB *, DB_TXN *, u_int32_t  *, u_int32_t));
int __dbcl_db_upgrade __P((DB *, const char *, u_int32_t));
int __dbcl_db_verify __P((DB *, const char *, const char *, FILE *, u_int32_t));
int __dbcl_db_cursor __P((DB *, DB_TXN *, DBC **, u_int32_t));
int __dbcl_db_join __P((DB *, DBC **, DBC **, u_int32_t));
int __dbcl_dbc_close __P((DBC *));
int __dbcl_dbc_count __P((DBC *, db_recno_t *, u_int32_t));
int __dbcl_dbc_del __P((DBC *, u_int32_t));
int __dbcl_dbc_dup __P((DBC *, DBC **, u_int32_t));
int __dbcl_dbc_get __P((DBC *, DBT *, DBT *, u_int32_t));
int __dbcl_dbc_pget __P((DBC *, DBT *, DBT *, DBT *, u_int32_t));
int __dbcl_dbc_put __P((DBC *, DBT *, DBT *, u_int32_t));
int __dbcl_lock_detect __P((DB_ENV *, u_int32_t, u_int32_t, int *));
int __dbcl_lock_get __P((DB_ENV *, u_int32_t, u_int32_t, const DBT *, db_lockmode_t, DB_LOCK *));
int __dbcl_lock_id __P((DB_ENV *, u_int32_t *));
int __dbcl_lock_id_free __P((DB_ENV *, u_int32_t));
int __dbcl_lock_put __P((DB_ENV *, DB_LOCK *));
int __dbcl_lock_stat __P((DB_ENV *, DB_LOCK_STAT **, u_int32_t));
int __dbcl_lock_vec __P((DB_ENV *, u_int32_t, u_int32_t, DB_LOCKREQ *, int, DB_LOCKREQ **));
int __dbcl_log_archive __P((DB_ENV *, char ***, u_int32_t));
int __dbcl_log_cursor __P((DB_ENV *, DB_LOGC **, u_int32_t));
int __dbcl_log_file __P((DB_ENV *, const DB_LSN *, char *, size_t));
int __dbcl_log_flush __P((DB_ENV *, const DB_LSN *));
int __dbcl_log_put __P((DB_ENV *, DB_LSN *, const DBT *, u_int32_t));
int __dbcl_log_register __P((DB_ENV *, DB *, const char *));
int __dbcl_log_stat __P((DB_ENV *, DB_LOG_STAT **, u_int32_t));
int __dbcl_log_unregister __P((DB_ENV *, DB *));
int __dbcl_memp_fcreate __P((DB_ENV *, DB_MPOOLFILE **, u_int32_t));
int __dbcl_memp_register __P((DB_ENV *, int, int (*)(DB_ENV *, db_pgno_t, void *, DBT *), int (*)(DB_ENV *, db_pgno_t, void *, DBT *)));
int __dbcl_memp_stat __P((DB_ENV *, DB_MPOOL_STAT **, DB_MPOOL_FSTAT ***, u_int32_t));
int __dbcl_memp_sync __P((DB_ENV *, DB_LSN *));
int __dbcl_memp_trickle __P((DB_ENV *, int, int *));
int __dbcl_env_close_ret __P((DB_ENV *, u_int32_t, __env_close_reply *));
int __dbcl_env_create_ret __P((DB_ENV *, long, __env_create_reply *));
int __dbcl_env_open_ret __P((DB_ENV *, const char *, u_int32_t, int, __env_open_reply *));
int __dbcl_env_remove_ret __P((DB_ENV *, const char *, u_int32_t, __env_remove_reply *));
int __dbcl_txn_abort_ret __P((DB_TXN *, __txn_abort_reply *));
int __dbcl_txn_begin_ret __P((DB_ENV *, DB_TXN *, DB_TXN **, u_int32_t, __txn_begin_reply *));
int __dbcl_txn_commit_ret __P((DB_TXN *, u_int32_t, __txn_commit_reply *));
int __dbcl_txn_discard_ret __P((DB_TXN *, u_int32_t, __txn_discard_reply *));
int __dbcl_txn_recover_ret __P((DB_ENV *, DB_PREPLIST *, long, long *, u_int32_t, __txn_recover_reply *));
int __dbcl_db_close_ret __P((DB *, u_int32_t, __db_close_reply *));
int __dbcl_db_create_ret __P((DB *, DB_ENV *, u_int32_t, __db_create_reply *));
int __dbcl_db_get_ret __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t, __db_get_reply *));
int __dbcl_db_key_range_ret __P((DB *, DB_TXN *, DBT *, DB_KEY_RANGE *, u_int32_t, __db_key_range_reply *));
int __dbcl_db_open_ret __P((DB *, const char *, const char *, DBTYPE, u_int32_t, int, __db_open_reply *));
int __dbcl_db_pget_ret __P((DB *, DB_TXN *, DBT *, DBT *, DBT *, u_int32_t, __db_pget_reply *));
int __dbcl_db_put_ret __P((DB *, DB_TXN *, DBT *, DBT *, u_int32_t, __db_put_reply *));
int __dbcl_db_remove_ret __P((DB *, const char *, const char *, u_int32_t, __db_remove_reply *));
int __dbcl_db_rename_ret __P((DB *, const char *, const char *, const char *, u_int32_t, __db_rename_reply *));
int __dbcl_db_stat_ret __P((DB *, void *, u_int32_t, __db_stat_reply *));
int __dbcl_db_truncate_ret __P((DB *, DB_TXN *, u_int32_t  *, u_int32_t, __db_truncate_reply *));
int __dbcl_db_cursor_ret __P((DB *, DB_TXN *, DBC **, u_int32_t, __db_cursor_reply *));
int __dbcl_db_join_ret __P((DB *, DBC **, DBC **, u_int32_t, __db_join_reply *));
int __dbcl_dbc_close_ret __P((DBC *, __dbc_close_reply *));
int __dbcl_dbc_count_ret __P((DBC *, db_recno_t *, u_int32_t, __dbc_count_reply *));
int __dbcl_dbc_dup_ret __P((DBC *, DBC **, u_int32_t, __dbc_dup_reply *));
int __dbcl_dbc_get_ret __P((DBC *, DBT *, DBT *, u_int32_t, __dbc_get_reply *));
int __dbcl_dbc_pget_ret __P((DBC *, DBT *, DBT *, DBT *, u_int32_t, __dbc_pget_reply *));
int __dbcl_dbc_put_ret __P((DBC *, DBT *, DBT *, u_int32_t, __dbc_put_reply *));
#if defined(__cplusplus)
}
#endif
#endif /* _rpc_client_ext_h_ */
