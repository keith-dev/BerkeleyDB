/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_mutex_ext_h_
#define	_mutex_ext_h_
#if defined(__cplusplus)
extern "C" {
#endif
int __db_fcntl_mutex_init __P((DB_ENV *, DB_MUTEX *, u_int32_t));
int __db_fcntl_mutex_lock __P((DB_ENV *, DB_MUTEX *, DB_FH *));
int __db_fcntl_mutex_unlock __P((DB_ENV *, DB_MUTEX *));
int __db_fcntl_mutex_destroy __P((DB_MUTEX *));
int __db_pthread_mutex_init __P((DB_ENV *, DB_MUTEX *, u_int32_t));
int __db_pthread_mutex_lock __P((DB_ENV *, DB_MUTEX *));
int __db_pthread_mutex_unlock __P((DB_ENV *, DB_MUTEX *));
int __db_pthread_mutex_destroy __P((DB_MUTEX *));
int __db_tas_mutex_init __P((DB_ENV *, DB_MUTEX *, u_int32_t));
int __db_tas_mutex_lock __P((DB_ENV *, DB_MUTEX *));
int __db_tas_mutex_unlock __P((DB_ENV *, DB_MUTEX *));
int __db_tas_mutex_destroy __P((DB_MUTEX *));
int __db_mutex_alloc __P((DB_ENV *, REGINFO *, int, DB_MUTEX **));
void __db_mutex_free __P((DB_ENV *, REGINFO *, DB_MUTEX *));
void __db_shreg_locks_clear __P((DB_MUTEX *, REGINFO *, REGMAINT *));
void __db_shreg_locks_destroy __P((REGINFO *, REGMAINT *));
int __db_shreg_mutex_init __P((DB_ENV *, DB_MUTEX *, u_int32_t, u_int32_t, REGINFO *, REGMAINT *));
void __db_shreg_maintinit __P((REGINFO *, void *addr, size_t));
#if defined(__cplusplus)
}
#endif
#endif /* _mutex_ext_h_ */
