/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_txn_ext_h_
#define	_txn_ext_h_
#if defined(__cplusplus)
extern "C" {
#endif
int __txn_xa_begin __P((DB_ENV *, DB_TXN *));
int __txn_compensate_begin __P((DB_ENV *, DB_TXN **txnp));
int __txn_activekids __P((DB_ENV *, u_int32_t, DB_TXN *));
void __txn_force_abort __P((u_int8_t *));
void __txn_preclose __P((DB_ENV *));
int __txn_old_regop_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_old_regop_read __P((DB_ENV *, void *, __txn_old_regop_args **));
int __txn_regop_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, int32_t));
int __txn_regop_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_regop_read __P((DB_ENV *, void *, __txn_regop_args **));
int __txn_old_ckp_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_old_ckp_read __P((DB_ENV *, void *, __txn_old_ckp_args **));
int __txn_ckp_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, DB_LSN *, DB_LSN *, int32_t));
int __txn_ckp_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_ckp_read __P((DB_ENV *, void *, __txn_ckp_args **));
int __txn_xa_regop_old_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_xa_regop_old_read __P((DB_ENV *, void *, __txn_xa_regop_old_args **));
int __txn_xa_regop_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, const DBT *, int32_t, u_int32_t, u_int32_t, DB_LSN *));
int __txn_xa_regop_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_xa_regop_read __P((DB_ENV *, void *, __txn_xa_regop_args **));
int __txn_child_old_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_child_old_read __P((DB_ENV *, void *, __txn_child_old_args **));
int __txn_child_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, DB_LSN *));
int __txn_child_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_child_read __P((DB_ENV *, void *, __txn_child_args **));
int __txn_init_print __P((DB_ENV *));
int __txn_init_recover __P((DB_ENV *));
int __txn_regop_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_xa_regop_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_ckp_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __txn_child_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
void __txn_continue __P((DB_ENV *, DB_TXN *, TXN_DETAIL *, size_t));
int __txn_map_gid __P((DB_ENV *, u_int8_t *, TXN_DETAIL **, size_t *));
int __txn_get_prepared __P((DB_ENV *, XID *, DB_PREPLIST *, long, long *, u_int32_t));
void __txn_dbenv_create __P((DB_ENV *));
int __txn_open __P((DB_ENV *));
int __txn_close __P((DB_ENV *));
void __txn_region_destroy __P((DB_ENV *, REGINFO *));
#if defined(__cplusplus)
}
#endif
#endif /* _txn_ext_h_ */
