/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_clib_ext_h_
#define	_clib_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef HAVE_ABORT
void abort __P((void));
#endif
#ifndef HAVE_ATOI
int atoi __P((const char *));
#endif
#ifndef HAVE_ATOL
long atol __P((const char *));
#endif
char *__db_ctime __P((const time_t *, char *));
#if defined(HAVE_REPLICATION_THREADS)
int __db_getaddrinfo __P((DB_ENV *, const char *, u_int, const char *, const ADDRINFO *, ADDRINFO **));
#endif
#if defined(HAVE_REPLICATION_THREADS)
void __db_freeaddrinfo __P((DB_ENV *, ADDRINFO *));
#endif
#ifndef HAVE_GETCWD
char *getcwd __P((char *, size_t));
#endif
#ifndef HAVE_GETOPT
int getopt __P((int, char * const *, const char *));
#endif
#ifndef HAVE_ISALPHA
int isalpha __P((int));
#endif
#ifndef HAVE_ISDIGIT
int isdigit __P((int));
#endif
#ifndef HAVE_ISPRINT
int isprint __P((int));
#endif
#ifndef HAVE_ISSPACE
int isspace __P((int));
#endif
#ifndef HAVE_MEMCMP
int memcmp __P((const void *, const void *, size_t));
#endif
#ifndef HAVE_MEMCPY
void *memcpy __P((void *, const void *, size_t));
#endif
#ifndef HAVE_MEMMOVE
void *memmove __P((void *, const void *, size_t));
#endif
#ifndef HAVE_PRINTF
int printf __P((const char *, ...));
#endif
#ifndef HAVE_PRINTF
int fprintf __P((FILE *, const char *, ...));
#endif
#ifndef HAVE_PRINTF
int vfprintf __P((FILE *, const char *, va_list));
#endif
#ifndef HAVE_RAISE
int raise __P((int));
#endif
#ifndef HAVE_RAND
int rand __P((void));
void srand __P((unsigned int));
#endif
#ifndef HAVE_SNPRINTF
int snprintf __P((char *, size_t, const char *, ...));
#endif
#ifndef HAVE_VSNPRINTF
int vsnprintf __P((char *, size_t, const char *, va_list));
#endif
#ifndef HAVE_STRCASECMP
int strcasecmp __P((const char *, const char *));
#endif
#ifndef HAVE_STRCASECMP
int strncasecmp __P((const char *, const char *, size_t));
#endif
#ifndef HAVE_STRCAT
char *strcat __P((char *, const char *));
#endif
#ifndef HAVE_STRCHR
char *strchr __P((const char *,  int));
#endif
#ifndef HAVE_STRDUP
char *strdup __P((const char *));
#endif
#ifndef HAVE_STRERROR
char *strerror __P((int));
#endif
#ifndef HAVE_STRNCAT
char *strncat __P((char *, const char *, size_t));
#endif
#ifndef HAVE_STRNCMP
int strncmp __P((const char *, const char *, size_t));
#endif
#ifndef HAVE_STRRCHR
char *strrchr __P((const char *, int));
#endif
#ifndef HAVE_STRSEP
char *strsep __P((char **, const char *));
#endif
#ifndef HAVE_STRTOL
long strtol __P((const char *, char **, int));
#endif
#ifndef HAVE_STRTOUL
unsigned long strtoul __P((const char *, char **, int));
#endif

#if defined(__cplusplus)
}
#endif
#endif /* !_clib_ext_h_ */
