/*
 * $Id: db_config.h,v 11.39 2001/11/10 04:59:05 mjc Exp $
 */

/* Define if you are building a version for running the test suite. */
/* #undef CONFIG_TEST */

/* Define if you want a debugging version. */
/* #undef DEBUG */
#if defined(_DEBUG)
#if !defined(DEBUG)
#define DEBUG 1
#endif
#endif

/* Define if you want a version that logs read operations. */
/* #undef DEBUG_ROP */

/* Define if you want a version that logs write operations. */
/* #undef DEBUG_WOP */

/* Define if you want a version with run-time diagnostic checking. */
/* #undef DIAGNOSTIC */

/* Define if you have the `clock_gettime' function. */
/* #undef HAVE_CLOCK_GETTIME */

/* Define if you have the <dirent.h> header file, and it defines `DIR'. */
/* #undef HAVE_DIRENT_H */

/* Define if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define if you have EXIT_SUCCESS/EXIT_FAILURE #defines. */
#define HAVE_EXIT_SUCCESS 1

/* Define if fcntl/F_SETFD denies child access to file descriptors. */
/* #undef HAVE_FCNTL_F_SETFD */

/* Define if allocated filesystem blocks are not zeroed. */
#define HAVE_FILESYSTEM_NOTZERO 1

/* Define if you have the `getcwd' function. */
#define HAVE_GETCWD 1

/* Define if you have the `getopt' function. */
/* #undef HAVE_GETOPT */

/* Define if you have the `gettimeofday' function. */
/* #undef HAVE_GETTIMEOFDAY */

/* Define if you have the `getuid' function. */
/* #undef HAVE_GETUID */

/* Define if you have the <inttypes.h> header file. */
/* #undef HAVE_INTTYPES_H */

/* Define if you have the `nsl' library (-lnsl). */
/* #undef HAVE_LIBNSL */

/* Define if you have the `memcmp' function. */
#define HAVE_MEMCMP 1

/* Define if you have the `memcpy' function. */
#define HAVE_MEMCPY 1

/* Define if you have the `memmove' function. */
#define HAVE_MEMMOVE 1

/* Define if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define if you have the `mlock' function. */
/* #undef HAVE_MLOCK */

/* Define if you have the `mmap' function. */
/* #undef HAVE_MMAP */

/* Define if you have the `munlock' function. */
/* #undef HAVE_MUNLOCK */

/* Define if you have the `munmap' function. */
/* #undef HAVE_MUNMAP */

/* Define to use the GCC compiler and 68K assembly language mutexes. */
/* #undef HAVE_MUTEX_68K_GCC_ASSEMBLY */

/* Define to use the AIX _check_lock mutexes. */
/* #undef HAVE_MUTEX_AIX_CHECK_LOCK */

/* Define to use the GCC compiler and Alpha assembly language mutexes. */
/* #undef HAVE_MUTEX_ALPHA_GCC_ASSEMBLY */

/* Define to use the GCC compiler and PaRisc assembly language mutexes. */
/* #undef HAVE_MUTEX_HPPA_GCC_ASSEMBLY */

/* Define to use the msem_XXX mutexes on HP-UX. */
/* #undef HAVE_MUTEX_HPPA_MSEM_INIT */

/* Define to use the GCC compiler and IA64 assembly language mutexes. */
/* #undef HAVE_MUTEX_IA64_GCC_ASSEMBLY */

/* Define to use the msem_XXX mutexes on systems other than HP-UX. */
/* #undef HAVE_MUTEX_MSEM_INIT */

/* Define to use the GCC compiler and PowerPC assembly language mutexes. */
/* #undef HAVE_MUTEX_PPC_GCC_ASSEMBLY */

/* Define to use POSIX 1003.1 pthread_XXX mutexes. */
/* #undef HAVE_MUTEX_PTHREADS */

/* Define to use Reliant UNIX initspin mutexes. */
/* #undef HAVE_MUTEX_RELIANTUNIX_INITSPIN */

/* Define to use the SCO compiler and x86 assembly language mutexes. */
/* #undef HAVE_MUTEX_SCO_X86_CC_ASSEMBLY */

