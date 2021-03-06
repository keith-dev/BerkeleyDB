/* DO NOT EDIT: automatically built by dist/distrib. */
#ifndef _mp_ext_h_
#define _mp_ext_h_
int __memp_alloc __P((DB_MPOOL *,
    REGINFO *, MPOOLFILE *, size_t, roff_t *, void *));
int __memp_bhwrite
    __P((DB_MPOOL *, MPOOLFILE *, BH *, int *, int *));
int __memp_pgread __P((DB_MPOOLFILE *, BH *, int));
int __memp_pgwrite
    __P((DB_MPOOL *, DB_MPOOLFILE *, BH *, int *, int *));
int __memp_pg __P((DB_MPOOLFILE *, BH *, int));
void __memp_bhfree __P((DB_MPOOL *, BH *, int));
int __memp_fopen __P((DB_MPOOL *, MPOOLFILE *, const char *,
   u_int32_t, int, size_t, int, DB_MPOOL_FINFO *, DB_MPOOLFILE **));
int __memp_fremove __P((DB_MPOOLFILE *));
char * __memp_fn __P((DB_MPOOLFILE *));
char * __memp_fns __P((DB_MPOOL *, MPOOLFILE *));
void __memp_dbenv_create __P((DB_ENV *));
int __memp_open __P((DB_ENV *));
int __memp_close __P((DB_ENV *));
void __memp_dump_region __P((DB_ENV *, char *, FILE *));
int __mp_xxx_fh __P((DB_MPOOLFILE *, DB_FH **));
#endif /* _mp_ext_h_ */
