/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 1997, 1998, 1999, 2000
 *	Sleepycat Software.  All rights reserved.
 *
 * $Id: java_info.h,v 11.12 2000/06/01 14:17:58 dda Exp $
 */

#ifndef _JAVA_INFO_H_
#define	_JAVA_INFO_H_

/*
 * "Info" classes for Java implementation of Berkeley DB API.
 * These classes hold extra information for which there is
 * no room or counterpart in the base classes used in the C API.
 * In the case of a DBT, the DBT_javainfo class is stored in the
 * 'private' variable of the java Dbt, and the DBT_javainfo is subclassed
 * from a DBT.  In the case of DB and DB_ENV, the appropriate
 * info objects are pointed to by the DB and DB_ENV objects.
 * This is convenient to implement callbacks.
 */

/****************************************************************
 *
 * These functions are used internally to help manage callbacks that
 * must go from C back into Java that have no arguments from
 * which we can extract context.  These are handled via
 * a set of fixed C callback functions for each callback type.
 * The context_free_callback manages the allocation/deallocation
 * of the functions.
 *
 */

typedef struct _context_free_callback
{
	jobject callback_object_;
	int callback_slot_;
	int size_;
}
CONTEXT_FREE_CALLBACK;   /* used with all 'cfc' functions */

extern CONTEXT_FREE_CALLBACK *cfc_construct(int size);
extern void cfc_destroy(CONTEXT_FREE_CALLBACK *cfc, JNIEnv *jnienv);

/* Allocate a new slot, or if appropriate, just return the old slot. */
extern int cfc_get_new_slot(CONTEXT_FREE_CALLBACK *cfc,
			    jobject new_object,
			    void **array_of_containers,
			    void *this_container);

extern void cfc_set_callback_object(CONTEXT_FREE_CALLBACK *cfc,
				    JNIEnv *jnienv, jobject val);

extern int cfcget_slot(CONTEXT_FREE_CALLBACK *cfc);
extern jobject get_callback_object(CONTEXT_FREE_CALLBACK *cfc);

/****************************************************************
 *
 * Declaration of class DBT_javainfo
 *
 * A DBT_javainfo is created whenever a Dbt (java) object is created,
 * and a pointer to it is stored in its private info storage.
 * It is subclassed from DBT, because we must retain some extra
 * information in it while it is in use.  In particular, when
 * a java array is associated with it, we need to keep a Globally
 * Locked reference to it so it is not GC'd.  This reference is
 * destroyed when the Dbt is GC'd.
 */
typedef struct _dbt_javainfo
{
	DBT dbt;
	jbyteArray array_;
	int offset_;
}
DBT_JAVAINFO;  /* used with all 'dbtji' functions */

extern DBT_JAVAINFO *dbjit_construct();
extern void dbjit_release(DBT_JAVAINFO *dbjit, JNIEnv *jnienv);

/****************************************************************
 *
 * Declaration of class DB_ENV_JAVAINFO
 *
 * A DB_ENV_JAVAINFO is allocated and stuffed into the cj_internal
 * and the db_errpfx for every DB_ENV created.  It holds a
 * little extra info that is needed to support callbacks.
 *
 * There's a bit of trickery here, because we have built this
 * above a layer that has a C function callback that gets
 * invoked when an error occurs.  One of the C callback's arguments
 * is the prefix from the DB_ENV, but since we stuffed a pointer
 * to our own DB_ENV_JAVAINFO into the prefix, we get that object as an
 * argument to the C callback.  Thus, the C callback can have
 * access to much more than just the prefix, and it needs that
 * to call back into the Java enviroment.
 *
 * The DB_ENV_JAVAINFO object holds a copy of the Java Virtual Machine,
 * which is needed to attach to the current running thread
 * whenever we need to make a callback.  (This is more reliable
 * than our previous approach, which was to save the thread
 * that created the DbEnv).  It also has the Java callback object,
 * as well as a 'default' callback object that is used when the
 * caller sets the callback to null.  It also has the original
 * error prefix, since we overwrote the one in the DB_ENV.
 * There are also fields that are unrelated to the handling
 * of callbacks, but are convenient to attach to a DB_ENV.
 *
 * Note: We assume that the Java layer is the only one
 * fiddling with the contents of db_errpfx, db_errcall, cj_internal
 * for a DB_ENV that was created via Java.  Since the Java layer should
 * have the only pointer to such a DB_ENV, this should be true.
 */
