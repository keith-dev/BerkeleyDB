/* DO NOT EDIT: automatically built by dist/distrib. */
#ifndef _btree_ext_h_
#define _btree_ext_h_
int __bam_cmp __P((DB *, const DBT *,
   PAGE *, u_int32_t, int (*)(const DBT *, const DBT *)));
int __bam_defcmp __P((const DBT *, const DBT *));
size_t __bam_defpfx __P((const DBT *, const DBT *));
int __bam_pgin __P((db_pgno_t, void *, DBT *));
int __bam_pgout __P((db_pgno_t, void *, DBT *));
int __bam_mswap __P((PAGE *));
int __bam_cprint __P((DB *));
int __bam_ca_delete __P((DB *, db_pgno_t, u_int32_t, int));
void __bam_ca_di __P((DB *, db_pgno_t, u_int32_t, int));
void __bam_ca_dup __P((DB *,
   db_pgno_t, u_int32_t, u_int32_t, db_pgno_t, u_int32_t));
void __bam_ca_rsplit __P((DB *, db_pgno_t, db_pgno_t));
void __bam_ca_split __P((DB *,
   db_pgno_t, db_pgno_t, db_pgno_t, u_int32_t, int));
void __bam_ca_repl __P((DB *,
   db_pgno_t, u_int32_t, db_pgno_t, u_int32_t));
int __bam_c_init __P((DBC *));
int __bam_c_dup __P((DBC *, DBC *));
int __bam_delete __P((DB *, DB_TXN *, DBT *, u_int32_t));
int __bam_ditem __P((DBC *, PAGE *, u_int32_t));
int __bam_adjindx __P((DBC *, PAGE *, u_int32_t, u_int32_t, int));
int __bam_dpage __P((DBC *, const DBT *));
int __bam_dpages __P((DBC *));
int __bam_db_create __P((DB *));
int __bam_db_close __P((DB *));
int __bam_set_flags __P((DB *, u_int32_t *flagsp));
int __ram_set_flags __P((DB *, u_int32_t *flagsp));
int __bam_open __P((DB *, const char *, db_pgno_t));
void __bam_setovflsize __P((DB *));
int __bam_metachk __P((DB *, const char *, BTMETA *));
int __bam_read_root __P((DB *, const char *, db_pgno_t));
int __bam_iitem __P((DBC *,
   PAGE **, db_indx_t *, DBT *, DBT *, u_int32_t, u_int32_t));
u_int32_t __bam_partsize __P((u_int32_t, DBT *, PAGE *, u_int32_t));
int __bam_build __P((DBC *, u_int32_t,
    DBT *, PAGE *, u_int32_t, u_int32_t));
int __bam_ritem __P((DBC *, PAGE *, u_int32_t, DBT *));
int __bam_pg_alloc_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_pg_free_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_split_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_rsplit_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_adj_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_cadjust_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_cdel_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_repl_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_root_recover
  __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __bam_reclaim __P((DB *, DB_TXN *));
int __ram_open __P((DB *, const char *, db_pgno_t));
int __ram_c_del __P((DBC *, u_int32_t));
int __ram_c_get __P((DBC *, DBT *, DBT *, u_int32_t));
int __ram_c_put __P((DBC *, DBT *, DBT *, u_int32_t));
void __ram_ca __P((DB *, db_recno_t, ca_recno_arg));
int __ram_getno __P((DBC *, const DBT *, db_recno_t *, int));
int __ram_writeback __P((DB *));
int __bam_rsearch __P((DBC *, db_recno_t *, u_int32_t, int, int *));
int __bam_adjust __P((DBC *, int32_t));
int __bam_nrecs __P((DBC *, db_recno_t *));
db_recno_t __bam_total __P((PAGE *));
int __bam_search __P((DBC *,
    const DBT *, u_int32_t, int, db_recno_t *, int *));
int __bam_stkrel __P((DBC *, int));
int __bam_stkgrow __P((BTREE_CURSOR *));
int __bam_split __P((DBC *, void *));
int __bam_copy __P((DB *, PAGE *, PAGE *, u_int32_t, u_int32_t));
int __bam_stat __P((DB *, void *, void *(*)(size_t), u_int32_t));
int __bam_traverse __P((DBC *, db_lockmode_t,
    db_pgno_t, int (*)(DB *, PAGE *, void *, int *), void *));
int __bam_upgrade __P((DB *, int, char *, DB_FH *, char *));
int __bam_init_recover __P((DB_ENV *));
#endif /* _btree_ext_h_ */
