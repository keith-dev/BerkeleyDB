/* Do not edit: automatically built by gen_rec.awk. */

#ifndef qam_AUTO_H
#define qam_AUTO_H

#define	DB_qam_inc	(DB_qam_BEGIN + 1)

typedef struct _qam_inc_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	DB_LSN 	lsn;
} __qam_inc_args;

int __qam_inc_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, DB_LSN *));
int __qam_inc_print __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __qam_inc_read __P((void *, __qam_inc_args **));

#define	DB_qam_incfirst	(DB_qam_BEGIN + 2)

typedef struct _qam_incfirst_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_recno_t	recno;
} __qam_incfirst_args;

int __qam_incfirst_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, db_recno_t));
int __qam_incfirst_print __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __qam_incfirst_read __P((void *, __qam_incfirst_args **));

#define	DB_qam_mvptr	(DB_qam_BEGIN + 3)

typedef struct _qam_mvptr_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	u_int32_t	opcode;
	int32_t	fileid;
	db_recno_t	old_first;
	db_recno_t	new_first;
	db_recno_t	old_cur;
	db_recno_t	new_cur;
	DB_LSN 	metalsn;
} __qam_mvptr_args;

int __qam_mvptr_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, u_int32_t, int32_t, db_recno_t, db_recno_t, db_recno_t, db_recno_t, DB_LSN *));
int __qam_mvptr_print __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __qam_mvptr_read __P((void *, __qam_mvptr_args **));

#define	DB_qam_del	(DB_qam_BEGIN + 4)

typedef struct _qam_del_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	DB_LSN 	lsn;
	db_pgno_t	pgno;
	u_int32_t	indx;
	db_recno_t	recno;
} __qam_del_args;

int __qam_del_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t));
int __qam_del_print __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __qam_del_read __P((void *, __qam_del_args **));

#define	DB_qam_add	(DB_qam_BEGIN + 5)

typedef struct _qam_add_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	DB_LSN 	lsn;
	db_pgno_t	pgno;
	u_int32_t	indx;
	db_recno_t	recno;
	DBT	data;
	u_int32_t	vflag;
	DBT	olddata;
} __qam_add_args;

int __qam_add_log __P((DB_ENV *, DB_TXN *, DB_LSN *, u_int32_t, int32_t, DB_LSN *, db_pgno_t, u_int32_t, db_recno_t, const DBT *, u_int32_t, const DBT *));
int __qam_add_print __P((DB_ENV *, DBT *, DB_LSN *, int, void *));
int __qam_add_read __P((void *, __qam_add_args **));
int __qam_init_print __P((DB_ENV *));
#endif