typedef struct _db_env_javainfo
{
	JavaVM *javavm_;
	int is_dbopen_;
	char *errpfx_;
	jobject jdbref_;
	jobject jenvref_;
	jobject default_errcall_;
	jobject errcall_;
	unsigned char *conflict_;
	jobject feedback_;
	jobject recovery_init_;
}
DB_ENV_JAVAINFO;  /* used with all 'dbjie' functions */

/* create/initialize an object */
extern DB_ENV_JAVAINFO *dbjie_construct(JNIEnv *jnienv,
		       jobject default_errcall,
		       int is_dbopen);

/* release all objects held by this this one */
extern void dbjie_dealloc(DB_ENV_JAVAINFO *, JNIEnv *jnienv);

/* free this object, releasing anything allocated on its behalf */
extern void dbjie_destroy(DB_ENV_JAVAINFO *, JNIEnv *jnienv);

/* This gets the environment for the current thread */
extern JNIEnv *dbjie_get_jnienv(DB_ENV_JAVAINFO *);

extern void dbjie_set_errpfx(DB_ENV_JAVAINFO *, JNIEnv *jnienv,
			     jstring errpfx);
extern jstring dbjie_get_errpfx(DB_ENV_JAVAINFO *, JNIEnv *jnienv);
extern void dbjie_set_errcall(DB_ENV_JAVAINFO *, JNIEnv *jnienv,
			      jobject new_errcall);
extern void dbjie_set_conflict(DB_ENV_JAVAINFO *, unsigned char *v);
extern void dbjie_set_feedback_object(DB_ENV_JAVAINFO *, JNIEnv *jnienv,
				      DB_ENV *dbenv, jobject value);
extern void dbjie_call_feedback(DB_ENV_JAVAINFO *, DB_ENV *dbenv, jobject jenv,
				int opcode, int percent);
extern void dbjie_set_recovery_init_object(DB_ENV_JAVAINFO *, JNIEnv *jnienv,
					   DB_ENV *dbenv, jobject value);
extern int dbjie_call_recovery_init(DB_ENV_JAVAINFO *, DB_ENV *dbenv,
				    jobject jenv);
extern jobject dbjie_get_errcall(DB_ENV_JAVAINFO *) ;
extern int dbjie_is_dbopen(DB_ENV_JAVAINFO *);

/****************************************************************
 *
 * Declaration of class DB_JAVAINFO
 *
 * A DB_JAVAINFO is allocated and stuffed into the cj_internal field
 * for every DB created.  It holds a little extra info that is needed
 * to support callbacks.
 *
 * Note: We assume that the Java layer is the only one
 * fiddling with the contents of cj_internal
 * for a DB that was created via Java.  Since the Java layer should
 * have the only pointer to such a DB, this should be true.
 */
typedef struct _db_javainfo
{
	JavaVM *javavm_;
	jobject jdbref_;
	jobject feedback_;
	jint flags_;
} DB_JAVAINFO;

/* create/initialize an object */
extern DB_JAVAINFO *dbji_construct(JNIEnv *jnienv, jint flags);

/* release all objects held by this this one */
extern void dbji_dealloc(DB_JAVAINFO *, JNIEnv *jnienv);

/* free this object, releasing anything allocated on its behalf */
extern void dbji_destroy(DB_JAVAINFO *, JNIEnv *jnienv);

/* This gets the environment for the current thread */
extern JNIEnv *dbji_get_jnienv();
extern jint dbji_get_flags();

extern void dbji_set_feedback_object(DB_JAVAINFO *, JNIEnv *jnienv, DB *db, jobject value);
extern void dbji_call_feedback(DB_JAVAINFO *, DB *db, jobject jdb,
			       int opcode, int percent);

#endif /* !_JAVA_INFO_H_ */
