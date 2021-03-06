/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	__db_AUTO_H
#define	__db_AUTO_H
#define	DB___db_addrem	41
typedef struct ___db_addrem_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	u_int32_t	opcode;
	int32_t	fileid;
	db_pgno_t	pgno;
	u_int32_t	indx;
	u_int32_t	nbytes;
	DBT	hdr;
	DBT	dbt;
	DB_LSN	pagelsn;
} __db_addrem_args;

#define	DB___db_big	43
typedef struct ___db_big_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	u_int32_t	opcode;
	int32_t	fileid;
	db_pgno_t	pgno;
	db_pgno_t	prev_pgno;
	db_pgno_t	next_pgno;
	DBT	dbt;
	DB_LSN	pagelsn;
	DB_LSN	prevlsn;
	DB_LSN	nextlsn;
} __db_big_args;

#define	DB___db_ovref	44
typedef struct ___db_ovref_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	int32_t	adjust;
	DB_LSN	lsn;
} __db_ovref_args;

#define	DB___db_debug	47
typedef struct ___db_debug_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	DBT	op;
	int32_t	fileid;
	DBT	key;
	DBT	data;
	u_int32_t	arg_flags;
} __db_debug_args;

#define	DB___db_noop	48
typedef struct ___db_noop_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DB_LSN	prevlsn;
} __db_noop_args;

#define	DB___db_pg_alloc	49
typedef struct ___db_pg_alloc_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	DB_LSN	meta_lsn;
	db_pgno_t	meta_pgno;
	DB_LSN	page_lsn;
	db_pgno_t	pgno;
	u_int32_t	ptype;
	db_pgno_t	next;
	db_pgno_t	last_pgno;
} __db_pg_alloc_args;

#define	DB___db_pg_free	50
typedef struct ___db_pg_free_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DB_LSN	meta_lsn;
	db_pgno_t	meta_pgno;
	DBT	header;
	db_pgno_t	next;
	db_pgno_t	last_pgno;
} __db_pg_free_args;

#define	DB___db_cksum	51
typedef struct ___db_cksum_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
} __db_cksum_args;

#define	DB___db_pg_freedata	52
typedef struct ___db_pg_freedata_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DB_LSN	meta_lsn;
	db_pgno_t	meta_pgno;
	DBT	header;
	db_pgno_t	next;
	db_pgno_t	last_pgno;
	DBT	data;
} __db_pg_freedata_args;

#define	DB___db_pg_prepare	53
typedef struct ___db_pg_prepare_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
} __db_pg_prepare_args;

#define	DB___db_pg_new	54
typedef struct ___db_pg_new_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DB_LSN	meta_lsn;
	db_pgno_t	meta_pgno;
	DBT	header;
	db_pgno_t	next;
} __db_pg_new_args;

#define	DB___db_pg_init	60
typedef struct ___db_pg_init_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DBT	header;
	DBT	data;
} __db_pg_init_args;

#define	DB___db_pg_sort	61
typedef struct ___db_pg_sort_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	meta;
	DB_LSN	meta_lsn;
	db_pgno_t	last_free;
	DB_LSN	last_lsn;
	db_pgno_t	last_pgno;
	DBT	list;
} __db_pg_sort_args;

#endif
