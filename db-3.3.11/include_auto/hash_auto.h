/* Do not edit: automatically built by gen_rec.awk. */

#ifndef	ham_AUTO_H
#define	ham_AUTO_H
#define	DB_ham_insdel	21
typedef struct _ham_insdel_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	u_int32_t	opcode;
	int32_t	fileid;
	db_pgno_t	pgno;
	u_int32_t	ndx;
	DB_LSN	pagelsn;
	DBT	key;
	DBT	data;
} __ham_insdel_args;

#define	DB_ham_newpage	22
typedef struct _ham_newpage_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	u_int32_t	opcode;
	int32_t	fileid;
	db_pgno_t	prev_pgno;
	DB_LSN	prevlsn;
	db_pgno_t	new_pgno;
	DB_LSN	pagelsn;
	db_pgno_t	next_pgno;
	DB_LSN	nextlsn;
} __ham_newpage_args;

#define	DB_ham_splitmeta	23
typedef struct _ham_splitmeta_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	u_int32_t	bucket;
	u_int32_t	ovflpoint;
	u_int32_t	spares;
	DB_LSN	metalsn;
} __ham_splitmeta_args;

#define	DB_ham_splitdata	24
typedef struct _ham_splitdata_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	u_int32_t	opcode;
	db_pgno_t	pgno;
	DBT	pageimage;
	DB_LSN	pagelsn;
} __ham_splitdata_args;

#define	DB_ham_replace	25
typedef struct _ham_replace_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	u_int32_t	ndx;
	DB_LSN	pagelsn;
	int32_t	off;
	DBT	olditem;
	DBT	newitem;
	u_int32_t	makedup;
} __ham_replace_args;

#define	DB_ham_newpgno	26
typedef struct _ham_newpgno_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	u_int32_t	opcode;
	int32_t	fileid;
	db_pgno_t	pgno;
	db_pgno_t	free_pgno;
	u_int32_t	old_type;
	db_pgno_t	old_pgno;
	u_int32_t	new_type;
	DB_LSN	pagelsn;
	DB_LSN	metalsn;
} __ham_newpgno_args;

#define	DB_ham_ovfl	27
typedef struct _ham_ovfl_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	start_pgno;
	u_int32_t	npages;
	db_pgno_t	free_pgno;
	u_int32_t	ovflpoint;
	DB_LSN	metalsn;
} __ham_ovfl_args;

#define	DB_ham_copypage	28
typedef struct _ham_copypage_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DB_LSN	pagelsn;
	db_pgno_t	next_pgno;
	DB_LSN	nextlsn;
	db_pgno_t	nnext_pgno;
	DB_LSN	nnextlsn;
	DBT	page;
} __ham_copypage_args;

#define	DB_ham_metagroup	29
typedef struct _ham_metagroup_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	u_int32_t	bucket;
	db_pgno_t	pgno;
	DB_LSN	metalsn;
	DB_LSN	pagelsn;
} __ham_metagroup_args;

#define	DB_ham_groupalloc1	30
typedef struct _ham_groupalloc1_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	DB_LSN	metalsn;
	DB_LSN	mmetalsn;
	db_pgno_t	start_pgno;
	u_int32_t	num;
} __ham_groupalloc1_args;

#define	DB_ham_groupalloc2	31
typedef struct _ham_groupalloc2_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	DB_LSN	meta_lsn;
	DB_LSN	alloc_lsn;
	db_pgno_t	start_pgno;
	u_int32_t	num;
	db_pgno_t	free;
} __ham_groupalloc2_args;

#define	DB_ham_groupalloc	32
typedef struct _ham_groupalloc_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	DB_LSN	meta_lsn;
	db_pgno_t	start_pgno;
	u_int32_t	num;
	db_pgno_t	free;
} __ham_groupalloc_args;

#define	DB_ham_curadj	33
typedef struct _ham_curadj_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_pgno_t	pgno;
	u_int32_t	indx;
	u_int32_t	len;
	u_int32_t	dup_off;
	int	add;
	int	is_dup;
	u_int32_t	order;
} __ham_curadj_args;

#define	DB_ham_chgpg	34
typedef struct _ham_chgpg_args {
	u_int32_t type;
	DB_TXN *txnid;
	DB_LSN prev_lsn;
	int32_t	fileid;
	db_ham_mode	mode;
	db_pgno_t	old_pgno;
	db_pgno_t	new_pgno;
	u_int32_t	old_indx;
	u_int32_t	new_indx;
} __ham_chgpg_args;

#endif
