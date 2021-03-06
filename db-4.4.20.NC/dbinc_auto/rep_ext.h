/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_rep_ext_h_
#define	_rep_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __rep_update_buf __P((u_int8_t *, size_t, size_t *, DB_LSN *, u_int32_t));
int __rep_update_read __P((DB_ENV *, void *, void **, __rep_update_args **));
int __rep_fileinfo_buf __P((u_int8_t *, size_t, size_t *, u_int32_t, db_pgno_t, db_pgno_t, u_int32_t, int32_t, u_int32_t, u_int32_t, const DBT *, const DBT *));
int __rep_fileinfo_read __P((DB_ENV *, void *, void **, __rep_fileinfo_args **));
int __rep_update_req __P((DB_ENV *, int));
int __rep_page_req __P((DB_ENV *, int, DBT *));
int __rep_update_setup __P((DB_ENV *, int, REP_CONTROL *, DBT *));
int __rep_bulk_page __P((DB_ENV *, int, REP_CONTROL *, DBT *));
int __rep_page __P((DB_ENV *, int, REP_CONTROL *, DBT *));
int __rep_page_fail __P((DB_ENV *, int, DBT *));
int __rep_init_cleanup __P((DB_ENV *, REP *, int));
int __rep_pggap_req __P((DB_ENV *, REP *, __rep_fileinfo_args *, u_int32_t));
int __rep_finfo_alloc __P((DB_ENV *, __rep_fileinfo_args *, __rep_fileinfo_args **));
int __rep_elect __P((DB_ENV *, int, int, int, u_int32_t, int *, u_int32_t));
int __rep_vote1 __P((DB_ENV *, REP_CONTROL *, DBT *, int));
int __rep_vote2 __P((DB_ENV *, DBT *, int *));
void __rep_elect_master __P((DB_ENV *, REP *, int *));
int __rep_allreq __P((DB_ENV *, REP_CONTROL *, int));
int __rep_log __P((DB_ENV *, REP_CONTROL *, DBT *, time_t, DB_LSN *));
int __rep_bulk_log __P((DB_ENV *, REP_CONTROL *, DBT *, time_t, DB_LSN *));
int __rep_logreq __P((DB_ENV *, REP_CONTROL *, DBT *, int));
int __rep_loggap_req __P((DB_ENV *, REP *, DB_LSN *, u_int32_t));
int __rep_open __P((DB_ENV *));
int __rep_get_config __P((DB_ENV *, u_int32_t, int *));
int __rep_set_config __P((DB_ENV *, u_int32_t, int));
int __rep_start __P((DB_ENV *, DBT *, u_int32_t));
int __rep_client_dbinit __P((DB_ENV *, int, repdb_t));
int __rep_get_limit __P((DB_ENV *, u_int32_t *, u_int32_t *));
int __rep_set_limit __P((DB_ENV *, u_int32_t, u_int32_t));
int __rep_set_request __P((DB_ENV *, u_int32_t, u_int32_t));
int __rep_set_rep_transport __P((DB_ENV *, int, int (*)(DB_ENV *, const DBT *, const DBT *, const DB_LSN *, int, u_int32_t)));
int __rep_flush __P((DB_ENV *));
int __rep_sync __P((DB_ENV *, u_int32_t));
int __rep_process_message __P((DB_ENV *, DBT *, DBT *, int *, DB_LSN *));
int __rep_apply __P((DB_ENV *, REP_CONTROL *, DBT *, DB_LSN *, int *));
int __rep_process_txn __P((DB_ENV *, DBT *));
int __rep_check_doreq __P((DB_ENV *, REP *));
int __rep_region_init __P((DB_ENV *));
int __rep_region_destroy __P((DB_ENV *));
void __rep_dbenv_refresh __P((DB_ENV *));
int __rep_dbenv_close __P((DB_ENV *));
int __rep_preclose __P((DB_ENV *));
int __rep_write_egen __P((DB_ENV *, u_int32_t));
int __rep_stat_pp __P((DB_ENV *, DB_REP_STAT **, u_int32_t));
int __rep_stat_print_pp __P((DB_ENV *, u_int32_t));
int __rep_stat_print __P((DB_ENV *, u_int32_t));
int __rep_bulk_message __P((DB_ENV *, REP_BULK *, REP_THROTTLE *, DB_LSN *, const DBT *, u_int32_t));
int __rep_send_bulk __P((DB_ENV *, REP_BULK *, u_int32_t));
int __rep_bulk_alloc __P((DB_ENV *, REP_BULK *, int, uintptr_t *, u_int32_t *, u_int32_t));
int __rep_bulk_free __P((DB_ENV *, REP_BULK *, u_int32_t));
int __rep_send_message __P((DB_ENV *, int, u_int32_t, DB_LSN *, const DBT *, u_int32_t, u_int32_t));
int __rep_new_master __P((DB_ENV *, REP_CONTROL *, int));
int __rep_is_client __P((DB_ENV *));
int __rep_noarchive __P((DB_ENV *));
void __rep_send_vote __P((DB_ENV *, DB_LSN *, int, int, int, u_int32_t, u_int32_t, int, u_int32_t));
void __rep_elect_done __P((DB_ENV *, REP *));
int __rep_grow_sites __P((DB_ENV *dbenv, int nsites));
int __env_rep_enter __P((DB_ENV *, int));
int __env_db_rep_exit __P((DB_ENV *));
int __db_rep_enter __P((DB *, int, int, int));
int __op_rep_enter __P((DB_ENV *));
int __op_rep_exit __P((DB_ENV *));
int __rep_get_gen __P((DB_ENV *, u_int32_t *));
int __rep_lockout __P((DB_ENV *, REP *, u_int32_t));
int __rep_send_throttle __P((DB_ENV *, int, REP_THROTTLE *, u_int32_t));
void __rep_print_message __P((DB_ENV *, int, REP_CONTROL *, char *));
int __rep_verify __P((DB_ENV *, REP_CONTROL *, DBT *, int, time_t));
int __rep_verify_fail __P((DB_ENV *, REP_CONTROL *, int));
int __rep_verify_req __P((DB_ENV *, REP_CONTROL *, int));
int __rep_verify_match __P((DB_ENV *, DB_LSN *, time_t));
int __rep_log_backup __P((DB_LOGC *, DB_LSN *));

#if defined(__cplusplus)
}
#endif
#endif /* !_rep_ext_h_ */