/* Define to use the obsolete POSIX 1003.1 sema_XXX mutexes. */
/* #undef HAVE_MUTEX_SEMA_INIT */

/* Define to use the SGI XXX_lock mutexes. */
/* #undef HAVE_MUTEX_SGI_INIT_LOCK */

/* Define to use the Solaris _lock_XXX mutexes. */
/* #undef HAVE_MUTEX_SOLARIS_LOCK_TRY */

/* Define to use the Solaris lwp threads mutexes. */
/* #undef HAVE_MUTEX_SOLARIS_LWP */

/* Define to use the GCC compiler and Sparc assembly language mutexes. */
/* #undef HAVE_MUTEX_SPARC_GCC_ASSEMBLY */

/* Define if fast mutexes available. */
#define HAVE_MUTEX_THREADS 1

/* Define to use the UNIX International mutexes. */
/* #undef HAVE_MUTEX_UI_THREADS */

/* Define to use the UTS compiler and assembly language mutexes. */
/* #undef HAVE_MUTEX_UTS_CC_ASSEMBLY */

/* Define to use VMS mutexes. */
/* #undef HAVE_MUTEX_VMS */

/* Define to use VxWorks mutexes. */
/* #undef HAVE_MUTEX_VXWORKS */

/* Define to use Windows mutexes. */
#define HAVE_MUTEX_WIN32 1

/* Define to use the GCC compiler and x86 assembly language mutexes. */
/* #undef HAVE_MUTEX_X86_GCC_ASSEMBLY */

/* Define if you have the <ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_NDIR_H */

/* Define if you have the `pread' function. */
/* #undef HAVE_PREAD */

/* Define if you have the `pstat_getdynamic' function. */
/* #undef HAVE_PSTAT_GETDYNAMIC */

/* Define if you have the `pwrite' function. */
/* #undef HAVE_PWRITE */

/* Define if building on QNX. */
/* #undef HAVE_QNX */

/* Define if you have the `qsort' function. */
#define HAVE_QSORT 1

/* Define if you have the `raise' function. */
#define HAVE_RAISE 1

/* Define if building RPC client/server. */
/* #undef HAVE_RPC */

/* Define if you have the `sched_yield' function. */
/* #undef HAVE_SCHED_YIELD */

/* Define if you have the `select' function. */
/* #undef HAVE_SELECT */

/* Define if you have the `shmget' function. */
/* #undef HAVE_SHMGET */

/* Define if you have the `snprintf' function. */
#define HAVE_SNPRINTF 1

/* Define if you have the <stdint.h> header file. */
/* #undef HAVE_STDINT_H */

/* Define if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define if you have the `strcasecmp' function. */
/* #undef HAVE_STRCASECMP */

/* Define if you have the `strerror' function. */
#define HAVE_STRERROR 1

/* Define if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define if `st_blksize' is member of `struct stat'. */
/* #undef HAVE_STRUCT_STAT_ST_BLKSIZE */

/* Define if you have the `sysconf' function. */
/* #undef HAVE_SYSCONF */

/* Define if you have the <sys/dir.h> header file, and it defines `DIR'. */
/* #undef HAVE_SYS_DIR_H */

/* Define if you have the <sys/ndir.h> header file, and it defines `DIR'. */
/* #undef HAVE_SYS_NDIR_H */

/* Define if you have the <sys/select.h> header file. */
/* #undef HAVE_SYS_SELECT_H */

/* Define if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define if you have the <sys/time.h> header file. */
/* #undef HAVE_SYS_TIME_H */

/* Define if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define if you have the `vsnprintf' function. */
#define HAVE_VSNPRINTF 1

/* Define if building VxWorks. */
/* #undef HAVE_VXWORKS */

/* Define if you have the `yield' function. */
/* #undef HAVE_YIELD */

/* Define if you have the `_fstati64' function. */
#define HAVE__FSTATI64 1

/* Define if your sprintf returns a pointer, not a length. */
/* #undef SPRINTF_RET_CHARPNT */

/* Define if the `S_IS*' macros in <sys/stat.h> do not work properly. */
/* #undef STAT_MACROS_BROKEN */

/* Define if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>. */
/* #undef TIME_WITH_SYS_TIME */

