/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_qam_ext_h_
#define	_qam_ext_h_
#if defined(__cplusplus)
extern "C" {
#endif
int __qam_position __P((DBC *, db_recno_t *, qam_position_mode, int *));
int __qam_pitem __P((DBC *,  QPAGE *, u_int32_t, db_recno_t, DBT *));
int __qam_append __P((DBC *, DBT *, DBT *));
int __qam_c_dup __P((DBC *, DBC *));
int __qam_c_init __P((DBC *));
int __qam_truncate __P((DB *, DB_TXN *, u_int32_t *));
int __qam_incfirst_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, db_recno_t, db_pgno_t));
int __qam_incfirst_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_incfirst_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_incfirst_read __P((DB_ENV *, void *, __qam_incfirst_args **));
int __qam_mvptr_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, int32_t, db_recno_t, db_recno_t, db_recno_t, db_recno_t, DB_LSN *, db_pgno_t));
int __qam_mvptr_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_mvptr_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_mvptr_read __P((DB_ENV *, void *, __qam_mvptr_args **));
int __qam_del_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t));
int __qam_del_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_del_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_del_read __P((DB_ENV *, void *, __qam_del_args **));
int __qam_add_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t, const DBT *, u_int32_t, const DBT *));
int __qam_add_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_add_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_add_read __P((DB_ENV *, void *, __qam_add_args **));
int __qam_delete_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, const DBT *, DB_LSN *));
int __qam_delete_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delete_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delete_read __P((DB_ENV *, void *, __qam_delete_args **));
int __qam_rename_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, const DBT *, const DBT *));
int __qam_rename_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_rename_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_rename_read __P((DB_ENV *, void *, __qam_rename_args **));
int __qam_delext_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t, const DBT *));
int __qam_delext_getpgnos __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delext_print __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delext_read __P((DB_ENV *, void *, __qam_delext_args **));
int __qam_init_print __P((DB_ENV *, int (***)(DB_ENV *, DBT *, DB_LSN *, db_recops, void *), size_t *));
int __qam_init_getpgnos __P((DB_ENV *, int (***)(DB_ENV *, DBT *, DB_LSN *, db_recops, void *), size_t *));
int __qam_init_recover __P((DB_ENV *));
int __qam_mswap __P((PAGE *));
int __qam_pgin_out __P((DB_ENV *, db_pgno_t, void *, DBT *));
int __qam_fprobe __P((DB *, db_pgno_t, void *, qam_probe_mode, u_int32_t));
int __qam_fclose __P((DB *, db_pgno_t));
int __qam_fremove __P((DB *, db_pgno_t));
int __qam_sync __P((DB *, u_int32_t));
int __qam_gen_filelist __P(( DB *, QUEUE_FILELIST **));
int __qam_extent_names __P((DB_ENV *, char *, char ***));
int __qam_db_create __P((DB *));
int __qam_db_close __P((DB *));
int __db_prqueue __P((DB *, u_int32_t));
int __qam_remove __P((DB *, const char *, const char *, DB_LSN *, int (**)(DB *, void*), void **));
int __qam_rename __P((DB *, const char *, const char *, const char *));
int __qam_open __P((DB *, const char *, db_pgno_t, int, u_int32_t));
int __qam_metachk __P((DB *, const char *, QMETA *));
int __qam_incfirst_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_mvptr_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_del_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delext_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_add_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_delete_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_rename_recover __P((DB_ENV *, DBT *, DB_LSN *, db_recops, void *));
int __qam_stat __P((DB *, void *, u_int32_t));
int __qam_31_qammeta __P((DB *, char *, u_int8_t *));
int __qam_32_qammeta __P((DB *, char *, u_int8_t *));
int __qam_vrfy_meta __P((DB *, VRFY_DBINFO *, QMETA *, db_pgno_t, u_int32_t));
int __qam_vrfy_data __P((DB *, VRFY_DBINFO *, QPAGE *, db_pgno_t, u_int32_t));
int __qam_vrfy_structure __P((DB *, VRFY_DBINFO *, u_int32_t));
#if defined(__cplusplus)
}
#endif
#endif /* _qam_ext_h_ */
