/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class com_sleepycat_db_DbTxn */

#ifndef _Included_com_sleepycat_db_DbTxn
#define _Included_com_sleepycat_db_DbTxn
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     com_sleepycat_db_DbTxn
 * Method:    abort
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sleepycat_db_DbTxn_abort
  (JNIEnv *, jobject);

/*
 * Class:     com_sleepycat_db_DbTxn
 * Method:    commit
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_com_sleepycat_db_DbTxn_commit
  (JNIEnv *, jobject, jint);

/*
 * Class:     com_sleepycat_db_DbTxn
 * Method:    finalize
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sleepycat_db_DbTxn_finalize
  (JNIEnv *, jobject);

/*
 * Class:     com_sleepycat_db_DbTxn
 * Method:    id
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_com_sleepycat_db_DbTxn_id
  (JNIEnv *, jobject);

/*
 * Class:     com_sleepycat_db_DbTxn
 * Method:    prepare
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_com_sleepycat_db_DbTxn_prepare
  (JNIEnv *, jobject);

#ifdef __cplusplus
}
#endif
#endif