/* Define to mask harmless unitialized memory read/writes. */
/* #undef UMRW */

/* Define if your processor stores words with the most significant byte first
   (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */

/* Number of bits in a file offset, on hosts where this is settable. */
/* #undef _FILE_OFFSET_BITS */

/* Define for large files, on AIX-style hosts. */
/* #undef _LARGE_FILES */

/* Define to empty if `const' does not conform to ANSI C. */
/* #undef const */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef mode_t */

/* Define to `long' if <sys/types.h> does not define. */
/* #undef off_t */

/* Define to `int' if <sys/types.h> does not define. */
/* #undef pid_t */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef size_t */

/*
 * Exit success/failure macros.
 */
#ifndef	HAVE_EXIT_SUCCESS
#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0
#endif

/*
 * Don't step on the namespace.  Other libraries may have their own
 * implementations of these functions, we don't want to use their
 * implementations or force them to use ours based on the load order.
 */
#ifndef	HAVE_GETCWD
#define	getcwd		__db_Cgetcwd
#endif
#ifndef	HAVE_MEMCMP
#define	memcmp		__db_Cmemcmp
#endif
#ifndef	HAVE_MEMCPY
#define	memcpy		__db_Cmemcpy
#endif
#ifndef	HAVE_MEMMOVE
#define	memmove		__db_Cmemmove
#endif
#ifndef	HAVE_RAISE
#define	raise		__db_Craise
#endif
#ifndef	HAVE_SNPRINTF
#define	snprintf	__db_Csnprintf
#endif
#ifndef	HAVE_STRCASECMP
#define	strcasecmp	__db_Cstrcasecmp
#endif
#ifndef	HAVE_STRERROR
#define	strerror	__db_Cstrerror
#endif
#ifndef	HAVE_VSNPRINTF
#define	vsnprintf	__db_Cvsnprintf
#endif

/*
 * XXX
 * The following is not part of the automatic configuration setup, but
 * provides the information necessary to build Berkeley DB on Windows.
 */
#include <sys/types.h>
#include <sys/stat.h>

#include <direct.h>
#include <fcntl.h>
#include <io.h>
#include <limits.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

/*
 * To build Tcl interface libraries, the include path must be configured to
 * use the directory containing <tcl.h>, usually the include directory in
 * the Tcl distribution.
 */
#ifdef DB_TCL_SUPPORT
#include <tcl.h>
#endif

#define	WIN32_LEAN_AND_MEAN
#include <windows.h>

/*
 * All of the necessary includes have been included, ignore the #includes
 * in the Berkeley DB source files.
 */
#define	NO_SYSTEM_INCLUDES

/*
 * Win32 has fsync, getcwd, snprintf and vsnprintf, but under different names.
 */
#define	fsync(fd)		_commit(fd)
#define	getcwd(buf, size)	_getcwd(buf, size)
#define	snprintf		_snprintf
#define	vsnprintf		_vsnprintf

/*
 * Win32 does not define getopt and friends in any header file, so we must.
 */
#if defined(__cplusplus)
extern "C" {
#endif
extern int optind;
extern char *optarg;
extern int getopt(int, char * const *, const char *);
#if defined(__cplusplus)
}
#endif

/*
 * We use DB_WIN32 much as one would use _WIN32, to determine that we're
 * using an operating system environment that supports Win32 calls
 * and semantics.  We don't use _WIN32 because cygwin/gcc also defines
 * that, even though it closely emulates the Unix environment.
 */
#define DB_WIN32 1

/*
 * This is a grievous hack -- once we've included windows.h, we have no choice
 * but to use ANSI-style varargs (because it pulls in stdarg.h for us).  DB's
 * code decides which type of varargs to use based on the state of __STDC__.
 * Sensible.  Unfortunately, Microsoft's compiler _doesn't_ define __STDC__
 * unless you invoke it with arguments turning OFF all vendor extensions.  Even
 * more unfortunately, if we do that, it fails to parse windows.h!!!!!  So, we
 * define __STDC__ here, after windows.h comes in.  Note: the compiler knows
 * we've defined it, and starts enforcing strict ANSI compilance from this point
 * on.
 */
#define __STDC__ 1
